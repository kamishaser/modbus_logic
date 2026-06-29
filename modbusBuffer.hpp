#pragma once
#include "string.h"
#include <cstdint>
#include <bit>

class ModbusBuffer
{
	uint8_t data[256];
	uint16_t packet_size = 0;
	uint16_t iterator = 0;
	bool writeMode = false;
	bool readMode = false;
	bool writeEvent = false;

	friend class ModbusSlave;
	friend class ModbusMaster;

	void set_size(uint16_t size)
	{
		packet_size = size;
	}
	uint16_t getPacketCrc()
	{
		return data[packet_size - 2] | 
			(static_cast<uint16_t>(data[packet_size - 1]) << 8);
	}
public:
	constexpr ModbusBuffer()
		:data()
	{
	}

	uint8_t* get()
	{
		return data;
	}
	
	
	bool is_read_mode()
	{
		return readMode;
	}
	bool is_write_mode()
	{
		return writeMode;
	}
	uint16_t size()
	{
		return packet_size;
	}
	uint16_t get_iterator()
	{
		return iterator;
	}
	void set_iterator(uint16_t pos)
	{
		iterator = pos;
	}

	//управление
	void setPacketSize(uint16_t size)
	{
		packet_size = size;
	}
	void clear()
	{
		stop();
		packet_size = 0;
		writeEvent = false;
	}

	//запустить режим чтения с 0
	void start_read()
	{
		stop();
		iterator = 0;
		readMode = true;
	}
	//запустить режим записи с 0
	void start_write()
	{
		stop();
		iterator = 0;
		writeMode = true;
	}
	void stop()
	{
		if (writeMode)
		{
			packet_size = iterator;
			writeEvent = true;
		}
		writeMode = false;
		readMode = false;
	}

	//записывает значение в буффер.
	//В случае успеха возвращает итератор. Иначе 0
	template<class T>
  uint16_t write(T value)
	{
		if (!writeMode || iterator + sizeof(T) > 254)
			return 0; // запись недоступна
		if constexpr (sizeof(T) == 8)
		{
			uint64_t buf = hton(*reinterpret_cast<uint64_t*>(&value));
			memcpy(data + iterator, reinterpret_cast<uint64_t*>(&buf), 8);
			iterator += 8;
		}
		else if constexpr (sizeof(T) == 4)
		{
			uint32_t buf = hton(*reinterpret_cast<uint32_t*>(&value));
			memcpy(data + iterator, reinterpret_cast<uint32_t*>(&buf), 4);
			iterator += 4;
		}
		else if constexpr (sizeof(T) == 2)
		{
			uint16_t buf = hton(*reinterpret_cast<uint16_t*>(&value));
			memcpy(data + iterator, reinterpret_cast<uint16_t*>(&buf), 2);
			iterator += 2;
		}
		else if constexpr (sizeof(T) == 1)
		{
			data[iterator] = *reinterpret_cast<uint8_t*>(&value);
			++iterator;
		}
		else
			return 0;
		return iterator;
	}

	uint16_t write(uint8_t* buf, uint16_t length)
	{
		if (!writeMode || iterator + length > 254)
			return 0;
		memcpy(data + iterator, buf, length);
		iterator += length;
		return iterator;
	}

	uint16_t read(uint8_t* buf, uint16_t length)
	{
		if (!readMode || iterator + length > packet_size)
			return 0;
		memcpy(buf, data + iterator, length);
		iterator += length;
		return iterator;
	}

	//читает значения из буффера
	//В случае успеха возвращает итератор. Иначе 0
	template<class T>
	uint16_t read(T& value)
	{
		if (!readMode || iterator + sizeof(T) > packet_size)
			return 0; // запись недоступна
		if constexpr (sizeof(T) == 8)
		{
			uint64_t buf;
			memcpy(reinterpret_cast<uint8_t*>(&buf), data + iterator,  8);
			buf = hton(buf);
			value = *reinterpret_cast<T*>(&buf);
			iterator += 8;
		}
		else if constexpr (sizeof(T) == 4)
		{
			uint32_t buf;
			memcpy(reinterpret_cast<uint8_t*>(&buf), data + iterator, 4);
			buf = hton(buf);
			value = *reinterpret_cast<T*>(&buf);
			iterator += 4;
		}
		else if constexpr (sizeof(T) == 2)
		{
			uint16_t buf;
			memcpy(reinterpret_cast<uint8_t*>(&buf), data + iterator, 2);
			buf = hton(buf);
			value = *reinterpret_cast<T*>(&buf);
			iterator += 2;
		}
		else if constexpr (sizeof(T) == 1)
		{
			value = *reinterpret_cast<T*>(&data[iterator]);
			++iterator;
		}
		else
			return 0;
		return iterator;
	}
	

	

