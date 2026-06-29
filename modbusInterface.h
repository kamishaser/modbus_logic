#pragma once
#include "modbusBuffer.hpp"

class ModbusInterface
{
public:
	virtual ~ModbusInterface() {}
	virtual void pull(ModbusBuffer*) {};
	virtual void push(ModbusBuffer*) {};
	virtual uint8_t time() { return 0; };
};