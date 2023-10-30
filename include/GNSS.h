#pragma once

#using <System.dll>
#include "NetworkedModule.h"


ref class GNSS : public NetworkedModule
{
public:
	GNSS(SM_ThreadManagement^ SM_TM, SM_GNSS^ SM_GNSS);
	error_state setupSharedMemory();
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads();
	bool getShutdownFlag() override;
	error_state communicate() override;
	error_state checkData();
	error_state processSharedMemory() override;
	error_state connect(String^ hostName, int portNumber) override;

	~GNSS() {};
};