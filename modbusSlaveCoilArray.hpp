#pragma once
#include "modbusSlave.hpp"
#include <bitset>

template<uint16_t coilNumber>
class ModbusSlaveCoilArray
{
public:
	std::bitset<coilNumber> coils;

	const std::function<void(ModbusBuffer*)> writeOneCoilFunctor = [this](
		ModbusBuffer* buffer)
		{this->writeOneCoil(buffer); };
	const std::function<void(ModbusBuffer*)> writeMultipleCoilsFunctor = [this](
		ModbusBuffer* buffer)
		{this->writeMultipleCoils(buffer); };
	const std::function<void(ModbusBuffer*)> readMultipleCoilsFunctor = [this](
		ModbusBuffer* buffer)
		{this->readMultipleCoils(buffer); };


	ModbusSlaveCoilArray(ModbusSlave& slave)
	{
		slave.bindHandler(readMultipleCoilsFunctor, 1);
		slave.bindHandler(writeOneCoilFunctor, 5);
		slave.bindHandler(writeMultipleCoilsFunctor, 0xf);
	}
protected:

	void writeOneCoil(ModbusBuffer* buffer)
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
		

		if (address > coils.size())
		{ // ошибка адреса данных 
			ModbusSlave::generateErrorResponce(buffer, 5, 2);
			return;
		}
		if (value == 0xFF00)
			coils[address] = 1;
		else if (value == 0x0000)
			coils[address] = 0;
		else
		{ // ошибка значения данных 
			return ModbusSlave::generateErrorResponce(buffer, 5, 3);
		}

		//генерация ответа
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);

		buffer->write(address);
		buffer->write(value);
		buffer->stop();
	}

	void writeMultipleCoils(ModbusBuffer* buffer)
	{
		//обработка запроса
		buffer->start_read();

		uint8_t slave, func_code, byteCount;
		buffer->read(slave);
		buffer->read(func_code);

		uint16_t address, coilCount;
		buffer->read(address);
		buffer->read(coilCount);

		buffer->read(byteCount);

		uint16_t i1 = 0;
		uint16_t pos = address;
		uint8_t data;

		//переписываем byteCount ввиду ошибки используемой master программы
		byteCount = (coilCount & 0x7) ? coilCount / 8 + 1 : coilCount / 8;

		if (coils.size() < address + coilCount)
		{
			buffer->stop();
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}

		for (; i1 < byteCount - 1; ++i1)
		{
			buffer->read<uint8_t>(data);
			coils[pos + 0] = data        & 1;
			coils[pos + 1] = (data >> 1) & 1;
			coils[pos + 2] = (data >> 2) & 1;
			coils[pos + 3] = (data >> 3) & 1;
			coils[pos + 4] = (data >> 4) & 1;
			coils[pos + 5] = (data >> 5) & 1;
			coils[pos + 6] = (data >> 6) & 1;
			coils[pos + 7] = (data >> 7) & 1;
			pos += 8;
		}

		//последний байт
		buffer->read<uint8_t>(data);
		for (int16_t i2 = 0; i2 < (coilCount - 8 * (byteCount - 1)); ++i2)
			coils[pos + i2] = (data >> i2) & 1;
		buffer->stop();

		//формирование ответа
		buffer->start_write();
		buffer->write<uint8_t>(slave);
		buffer->write<uint8_t>(func_code);
		buffer->write<uint16_t>(address);
		buffer->write<uint8_t>(byteCount);
		buffer->stop();
	}

	void readMultipleCoils(ModbusBuffer* buffer)
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
		
		if (address + coilCount > coils.size() || coilCount == 0 || coilCount > 1024)
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
			data =             coils[pos + 7];
			data = data << 1 | coils[pos + 6];
			data = data << 1 | coils[pos + 5];
			data = data << 1 | coils[pos + 4];
			data = data << 1 | coils[pos + 3];
			data = data << 1 | coils[pos + 2];
			data = data << 1 | coils[pos + 1];
			data = data << 1 | coils[pos + 0];
			buffer->write<uint8_t>(data);
			pos += 8;
		}
		data = 0;
		//последний байт
		for (int16_t i2 = 0; i2 < (coilCount - 8*(byteCount-1)); ++i2)
			data |= (uint8_t)coils[pos + i2] << i2;
		buffer->write<uint8_t>(data);
		buffer->stop();
		return;

		/* старая версия
		uint8_t byteCount = (coilCount & 0x7) ? coilCount / 8 + 1 : coilCount / 8;
		buffer->write(byteCount);


		uint8_t* buffer_begin = buffer->get() + buffer->get_iterator();
		for (int i1 = 0; i1 < byteCount; ++i1)
		{
			buffer->write<uint8_t>(0);
			std::bitset<8> bitset;
			for (int i2 = 0; i2 < 8; ++i2)
			{
				if (coilCount <= i1 * 8 + i2)
					break;
				bitset[i2] = coils[i1 * 8 + i2 + address];

			}
			buffer_begin[i1] = (uint8_t)bitset.to_ulong();
		}
		buffer->stop();*/
	}
};


