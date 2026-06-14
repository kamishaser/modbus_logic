#pragma once
#include <cstdint>

class ModbusMaster
{
	std::function<uint16_t(uint8_t*, uint16_t)> transmitFunc;
public:

	ModbusMaster(std::function<uint8_t(uint8_t*, uint16_t)> transmitF)
		:transmitFunc(transmitF)
	{}

	void handle_responce(uint8_t* buffer, uint16_t length)
	{
	}

};