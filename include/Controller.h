#pragma once

#include "ControllerInterface.h"
#include "UGVModule.h"

ref class Controller : public UGVModule
{
public:
	Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, SM_Display^ SM_DISPLAY);
	~Controller();
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();

	void updateVC(bool controlling);
private:
	ControllerInterface* connectedController;
	controllerState currentState;
	bool input;

	SM_VehicleControl^ SM_VC;
	SM_Display^ SM_DISPLAY;		// To display controller info
};
//hello