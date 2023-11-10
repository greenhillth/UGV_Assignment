#pragma once

#include "ControllerInterface.h"
#include "UGVModule.h"

ref class Controller : public UGVModule
{
public:
	Controller(SM_ThreadManagement^ SM_TM, SM_VC^ SM_VC_, SM_Display^ SM_DISPLAY);
	~Controller();
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();
private:
	ControllerInterface* connectedController;
	controllerState currentState;
	bool input;

	SM_VC^ SM_VC_;
	SM_Display^ SM_DISPLAY_;		// To display controller info
};
//hello