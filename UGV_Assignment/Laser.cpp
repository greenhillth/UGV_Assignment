#include "Laser.h"

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser)
{
	SM_Laser_ = SM_Laser;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

error_state Laser::setupSharedMemory()
{
	return SUCCESS;
}

void Laser::threadFunction()
{
	Console::WriteLine("Laser thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("Laser thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS && checkData() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Laser thread is terminating");
}

error_state Laser::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_LASER) == 0)
	{
		SM_TM_->heartbeat |= bit_LASER;	// put laser flag up
		Watch->Restart();					// Restart stopwatch
	}
	else
	{
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT_MS) {
			shutdownThreads();
			return ERR_TMT_TIMEOUT;
		}
	}
	return SUCCESS;
}

void Laser::shutdownThreads()
{
    throw gcnew System::NotImplementedException();
}

bool Laser::getShutdownFlag()
{
    return SM_TM_->shutdown & bit_LASER;
}

error_state Laser::communicate()
{
    return error_state();
}

error_state Laser::checkData()
{
    return error_state();
}

error_state Laser::processSharedMemory()
{
    return error_state();
}

error_state Laser::connect(String^ hostName, int portNumber)
{
    return error_state();
}
