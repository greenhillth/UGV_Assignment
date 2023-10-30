#pragma once

#using <System.dll>
#include "NetworkedModule.h"

ref class Display : public NetworkedModule
{
public:
	Display(SM_ThreadManagement^ SM_TM);
	void sendDisplayData(array<double>^ xData, array<double>^ yData, NetworkStream^ stream);
	error_state connect(String^ hostName, int portNumber) override;
	error_state communicate() override;
	error_state processSharedMemory() override;
	error_state processHeartbeats();
	void shutdownThreads();
	bool getShutdownFlag() override;
	void threadFunction() override;

private:

};