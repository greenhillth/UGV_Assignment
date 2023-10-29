#using <System.dll>

#include "TMM.h"

error_state ThreadManagement::setupSharedMemory()
{
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_Laser_ = gcnew SM_Laser;
	SM_GNSS_ = gcnew SM_GNSS;
	SM_VehicleControl_ = gcnew SM_VehicleControl;

	return SUCCESS;
}

error_state ThreadManagement::processSharedMemory()
{
	return error_state();
}

void ThreadManagement::shutdownModules()
{
	SM_TM_->shutdown = 0xFF;
}

bool ThreadManagement::getShutdownFlag() { return SM_TM_->shutdown & bit_PM; }

void ThreadManagement::threadFunction()
{
	Console::WriteLine("TMT Thread is starting.");
	ThreadPropertiesList = gcnew array<ThreadProperties^> {
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_), &Laser::threadFunction), true, bit_LASER, "Laser Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_GNSS_), &GNSS::threadFunction), false, bit_GNSS, "GNSS Thread")
	};

	ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);		// Create list of threads

	StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);	// Create list of Stopwatches

	SM_TM_->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);// Make thread barriers

	for (int i = 0; i < ThreadPropertiesList->Length; i++)					// Start threads and init stopwatches
	{
		StopwatchList[i] = gcnew Stopwatch;
		ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
		ThreadList[i]->Start();
	}
	SM_TM_->ThreadBarrier->SignalAndWait();		// Wait at TMT Thread Barrier

	for (int i = 0; i < ThreadList->Length; i++)					// Start stopwatches
	{
		StopwatchList[i]->Start();
	}

	while (!Console::KeyAvailable && !getShutdownFlag())		// Check heartbeats
	{
		Console::WriteLine("TMT Thread is running");
		processHeartbeats();
		Thread::Sleep(50);
	}

	shutdownModules();

	for (int i = 0; i < ThreadPropertiesList->Length; i++)
	{
		ThreadList[i]->Join();
	}

	Console::WriteLine("TMT Thread terminating...");
}

error_state ThreadManagement::processHeartbeats()
{
	for (int i = 0; i < ThreadList->Length; i++)
	{
		// Check for high value of (i)th thread
		if (SM_TM_->heartbeat & ThreadPropertiesList[i]->BitID) {
			SM_TM_->heartbeat ^= ThreadPropertiesList[i]->BitID;
			StopwatchList[i]->Restart();	// Put flag down and reset stopwatch
		}
		else {
			if (StopwatchList[i]->ElapsedMilliseconds > CRASH_LIMIT_MS) {
				if (ThreadPropertiesList[i]->Critical) {
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + " failure. Shutting down all threads.");
					shutdownModules();
					return ERR_CRITICAL_THREAD_FAILURE;
				}
				else {
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + " failed. Attempting to restart.");
					ThreadList[i]->Abort();
					ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
					SM_TM_->ThreadBarrier = gcnew Barrier(1);
					ThreadList[i]->Start();

				}
			}
		}
	}
}
