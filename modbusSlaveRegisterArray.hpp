#pragma once
#include "modbusSlave.hpp"
#include <array>

template<uint16_t arraySize>
class ModbusSlaveRegisterArray
{
public:
	std::array<uint16_t, arraySize> registers;

	const std::function<void(ModbusBuffer*)> writeOneRegisterFunctor = [this](
		ModbusBuffer* buffer)
		{this->writeOneRegister(buffer); };
	const std::function<void(ModbusBuffer*)> writeMultipleRegistersFunctor = [this](
		ModbusBuffer* buffer)
		{this->writeMultipleRegisters(buffer); };
	const std::function<void(ModbusBuffer*)> readMultipleRegistersFunctor = [this](
		ModbusBuffer* buffer)
		{this->readMultipleRegisters(buffer); };


	ModbusSlaveRegisterArray(ModbusSlave& slave)
	{
		slave.bindHandler(readMultipleRegistersFunctor, 3);
		slave.bindHandler(writeOneRegisterFunctor, 5);
		slave.bindHandler(writeMultipleRegistersFunctor, 0xf);
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

		if (address > registers.size())
		{ // ошибка адреса данных 
			ModbusSlave::generateErrorResponce(buffer, 5, 2);
			return;
		}

		registers[address] = value;

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

		if ((address + registersCount > registers.size()) || registersCount > 122)
		{ // ошибка адреса данных
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, 5, 2);
			return;
		}
		if (registersCount * 2 != byteCount)
		{ // ошибка адреса данных
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, 5, 2);
			return;
		}

		for (int i = address; i < address + registerCount; ++i)
		{
			uint16_t temp;
			buffer->read(temp);
			registers[i] = temp;
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

		if ((address + registersCount > registers.size()) || registersCount > 122)
		{ // ошибка адреса данных
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, 5, 2);
			return;
		}

		uint8_t byteCount = registersCount * 2;


		//ответ
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);

		buffer->write(byteCount);

		for (int i = address; i < address + registersCount; ++i)
			buffer->write<uint16_t>(registers[i]);

		buffer->stop();
	}
};