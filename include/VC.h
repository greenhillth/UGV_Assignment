#pragma once

#using <System.dll>
#include "NetworkedModule.h"

ref class VehicleControl : public NetworkedModule
{
public:
	VehicleControl(SM_ThreadManagement^ SM_TM, SM_VC^ SM_VC_);

	error_state connect(String^ hostName, int portNumber) override;
	error_state communicate() override;
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();
private:

};