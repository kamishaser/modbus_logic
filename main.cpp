#include <iostream>
#include <functional>

#include "stdio.h"

class ModbusSlave
{
	uint8_t responceBuffer[256];

	uint8_t slaveID;

	std::function<uint16_t(uint8_t*, uint16_t)> transmitFunc;

#ifndef MODBUS_SLAVE_HANDLER_COUNT
#define MODBUS_SLAVE_HANDLER_COUNT 64
#endif

	//handlers - массив указателей на функции-обработчики modbus запросов
	//аргументы (requestBuffer*, responseBuffer*, requestSize)
	//возвращаемое значение - длина ответа
	//если вернёт 0 - ответ не будет отправлен
	std::function<uint16_t(uint8_t*, uint8_t*, uint16_t)> handlers[MODBUS_SLAVE_HANDLER_COUNT];
public:

	//transmitF указатель на функцию отправки ответа 
	//с аргументами (uint8_t* buffer, uint16_t length)
	//ответом является результат отправки, где 0 - успех
	ModbusSlave(std::function<uint8_t(uint8_t*, uint16_t)> transmitF, uint8_t slaveid)
		:slaveID(slaveid), handlers{ nullptr }
	{

	}

	void setSlaveID(uint8_t id)
	{
		slaveID = id;
	}
	uint8_t getSlaveID() const
	{
		return slaveID;
	}

	//передать сообщение. Извне применять можно только для отложенных ответов
	uint16_t transmit(uint8_t* buffer, uint16_t size)
	{
		if (size >= 254) //Ошибка длины пакета
			return 0;
		
		uint16_t crc = calculateModbusCRC(buffer, size);

		buffer[size] = crc & 0xffu;
		buffer[size + 1] = crc >> 8;
		return transmitFunc(buffer, size + 2);
	}

	//аргументы (handler, func_code)
	//handler принимает (requestBuffer*, responseBuffer*, requestSize)
	//и возвращает длину ответа если вернёт 0 - ответ не отправится
	//func_code - код функции, которую обрабатывает. Обязательно < 64
	//если эта цифра не переопределено макросом MODBUS_SLAVE_HANDLER_COUNT
	//рекомендуется использовать лямбды с захватом
	void bindHandler(
		std::function<uint16_t(uint8_t*, uint8_t*, uint16_t)> hf, uint16_t func_code)
	{
		if (func_code < MODBUS_SLAVE_HANDLER_COUNT)
			handlers[func_code] = hf;
	}

	std::function<uint16_t(uint8_t*, uint8_t*, uint16_t)> getHandler(uint16_t func_code)
	{
		return handlers[func_code];
	}

	uint16_t processRequest(uint8_t* requestBuffer, uint16_t requestSize)
	{
		if (requestSize < 3 || requestBuffer[0] != slaveID ||
			!isModbusPacketValid(requestBuffer, requestSize))
		{
			return 0;
		}
		uint8_t func_code = requestBuffer[1];

		if (func_code >= MODBUS_SLAVE_HANDLER_COUNT || handlers[func_code] == nullptr)
		{
			return 0; //недопустимый номер
		}

		uint16_t responceSize = 
			handlers[func_code](requestBuffer, responceBuffer, requestSize);

		if (responceSize == 0)
			return 0;

		return transmit(responceBuffer, responceSize);
	}
	/**
 * Вычисляет CRC-16 для массива байт в формате Modbus.
 *
 * @param data Указатель на массив данных.
 * @param length Количество байт в массиве.
 * @return Вычисленное значение CRC-16.
 */

	static uint16_t generateErrorResponce(
		uint8_t* responceBuffer, uint8_t slaveID, uint8_t funcCode, uint8_t errorCode)
	{
		responceBuffer[0] = slaveID;
		responceBuffer[1] = funcCode | 0x80;
		responceBuffer[2] = errorCode;
		return 3;
	}

protected:
	static uint16_t calculateModbusCRC(const uint8_t* data, size_t length) {
		uint16_t crc = 0xFFFF;

		for (size_t i = 0; i < length; ++i) {
			crc ^= static_cast<uint16_t>(data[i]);

			for (int j = 0; j < 8; ++j) {
				if (crc & 0x0001) {
					crc >>= 1;
					crc ^= 0xA001;
				}
				else {
					crc >>= 1;
				}
			}
		}
		return crc;
	}

	/**
	 * Проверяет валидность пакета Modbus.
	 * Предполагается, что последние два байта в массиве — это CRC (в порядке Low Byte, High Byte).
	 *
	 * @param packet Массив байт, включая контрольную сумму.
	 * @param length Общая длина массива.
	 * @return true, если контрольная сумма верна, иначе false.
	 */
	static bool isModbusPacketValid(const uint8_t* packet, size_t length) {
		if (length < 3) return false; // Минимум: адрес (1) + функция (1) + CRC (2)

		// Вычисляем CRC для всех байт, кроме последних двух (самой контрольной суммы)
		uint16_t computedCRC = calculateModbusCRC(packet, length - 2);

		// Извлекаем CRC из пакета (в Modbus CRC передается как Low Byte, затем High Byte)
		uint16_t receivedCRC = packet[length - 2] | (static_cast<uint16_t>(packet[length - 1]) << 8);

		return computedCRC == receivedCRC;
	}
};

uint16_t transceive(uint8_t* buffer, uint16_t size)
{
	for (int i = 0; i < size; ++i)
	{
		std::cout << std::hex << buffer[i] << ' ';
	}
	std::cout << std::endl;
}
ModbusSlave modbus(transceive, 2);
void modbus_init()
{
}

int main()
{
	std::cout << "laja" << std::endl;
}