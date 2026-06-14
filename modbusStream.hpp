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

	//принять пакет данных. Возращает 0 или код ошибки
	virtual uint8_t receive(ModbusBuffer* buffer);
	//{
	//	uint8_t slave, func_code, streamID, byteCount;
	//	buffer->start_read();
	//	buffer->read(slave);
	//	buffer->read(func_code);
	//	buffer->read(streamID);
	//	buffer->read(byteCount);//если 0 - приём завершён
	//	//data
	//	buffer->stop();
	//	
	//	buffer->start_write();
	//	buffer->write(slave);
	//	buffer->write(func_code);
	//	buffer->write(streamID);
	//	buffer->write(byteCount);
	//	buffer->stop();
	//}

	//передать пакет данных. Возращает 0 или код ошибки
	virtual uint8_t transmit(ModbusBuffer* buffer);
	//{
	//	uint8_t slave, func_code, streamID, repeat, byteCount;
	//	buffer->start_read();
	//	buffer->read(slave);
	//	buffer->read(func_code);
	//	buffer->read(streamID);
	//	buffer->read(repeat);//если не 0 - повторить предыдущий пакет
	//	buffer->stop();

	//	byteCount = 0;
	//	buffer->start_write();
	//	buffer->write(slave);
	//	buffer->write(func_code);
	//	buffer->write(streamID);
	//	buffer->write(byteCount);
	//	//data
	//	buffer->stop();
	//}
};