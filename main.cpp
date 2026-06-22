#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include <cassert>

#include "modbusSlave.hpp"
#include "modbusSlaveCoilArray.hpp"
#include "modbusSlaveRegisterArray.hpp"

#include "modbusSlaveStreamArray.hpp"

#include "modbusSlaveDiscreteInputArray.hpp"
#include "modbusSlave16BitInputArray.hpp"

#include "modbusStringStream.hpp"


uint16_t transmit(ModbusBuffer* buffer)
{
	for (int i = 0; i < buffer->size(); ++i)
	{
		uint8_t b0 = (buffer->get()[i] >> 4 & 0xf);
		b0 += b0 > 9 ? 'A' - 10 : '0';
		uint8_t b1 = (buffer->get()[i] & 0xf);
		b1 += b1 > 9 ? 'A' - 10 : '0';
		std::cout << *(unsigned char*)&b0 << *(unsigned char*)&b1 << ' ';
	}
	std::cout << std::endl;
	return 0;
}

ModbusSlave modbus(transmit, 2);
ModbusSlaveCoilArray<64> coils(modbus);
ModbusSlaveRegisterArray<64> reg(modbus);

ModbusSlaveDiscreteInputArray<64> discretInputs(modbus);
ModbusSlave16BitInputArray<64> i16Input(modbus);

ModbusSlaveStreamArray<16> streams(modbus);




void readModbusPacket(ModbusBuffer* buffer) 
{
	std::string input;

	// Считываем всю строку целиком, чтобы обработать пробелы
	std::getline(std::cin, input);

	std::stringstream ss(input);
	std::string temp;

	buffer->start_write();
	// Итерируемся по всем словам в строке
	while (ss >> temp) {
		try {
			// Преобразуем строку в число, интерпретируя как 16-ричное
			unsigned int value;
			std::stringstream converter;
			converter << std::hex << temp;

			if (converter >> value) {
				assert(value < 256);
				buffer->write(static_cast<uint8_t>(value));
			}
		}
		catch (...) {
			// В случае ошибки парсинга пропускаем некорректное значение
			assert(false);
		}
	}
	buffer->stop();
}

void modbus_init()
{
	for (int i = 0; i < 64; ++i)
		reg[i] = i*2;
	for (int i = 0; i < 64; ++i)
		i16Input[i] = i*4;
	for (int i = 0; i < 64; ++i)
		discretInputs[i] = i & 1;


	while (true)
	{
		readModbusPacket(modbus.getBuffer());
		modbus.processRequest();
	}
}

int main()
{
	coils[1] = 1;
	coils[3] = 1;
	coils[4] = 1;
	coils[7] = 1;
	coils[9] = 1;
	coils[13] = 1;
	coils[17] = 1;
	//modbus_init();

	
	/*std::bitset<8> b;
	b[3] = 1;
	b[7] = 1;
	

	std::cout << std::endl;

	uint8_t b0 = ((uint8_t)b.to_ulong() >> 4 & 0xf);
	b0 += b0 > 9 ? 'A' - 10 : '0';
	uint8_t b1 = ((uint8_t)b.to_ulong() & 0xf);
	b1 += b1 > 9 ? 'A' - 10 : '0';
	std::cout << *(unsigned char*)&b0 << *(unsigned char*)&b1 << ' ';
	std::cout << b.to_ulong() << std::endl;*/
}