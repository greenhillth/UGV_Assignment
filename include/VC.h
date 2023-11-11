#pragma once

#using <System.dll>
#include "NetworkedModule.h"

ref class VehicleControl : public NetworkedModule
{
public:
	VehicleControl(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, SM_Display^ SM_DISPLAY);

	error_state connect(String^ hostName, int portNumber) override;
	error_state connectionReattempt();
	error_state communicate() override;
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();

	void commandStr(double steer, double speed);

private:
	array<unsigned char>^ SendData;
	SM_VehicleControl^ SM_VC;
	String^ command;
	bool flag;
};
