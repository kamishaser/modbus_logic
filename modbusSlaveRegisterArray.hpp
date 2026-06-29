#pragma once
#include "modbusSlave.hpp"
#include <array>

template<uint16_t arraySize>
class ModbusSlaveRegisterArray: 
	public ModbusSlaveFunctionHandlerInterface, public std::array<uint16_t, arraySize>
{
public:

	ModbusSlaveRegisterArray(ModbusSlave& slave)
		:std::array<uint16_t, arraySize>(0)
	{
		slave.bindHandler(this, 3);
		slave.bindHandler(this, 6);
		slave.bindHandler(this, 0x10);
	}
	virtual void handle(ModbusBuffer* buffer) override
	{
		switch (buffer->get()[1])
		{
		case 3:
			readMultipleRegisters(buffer);
			break;
		case 6:
			writeOneRegister(buffer);
			break;
		case 16:
			writeMultipleRegisters(buffer);
			break;
		}
	}
protected:

	void writeOneRegister(ModbusBuffer* buffer)
	{
		//обработка запроса
		buffer->start_read();

		uint8_t slave, func_code;
		buffer->read(slave);
		buffer->read(func_code);

		uint16_t address, value;
		buffer->read(address);
		buffer->read(value);

		buffer->stop();

		if (buffer->size() != 8)
		{ // ошибка длины пакета
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, func_code, 9);
			return;
		}
		if (address > arraySize)
		{ // ошибка адреса данных 
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}

		(*this)[address] = value;

		//ответ
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);

		buffer->write(address);
		buffer->write(value);

		buffer->stop();
	}
	void writeMultipleRegisters(ModbusBuffer* buffer)
	{
		//обработка запроса
		buffer->start_read();

		uint8_t slave, func_code, byteCount;
		buffer->read(slave);
		buffer->read(func_code);

		uint16_t address, registersCount;
		buffer->read(address);
		buffer->read(registersCount);
		buffer->read(byteCount);

		if ((address + registersCount > arraySize) || registersCount > 122)
		{ // ошибка адреса данных
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}
		if (registersCount * 2 != byteCount || buffer->size() != 9 + byteCount)
		{ // ошибка адреса данных
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, func_code, 9);
			return;
		}

		for (int i = address; i < address + registersCount; ++i)
		{
			uint16_t temp;
			buffer->read(temp);
			(*this)[i] = temp;
		}
		buffer->stop();

		//ответ
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);

		buffer->write(address);
		buffer->write(registersCount);

		buffer->stop();
	}
	void readMultipleRegisters(ModbusBuffer* buffer)
	{
		//обработка запроса
		buffer->start_read();

		uint8_t slave, func_code;
		buffer->read(slave);
		buffer->read(func_code);

		uint16_t address, registersCount;
		buffer->read(address);
		buffer->read(registersCount);


		buffer->stop();

		if (buffer->size() != 8)
		{ // ошибка длины пакета
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, func_code, 9);
			return;
		}
		if ((address + registersCount > arraySize) || registersCount > 122)
		{ // ошибка адреса данных
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}

		uint8_t byteCount = registersCount * 2;


		//ответ
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);

		buffer->write(byteCount);

		for (int i = address; i < address + registersCount; ++i)
			buffer->write<uint16_t>((*this)[i]);

		buffer->stop();
	}
};