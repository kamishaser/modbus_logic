#pragma once
#include <cstdint>
#include <bitset>

#include "modbusTransceiver.h"
#include "modbusInterface.h"


#ifndef MODBUS_SLAVE_COUNT
#define MODBUS_SLAVE_COUNT 32
#endif
class ModbusMaster
{
	ModbusTransceiverInterface* transceiver;
	ModbusInterface* interface;

	ModbusBuffer requestBuffer;
	ModbusBuffer responceBuffer;
	uint32_t requestStartTime = 0;

	// 0 - если невозможно измерить или недостаточно данных для измерения
		// >=4 длина пакета
	uint8_t expectedResponceSize = 0;

	struct SlaveStatus
	{
		enum
		{
			enable,
			error,
			supportRp128

		} flag;

		uint16_t timeout_millis = 254;
		std::bitset<3> status{"101"};
		
	} ;
	SlaveStatus slaveStatus[MODBUS_SLAVE_COUNT];


	
	
public:
	enum class Stage
	{
		waiting,
		transmition,
		reception,
		timeout,
	};

	Stage stage = Stage::waiting;
	uint8_t attempt = 0;
	uint8_t attemptCount = 3;

	
	constexpr ModbusMaster(ModbusInterface* Interface, ModbusTransceiverInterface* Transceiver)
		:transceiver(Transceiver), interface(Interface)
	{
	}

	void setSlave(uint8_t id, uint8_t timeout = 254, bool supportRp128 = false)
	{
		if (id >= MODBUS_SLAVE_COUNT)
			return;
		slaveStatus[id].timeout_millis = timeout;
		slaveStatus[id].status[SlaveStatus::supportRp128] = supportRp128;
		slaveStatus[id].status[SlaveStatus::enable] = true;
	}

	void update()
	{
		if (stage == Stage::timeout)
		{
			uint32_t delta = interface->time() - requestStartTime;
			uint8_t slaveId = requestBuffer.get()[0];
			if (slaveId > MODBUS_SLAVE_COUNT)
				slaveId = 0;
			if (delta < slaveStatus[slaveId].timeout_millis)//нужно оптимизировать
				return;
			
			nextAttempt();
		}

		if (stage == Stage::waiting)
		{
			interface->pull(&requestBuffer);
			if (requestBuffer.size() == 0)
				return;
			requestBuffer.setCrc();
			transmit();
		}

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
protected:
	void transmit()
	{
		requestStartTime = interface->time();
		transceiver->transmit(&requestBuffer);
		stage == Stage::transmition;
	}
	void abortReception()//оборвать приём, начать следующую попытку
	{
		responceBuffer.clear();
		stage = Stage::timeout;
	}

	void nextAttempt()//следующая попытка.
	{
		++attempt;
		
		if (attempt < attemptCount)
		{
			if (attempt == 1)//первый повтор
			{
				uint8_t slaveId = requestBuffer.get()[0];
				if (slaveId > MODBUS_SLAVE_COUNT)
					slaveId = 0;
				if (slaveStatus[slaveId].status[SlaveStatus::supportRp128])
					requestBuffer.get()[1] |= 1 << 7; //включение режима повтора
				//!! внимание, при включении режима повтора crc16 менять НЕ надо
			}
			transmit();
			return;
		}
		responceBuffer.clear();
		interface->push(&responceBuffer);//отправить пустой буффер как отсутсвие ответа
		stage = Stage::waiting;
	}


	void updateReception()
	{
		uint8_t oldSize = responceBuffer.size();
		// 0 если прием завершен
		// 1 если прием продолжается
		// 2 в случае ошибки приема (например переполнения принимающего буффера)
		uint8_t result = transceiver->receive(&responceBuffer);

		if (result == 1)
			return;
		if (result == 2 || responceBuffer.size() < 4)
		{
			abortReception();
			return;
		}
		processResponce();
		responceBuffer.clear();
	}
	void updateTransmition()
	{
		uint8_t result = transceiver->isTransmitionOngoing();
		if (result == 0)
		{
			stage = Stage::reception;
		}
	}
	void processResponce()
	{
		//повторить запрос при несовпадении контрольной суммы
		if (responceBuffer.size() == 0 ||  !responceBuffer.isModbusPacketValid())
		{
			nextAttempt();
			return;
		}
		interface->push(&responceBuffer);
	}

};