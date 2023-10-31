#include "GNSS.h"

GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GNSS^ SM_GNSS)
{
	SM_GNSS_ = SM_GNSS;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

error_state GNSS::setupSharedMemory()
{
	return SUCCESS;
}

void GNSS::threadFunction()
{
	Console::WriteLine("GNSS thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		//Console::WriteLine("GNSS thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS && checkData() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("GNSS thread is terminating");
}

error_state GNSS::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_GNSS) == 0)
	{
		SM_TM_->heartbeat |= bit_GNSS;	// put gnss flag up
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

void GNSS::shutdownThreads()
{
}

bool GNSS::getShutdownFlag() { return SM_TM_->shutdown & bit_GNSS; }

error_state GNSS::communicate()
{
	return error_state();
}

error_state GNSS::checkData()
{
	return error_state();
}

error_state GNSS::processSharedMemory()
{
	return error_state();
}

error_state GNSS::connect(String^ hostName, int portNumber)
{
	return error_state();
}
