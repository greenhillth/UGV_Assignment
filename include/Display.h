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

private:

};