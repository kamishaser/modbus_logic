#pragma once
#include <cstdint>

class ModbusRequestGenerator;
class ModbusMaster;
class ModbusRequest
{
public:
	uint8_t* data;
	uint32_t channelID;
	uint16_t address;
	uint16_t count;

	uint8_t slave;
	uint8_t funcCode;

	ModbusRequest(const ModbusRequest&) = delete;
};
class ModbusResponce
{
};


class ModbusRequestGenerator
{
private:

};