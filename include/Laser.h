#pragma once

#using <System.dll>
#include "NetworkedModule.h"

ref class Laser : public NetworkedModule
{
public:
	Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_Display^ SM_DISPLAY);
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();
	bool getShutdownFlag() override;

	error_state communicate() override;
	error_state processSharedMemory() override;

	error_state connect(String^ hostName, int portNumber) override;
	
	error_state sendCommand(String^ command);
	
	error_state connectionReattempt();

	~Laser() {};
private:
	array<unsigned char>^ SendData;
	SM_Laser^ SM_LASER;
};

