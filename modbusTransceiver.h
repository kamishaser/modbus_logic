#pragma once
#include "modbusBuffer.hpp"

class ModbusTransceiverInterface
{
public:
	virtual ~ModbusTransceiverInterface() {}

	//Считать данные с буффера порта в буффер slave
	//Возращает 
	// 0 если прием завершен
	// 1 если прием продолжается
	// 2 в случае ошибки приема (например переполнения принимающего буффера)
	// 
	virtual uint8_t receive(ModbusBuffer*) { return 2; }

	//начать передачу данных. Возвращает 0 в случае успеха
	//1 в случае провала (например если истек timeout)
	virtual uint8_t transmit(ModbusBuffer*) { return 1; }

	//обновляет статус передачи. Если передача не ведётся или завершена, возвращает 0.
	virtual uint8_t isTransmitionOngoing() { return 1; }

	//режим инициализации подлючения. (процесс присвоения slaveID)
	virtual uint8_t isInitializationMode() { return 0; }
};