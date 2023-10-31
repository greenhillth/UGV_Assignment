#include "VC.h"

VehicleControl::VehicleControl(SM_ThreadManagement^ SM_TM, SM_VC^ SM_VC_)
{
	SM_VC_ = SM_VC_;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

error_state VehicleControl::connect(String^ hostName, int portNumber)
{
	return error_state();
}

error_state VehicleControl::communicate()
{
	return error_state();
}

error_state VehicleControl::processSharedMemory()
{
	return error_state();
}

bool VehicleControl::getShutdownFlag() { return SM_TM_->shutdown & bit_VC;}

void VehicleControl::threadFunction()
{
	Console::WriteLine("Vehicle control thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		//Console::WriteLine("Vehicle control thread is running");
		processHeartbeats();
		Thread::Sleep(20);
	}
	Console::WriteLine("Vehicle control thread is terminating");
}

error_state VehicleControl::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_VC) == 0)
	{
		SM_TM_->heartbeat |= bit_VC;	// put VC flag up
		Watch->Restart();					// Restart stopwatch
	}
	else
	{
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT_MS) {

			shutdownThreads();
			return ERR_TMS_TIMEOUT;
		}
	}
	return SUCCESS;
}

void VehicleControl::shutdownThreads()
{
}
