#pragma once

#include "modbusSlave.hpp"
#include <bitset>

template<uint16_t size>
class ModbusSlaveDiscreteInputArray: public std::bitset<size>
{
public:
	std::bitset<size> eventMode;
	const std::function<void(ModbusBuffer*)> readFunctor = [this](
		ModbusBuffer* buffer)
		{this->read(buffer); };


	ModbusSlaveDiscreteInputArray(ModbusSlave& slave)
	{
		slave.bindHandler(readFunctor, 2);
	}
protected:

	void read(ModbusBuffer* buffer)
	{
		//обработка запроса

		buffer->start_read();

		uint8_t slave, func_code;
		buffer->read(slave);
		buffer->read(func_code);

		uint16_t address, coilCount;
		buffer->read(address);
		buffer->read(coilCount);

		buffer->stop();

		if (buffer->size() != 8)
		{ // ошибка длины пакета
			ModbusSlave::generateErrorResponce(buffer, func_code, 9);
			return;
		}
		if (size < address + coilCount)
		{ // выход за предел массива
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}
		if (address + coilCount > size || coilCount == 0 || coilCount > 1024)
		{ // ошибка значения данных 
			return ModbusSlave::generateErrorResponce(buffer, 5, 2);
		}

		//генерация ответа
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);

		uint8_t byteCount = (coilCount & 0x7) ? coilCount / 8 + 1 : coilCount / 8;
		buffer->write(byteCount);

		uint8_t* buffer_begin = buffer->get() + buffer->get_iterator();

		uint16_t i1 = 0;
		uint16_t pos = address;
		uint8_t data;
		//целые байты
		for (; i1 < byteCount - 1u; ++i1)
		{
			data = 0;
			for (int16_t i2 = 0; i2 < 8; ++i2)
			{
				data |= (uint8_t)(*this)[pos + i2] << i2;
				(*this)[pos + i2] = (*this)[pos + i2] && !eventMode[pos + i2];
			}
			
			buffer->write<uint8_t>(data);
			
			pos += 8;
		}
		data = 0;
		//последний байт
		for (int16_t i2 = 0; i2 < (coilCount - 8 * (byteCount - 1)); ++i2)
		{
			data |= (uint8_t)(*this)[pos + i2] << i2;
			(*this)[pos + i2] = (*this)[pos + i2] && !eventMode[pos + i2];
		}
		buffer->write<uint8_t>(data);
		buffer->stop();
		return;
	}
};


