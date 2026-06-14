#pragma once
#include "modbusSlave.hpp"
#include "modbusMaster.hpp"

//виртуальная шина modbus. Нужна преимущественно для отладки
class ModbusVirtual
{
	ModbusSlave* slaveList[248];
	ModbusMaster* master;
public:
	ModbusVirtual(ModbusMaster* Master)
		:slaveList(nullptr), master(Master)
	{}
	bool setSlave(ModbusSlave* slave)
	{
		if (slave->getSlaveID() > 248 || slaveList[slave->getSlaveID()] == nullptr)
			return false;
		slaveList[slave->getSlaveID()] = slave;
		slave->setTransmitFunc(slaveTransmitFunctor);
		return true;
	}
protected:
	uint8_t slave_Transmit(uint8_t* buffer, uint16_t length)
	{

	}
	uint8_t master_Transmit(uint8_t* buffer, uint16_t length)
	{
	}
	std::function<uint8_t(uint8_t*, uint16_t)> slaveTransmitFunctor;
};