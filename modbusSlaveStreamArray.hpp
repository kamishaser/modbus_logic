#pragma once
#include "modbusStream.hpp"
#include "modbusSlave.hpp"

//30 read stream
//   request
// 1b - slaveID
// 1b - funcCode
// 1b - streamID
//   responce
// 1b - slaveID
// 1b - funcCode
// 1b - streamID
// 1b - byteCount (0-250)
// 0-250b data 
//
// ќпрос буфера должен вестить до получени€ ответа длиной 0 байт,
// который подтверждает, что приЄм окончен
// если byteCount == 0 добавл€ютс€ 2 байта дополнительной crc16 проверки
// или 0000 при еЄ отсутствии



//31 write stream
//   request
// 1b - slaveID
// 1b - funcCode
// 1b - streamID
// 1b - byteCount (0-248)
// 0-248b data
//   responce
// 1b - slaveID
// 1b - funcCode
// 1b - streamID
// 1b - byteCount
//
// ѕосле окончани€ записи необходимо отправить пакет длиной 0b
// дл€ подтверждени€ окончани€ передачи
// если byteCount == 0 добавл€ютс€ 2 байта дополнительной crc16 проверки
// или 0000 при еЄ отсутствии

// если запись/чтение	не удались - ошибка 4
// если провалилась дополнительна€ сrc16 проверка - ошибка 8 и рестарт

template<uint8_t arraySize>
class ModbusSlaveStreamArray: public ModbusSlaveHandlerInterface
{
	std::array<ModbusStreamInterface*, arraySize> streams;
	
public:



	ModbusSlaveStreamArray(ModbusSlave& slave)
		:streams{ nullptr }
	{
		slave.bindHandler(this, 30);
		slave.bindHandler(this, 31);
	}

	virtual void handle(ModbusBuffer* buffer) override
	{
		switch (buffer->get()[1])
		{
		case 30:
			readStream(buffer);
			break;
		case 31:
			writeStream(buffer);
			break;
		}
	}

 	void readStream(ModbusBuffer* buffer)
	{//обработка запроса
		buffer->start_read();

		uint8_t slave, func_code, streamID, repeat;
		buffer->read(slave);
		buffer->read(func_code);

		buffer->read(streamID);

		buffer->stop();
		if (streams[streamID] == nullptr || !streams[streamID]->transmittionMode())
		{
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}
		streams[streamID]->receive(buffer);
	}
	void writeStream(ModbusBuffer* buffer)
	{
		uint8_t slave, func_code, streamID, byteCount;
		buffer->read(slave);
		buffer->read(func_code);

		buffer->read(streamID);
		buffer->read(byteCount);

		buffer->stop();
		if (streams[streamID] == nullptr || !streams[streamID]->receptionMode())
		{
			ModbusSlave::generateErrorResponce(buffer, func_code, 2);
			return;
		}
		streams[streamID]->transmit(buffer);
	}

	void bindStream(ModbusStreamInterface* stream, uint16_t id)
	{
		if (id > streams.size())
			return;
		streams[id] = stream;
	}
};