	//установить контрольную сумму для отправки пакета
	void setCrc()
	{
		stop();
		uint16_t crc = calculateModbusCRC(data, iterator);
		packet_size += 2;
		data[iterator] = crc & 0xffu;
		data[iterator + 1] = crc >> 8;
	}
	protected:
	bool getWriteEvent()
	{
		if (writeEvent)
		{
			writeEvent = false;
			return true;
		}
		return false;
	}
	/*
 * Вычисляет CRC-16 для массива байт в формате Modbus.
 *
 * @param data Указатель на массив данных.
 * @param length Количество байт в массиве.
 * @return Вычисленное значение CRC-16.
 */
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
	 * @return true, если контрольная сумма верна, иначе false.
	 */
	bool isModbusPacketValid() const
	{
		if (packet_size < 3) return false; // Минимум: адрес (1) + функция (1) + CRC (2)

		// Вычисляем CRC для всех байт, кроме последних двух (самой контрольной суммы)
		uint16_t computedCRC = calculateModbusCRC(data, packet_size - 2);

		// Извлекаем CRC из пакета (в Modbus CRC передается как Low Byte, затем High Byte)
		uint16_t receivedCRC = 
			data[packet_size - 2] | (static_cast<uint16_t>(data[packet_size - 1]) << 8);

		return computedCRC == receivedCRC;
	}

	constexpr bool is_little_endian() {
		return std::endian::native == std::endian::little;
	}

	// Вспомогательная функция для реверса байтов
	template <typename T>
	constexpr T swap_endian(T val) {
		if constexpr (sizeof(T) == 2) {
			return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
		}
		else if constexpr (sizeof(T) == 4) {
			return ((val << 24) & 0xFF000000) |
				((val << 8) & 0x00FF0000) |
				((val >> 8) & 0x0000FF00) |
				((val >> 24) & 0x000000FF);
		}
		else if constexpr (sizeof(T) == 8) {
			return ((val << 56) & 0xFF00000000000000ULL) |
				((val << 40) & 0x00FF000000000000ULL) |
				((val << 24) & 0x0000FF0000000000ULL) |
				((val << 8) & 0x000000FF00000000ULL) |
				((val >> 8) & 0x00000000FF000000ULL) |
				((val >> 24) & 0x0000000000FF0000ULL) |
				((val >> 40) & 0x000000000000FF00ULL) |
				((val >> 56) & 0x00000000000000FFULL);
		}
		return val;
	}
	public:
	// Реализация hton (Host to Network)
	template <typename T>
	constexpr T hton(T val) {
		if constexpr (std::endian::native == std::endian::little) {
			return swap_endian(val);
		}
		return val;
	}

	// Реализация ntoh (Network to Host)
	// В случае с endianness, операция обратна hton, поэтому логика идентична
	template <typename T>
	constexpr T ntoh(T val) {
		return hton(val);
	}
};

//экземпляры шаблонов функций
template uint16_t ModbusBuffer::write(int8_t);
template uint16_t ModbusBuffer::write(int16_t);
template uint16_t ModbusBuffer::write(int32_t);
template uint16_t ModbusBuffer::write(int64_t);
template uint16_t ModbusBuffer::write(uint8_t);
template uint16_t ModbusBuffer::write(uint16_t);
template uint16_t ModbusBuffer::write(uint32_t);
template uint16_t ModbusBuffer::write(uint64_t);
template uint16_t ModbusBuffer::write(double);
template uint16_t ModbusBuffer::write(float);
template uint16_t ModbusBuffer::write(bool);

template uint16_t ModbusBuffer::read(int8_t&);
template uint16_t ModbusBuffer::read(int16_t&);
template uint16_t ModbusBuffer::read(int32_t&);
template uint16_t ModbusBuffer::read(int64_t&);
template uint16_t ModbusBuffer::read(uint8_t&);
template uint16_t ModbusBuffer::read(uint16_t&);
template uint16_t ModbusBuffer::read(uint32_t&);
template uint16_t ModbusBuffer::read(uint64_t&);
template uint16_t ModbusBuffer::read(double&);
template uint16_t ModbusBuffer::read(float&);
template uint16_t ModbusBuffer::read(bool&);