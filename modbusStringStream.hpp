#pragma once
#include <sstream>
#include "modbusStream.hpp"

class ModbusStringStream : public std::stringstream, public ModbusStreamInterface
{
	std::streampos previousTransmittionPos = 0;
	bool rMode;
	bool tMode;
	bool rEndFlag;
	bool tEndFlag;
	
public:
	ModbusStringStream(bool receptionMode, bool transmittionMode)
		:std::stringstream(), ModbusStreamInterface(),
		rMode(receptionMode), tMode(transmittionMode)
	{}

	virtual bool receptionMode() override 
	{
		return rMode;
	}
	virtual bool transmittionMode() override
	{
		return tMode;
	}
	bool rEndEvent()
	{
		if (rEndFlag)
		{
			rEndFlag = false;
			return true;
		}
		return false;
	}
	bool tEndEvent()
	{
		if (tEndFlag)
		{
			tEndFlag = false;
			return true;
		}
		return false;
	}

	virtual uint8_t transmit(ModbusBuffer* buffer) override
	{
		uint8_t slave, func_code, streamID, repeat, byteCount;
		buffer->start_read();
		buffer->read(slave);
		buffer->read(func_code);
		buffer->read(streamID);
		buffer->read(repeat);//если не 0 - повторить предыдущий пакет
		buffer->stop();

		if (repeat)
		{
			byteCount = tellg() - previousTransmittionPos;
			seekg(previousTransmittionPos);
		}
		uint32_t available = rdbuf()->in_avail();
		if (available > 128)
			byteCount = 128;
		else if (available == 0)
		{
			byteCount = 0;
			tEndFlag = true;
		}
		else
			byteCount = available;
			
		buffer->start_write();
		buffer->write(slave);
		buffer->write(func_code);
		buffer->write(streamID);
		buffer->write(byteCount);
		if(byteCount > 0)
			write(reinterpret_cast<char*>(buffer->get() + buffer->get_iterator()), byteCount);
		buffer->stop();
	}
	
	virtual uint8_t receive(ModbusBuffer* buffer) override
	{
		uint8_t slave, func_code, streamID, byteCount;
		buffer->start_read();
		buffer->read(slave);
		buffer->read(func_code);
		buffer->read(streamID);
		buffer->read(byteCount);//если 0 - приём завершён
		//data
		if (byteCount == 0)
			rEndFlag = true;
		else
			read(reinterpret_cast<char*>(buffer->get() + buffer->get_iterator()), byteCount);
		buffer->stop();
		
		buffer->start_write();
		buffer->write(slave);
		buffer->write(func_code);
		buffer->write(streamID);
		buffer->write(byteCount);
		buffer->stop();
	}

	virtual void restartTransmittion() override
	{
		seekg(std::ios_base::beg);
	}

	virtual void restartReception() override
	{
		seekp(std::ios_base::beg);
	}
	
};