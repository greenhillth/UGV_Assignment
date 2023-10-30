#pragma once

#include "UGVModule.h"
//#include "ControllerInterface.h"

ref class Controller : public UGVModule
{
public:
	Controller(SM_ThreadManagement^ SM_TM);
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();
private:

};