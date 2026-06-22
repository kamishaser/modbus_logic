#pragma once
#include "modbusSlave.hpp"
#include <array>

template<uint16_t arraySize, bool ref = false>
class ModbusSlave16BitInputArray : public std::array<uint16_t, arraySize>
{
public:

	const std::function<void(ModbusBuffer*)> readMultipleRegistersFunctor = [this](
		ModbusBuffer* buffer)
		{this->readMultipleRegisters(buffer); };


	ModbusSlave16BitInputArray(ModbusSlave& slave)
		:std::array<uint16_t, arraySize>(0)
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