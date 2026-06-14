#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include <cassert>

#include "modbusSlave.hpp"
#include "modbusSlaveCoilArray.hpp"

#include "modbusSlaveStreamArray.hpp"

uint16_t transmit(uint8_t* buffer, uint16_t size)
{
	for (int i = 0; i < size; ++i)
	{
		uint8_t b0 = (buffer[i] >> 4 & 0xf);
		b0 += b0 > 9 ? 'A' - 10 : '0';
		uint8_t b1 = (buffer[i] & 0xf);
		b1 += b1 > 9 ? 'A' - 10 : '0';
		std::cout << *(unsigned char*)&b0 << *(unsigned char*)&b1 << ' ';
	}
	std::cout << std::endl;
	return 0;
}

ModbusSlave modbus(transmit, 2);
ModbusSlaveCoilArray<64> coils(modbus);

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
	while (true)
	{
		readModbusPacket(modbus.getBuffer());
		modbus.processRequest(1);
	}
}

int main()
{
	coils.coils[1] = 1;
	coils.coils[3] = 1;
	coils.coils[4] = 1;
	coils.coils[7] = 1;
	coils.coils[9] = 1;
	coils.coils[13] = 1;
	coils.coils[17] = 1;
	modbus_init();

	
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