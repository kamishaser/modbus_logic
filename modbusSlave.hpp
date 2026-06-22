#pragma once
#include <functional>

#include <cstdint>
#include "modbusBuffer.hpp"

class ModbusSlave
{
	ModbusBuffer buffer;
	uint32_t requestCounter = 0;
	uint8_t slaveID;

	std::function<uint16_t(ModbusBuffer*)> transmitFunc;

#ifndef MODBUS_SLAVE_HANDLER_COUNT
#define MODBUS_SLAVE_HANDLER_COUNT 32
#endif

	//handlers - массив указателей на функции-обработчики modbus запросов
	//аргументы (ModbusBuffer* buffer)
	//возвращаемое значение - необходимость ответа
	//если вернёт 0 - ответ не будет отправлен
	std::function<void(ModbusBuffer*)> handlers[MODBUS_SLAVE_HANDLER_COUNT];
public:

	//transmitF указатель на функцию отправки ответа 
	//с аргументами (uint8_t* buffer, uint16_t length)
	//ответом является результат отправки, где 0 - успех
	ModbusSlave(std::function<uint8_t(ModbusBuffer*)> transmitF, uint8_t slaveid)
		:slaveID(slaveid), transmitFunc(transmitF), handlers{ nullptr }
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
		return &buffer;
	}

	//передать сообщение. Извне применять можно только для отложенных ответов
	uint16_t transmit()
	{
		buffer.stop();
		if (buffer.size() >= 254) //Ошибка длины пакета
			return 0;
		buffer.data[0] = slaveID; //защита от дурака
		buffer.setCrc();
		return transmitFunc(&buffer);
	}

	//аргументы (handler, func_code)
	//handler принимает (requestBuffer*, responseBuffer*, requestSize)
	//и возвращает длину ответа если вернёт 0 - ответ не отправится
	//func_code - код функции, которую обрабатывает. Обязательно < 64
	//если эта цифра не переопределено макросом MODBUS_SLAVE_HANDLER_COUNT
	//рекомендуется использовать лямбды с захватом
	void bindHandler(
		std::function<void(ModbusBuffer*)> hf, uint16_t func_code)
	{
		if (func_code < MODBUS_SLAVE_HANDLER_COUNT)
			handlers[func_code] = hf;
	}
	//шаблон
	//uint16_t handle(uint8_t* requestBuffer, uint8_t* responseBuffer, uint16_t requestSize)

	std::function<void(ModbusBuffer*)> getHandler(uint16_t func_code)
	{
		return handlers[func_code];
	}

	//обработать запрос. Требуется заранее заполнить буффер
	void processRequest()
	{
		if (buffer.size() < 4)
			return;

		buffer.start_read();
		uint8_t slave, func_code;
		buffer.read(slave);
		buffer.read(func_code);

		//игнорировать запрос
		if (slave != slaveID || !buffer.isModbusPacketValid())
			return;

		++requestCounter;

		//недопустимый код функции
		if (func_code >= MODBUS_SLAVE_HANDLER_COUNT || handlers[func_code] == nullptr)
		{
			if (func_code > 127)
				func_code = 0;
			generateErrorResponce(&buffer, func_code, 1);
			transmit();
			return;
		}

		handlers[func_code](&buffer);
		
		if (buffer.getWriteEvent())
			generateErrorResponce(&buffer, func_code, 4);
		transmit();
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

	void setTransmitFunc(std::function<uint8_t(ModbusBuffer*)> transmitF)
	{
		transmitFunc = transmitF;
	}
	uint32_t getRequestCount() const
	{
		return requestCounter;
	}
};