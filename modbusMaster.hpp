#pragma once
#include <cstdint>
#include <vector>
#include <queue>

#include "modbusTransceiver.h"



class ModbusMaster
{

public:
	std::vector<ModbusTransceiverInterface*> bus;
	ModbusMaster()
	{}



};