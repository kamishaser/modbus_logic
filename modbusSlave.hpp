#pragma once

#include <cstdint>
#include "modbusBuffer.hpp"
#include "modbusTransceiver.h"

#ifndef MODBUS_SLAVE_HANDLER_COUNT
#define MODBUS_SLAVE_HANDLER_COUNT 32
#endif

class ModbusSlaveFunctionHandlerInterface
{
public:
	virtual ~ModbusSlaveFunctionHandlerInterface(){}

	//измеряет ожидаемую длину пакета Возращает
	// 0 - если невозможно измерить или недостаточно данных для измерения
	// >=4 длину пакета
	//
	virtual uint8_t expectedPacketSize(ModbusBuffer*) { return 0; }
	virtual void handle(ModbusBuffer*) {}
	
};

class ModbusSlave
{
	ModbusBuffer buffers[2];
	
	ModbusTransceiverInterface* transceiver;

	uint32_t requestCounter = 0;

	uint16_t oldCrc = 0;
	
	uint8_t currentBufferNumber = 0;
	uint8_t slaveID;
	uint8_t expectedPacketSize = 0;
	

	//массив интерфейсов обработчиков modbus запросов
	ModbusSlaveFunctionHandlerInterface* handlers[MODBUS_SLAVE_HANDLER_COUNT];

	enum class Stage
	{
		reception,
		transmition,
	};
	Stage stage = Stage::reception;

public:

	ModbusSlave(ModbusTransceiverInterface* Transceiver, uint8_t slaveid)
		:slaveID(slaveid), transceiver(Transceiver), handlers{ nullptr }
	{}

	void setSlaveID(uint8_t id)
	{
		slaveID = id;
	}
	uint8_t getSlaveID() const
	{
		return slaveID;
	}
	ModbusBuffer* getBuffer()
	{
		return &buffers[currentBufferNumber];
	}

	

	//аргументы (handler, func_code)
	//handler принимает (requestBuffer*, responseBuffer*, requestSize)
	//и возвращает длину ответа если вернёт 0 - ответ не отправится
	//func_code - код функции, которую обрабатывает. Обязательно < 64
	//если эта цифра не переопределено макросом MODBUS_SLAVE_HANDLER_COUNT
	//рекомендуется использовать лямбды с захватом
	bool bindHandler(
		ModbusSlaveFunctionHandlerInterface* handler, uint16_t func_code)
	{
		if (func_code >= MODBUS_SLAVE_HANDLER_COUNT)
			return false;
		if (handlers[func_code] != nullptr)
			return false;
			
		handlers[func_code] = handler;
		return true;
	}

	ModbusSlaveFunctionHandlerInterface* getHandler(uint16_t func_code)
	{
		return handlers[func_code];
	}

	

	static void generateErrorResponce(ModbusBuffer* buffer, 
		uint8_t func_code, uint8_t error_code)
	{
		buffer->start_write();
		buffer->write<uint8_t>(0);
		buffer->write<uint8_t>(func_code | 0x80);
		buffer->write(error_code);
		buffer->stop();
	}

	uint32_t getRequestCount() const
	{
		return requestCounter;
	}

	

	void update()
	{
		switch (stage)
		{
		case Stage::reception:
			updateReception();
			break;

		case Stage::transmition:
			updateTransmition();
			break;
		}


	}

	void processRequest()
	{
		ModbusBuffer* currentBuffer = getBuffer();
		currentBuffer->start_read();
		uint8_t slave, func_code;
		currentBuffer->read(slave);
		currentBuffer->read(func_code);

		// 1 в старшем бите funcCode значит, что запрос отправлен повторно, 
		// по причине того, что master не получил ответ.
		// 
		// если этот запрос уже обрабатывался (что проверяется совпадением crc)
		// возвращается сохранённый ответ
		// 
		// если нет, фукция выполняется как обычно


		//игнорировать запрос, который относится к другому slave
		// 0   -	широкое вещание
		// 254 -	initialisationMode
		// 253 -  запрос по индивидуальному каналу
		if (!(slave == slaveID ||
			slave == 0 ||
			(slave != 254 && transceiver->isInitializationMode()) ||
			slave == 253))
			return;



		bool repeat = currentBuffer->get()[1] > 128;

		if(repeat)//crc считается для базового варианта
			currentBuffer->get()[1] -= 128;

		//игнорировать запрос по несовпадению контрольной суммы
		if (!currentBuffer->isModbusPacketValid())
		{
			return;
		}

		//повторение сохранённого ответа на предыщущий вопрос
		if (repeat && currentBuffer->getPacketCrc() == oldCrc)
		{
			swapBuffer();
			transmit();
			swapBuffer();
			return;
		}
		oldCrc = currentBuffer->getPacketCrc();
		++requestCounter;


		//недопустимый код функции
		if (func_code >= MODBUS_SLAVE_HANDLER_COUNT || handlers[func_code] == nullptr)
		{
			if (func_code > 127)
				func_code = 0;
			generateErrorResponce(currentBuffer, func_code, 1);
			transmit();
			swapBuffer();
			return;
		}

		handlers[func_code]->handle(currentBuffer);

		if (currentBuffer->getWriteEvent())
		{
			transmit();
			swapBuffer();
		}

	}
protected:
	//передать сообщение. Извне применять можно только для отложенных ответов
	uint16_t transmit()
	{
		getBuffer()->stop();
		if (getBuffer()->size() >= 254) //Ошибка длины пакета
			return 0;
		getBuffer()->data[0] = slaveID; //защита от дурака
		getBuffer()->setCrc();
		stage = Stage::transmition;
		uint16_t result = transceiver->transmit(getBuffer());
		if (result == 0)//если передача завершена мгновенно
			stage = Stage::reception;
		return result;
	}
	void swapBuffer()
	{
		currentBufferNumber = (uint8_t)!static_cast<bool>(currentBufferNumber);
	}
	//обработать запрос. Требуется заранее заполнить буффер
	
	void calculateExpectedPacketLength()
	{
		expectedPacketSize = 0; /////////////////////////////
	}

	void resetReception()
	{
		getBuffer()->clear();
		expectedPacketSize = 0;
	}
	void updateReception()
	{
		ModbusBuffer* currentBuffer = getBuffer();
		uint8_t oldSize = currentBuffer->size();
		
		// 0 если прием завершен
		// 1 если прием продолжается
		// 2 в случае ошибки приема (например переполнения принимающего буффера)
		uint8_t result = transceiver->receive(currentBuffer);
		if (result == 2)
		{
			resetReception();
			return;
		}
		

		if (result == 1)
		{
			if (currentBuffer->size() == oldSize)
				return;

			// 0 - если невозможно измерить или недостаточно данных для измерения
			// >=4 длина пакета
			if (expectedPacketSize == 0 && currentBuffer->size() > 3)
				calculateExpectedPacketLength();

			if (currentBuffer->size() < expectedPacketSize)
				return;
		}
		if ((currentBuffer->size() < 4) ||
			(expectedPacketSize > 1 && currentBuffer->size() != expectedPacketSize))
		{
			resetReception();
			return;
		}
		expectedPacketSize = 0;
		processRequest();
		getBuffer()->clear();

		expectedPacketSize = 0;
	}

	void updateTransmition()
	{
		uint8_t result = transceiver->isTransmitionOngoing();
		if (result == 0)
		{
			getBuffer()->clear();
			stage = Stage::reception;
		}
	}
};