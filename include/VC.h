#pragma once

#using <System.dll>
#include "NetworkedModule.h"

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

ref class VehicleControl : public NetworkedModule
{
public:
	error_state connect(String^ hostName, int portNumber) override;
	error_state communicate() override;
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;
private:

};