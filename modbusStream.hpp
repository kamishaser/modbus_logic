#pragma once
#include <cstdint>
#include "modbusBuffer.hpp"

class ModbusStreamInterface
{
public:
	virtual ~ModbusStreamInterface()
	{}
	virtual bool transmittionMode()
	{
		return false;
	}
	virtual bool receptionMode()
	{
		return false;
	}
	virtual void restartReception() = 0;
	virtual void restartTransmittion() = 0;

	//принять пакет данных. Возращает 0 или код ошибки
	virtual uint8_t receive(ModbusBuffer* buffer) = 0;

		//передать пакет данных. Возращает 0 или код ошибки
	virtual uint8_t transmit(ModbusBuffer * buffer) = 0;
};