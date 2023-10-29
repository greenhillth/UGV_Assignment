#pragma once

#using <System.dll>
#include "NetworkedModule.h"

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

ref class Display : public NetworkedModule
{
public:
	void sendDisplayData(array<double>^ xData, array<double>^ yData, NetworkStream^ stream);
	error_state connect(String^ hostName, int portNumber) override;
	error_state communicate() override;
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;

private:

};