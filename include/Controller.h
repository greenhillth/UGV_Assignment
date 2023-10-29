#pragma once

#include "UGVModule.h"
#include "ControllerInterface.h"

ref class Controller : public UGVModule
{
public:
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
private:

};