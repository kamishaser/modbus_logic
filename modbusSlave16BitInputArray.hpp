#pragma once
#include "modbusSlave.hpp"
#include <array>

template<uint16_t arraySize, bool ref = false>
class ModbusSlave16BitInputArray
{
public:
	std::array<uint16_t, arraySize> registers;

	const std::function<void(ModbusBuffer*)> readMultipleRegistersFunctor = [this](
		ModbusBuffer* buffer)
		{this->readMultipleRegisters(buffer); };


	ModbusSlaveRegisterArray(ModbusSlave& slave)
	{
		slave.bindHandler(readMultipleRegistersFunctor, 4);
	}
protected:
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

		if ((address + registersCount > registers.size()) || registersCount > 123)
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