#using <System.dll>

#include "TMM.h"

error_state ThreadManagement::setupSharedMemory()
{
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_Laser_ = gcnew SM_Laser;
	SM_GNSS_ = gcnew SM_GNSS;
	SM_VC_ = gcnew SM_VC;
	SM_DISPLAY_ = gcnew SM_Display;

	return SUCCESS;
}

error_state ThreadManagement::processSharedMemory()
{
	for (int i=1; i < ThreadList->Length; i++) {
		SM_DISPLAY_->threadStatus[i] = ThreadList[i]->ThreadState;
	}

	SM_DISPLAY_->sentCommand = SM_VC_->formattedCMD;

	SM_DISPLAY_->GPSData[0] = SM_GNSS_->Northing;
	SM_DISPLAY_->GPSData[1] = SM_GNSS_->Easting;
	SM_DISPLAY_->GPSData[2] = SM_GNSS_->Height;

	return SUCCESS;
}

void ThreadManagement::shutdownModules()
{
	SM_TM_->shutdown = 0xFF;
}

bool ThreadManagement::getShutdownFlag() { return SM_TM_->shutdown & bit_PM; }


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
	return SUCCESS;
}

//THREAD FUNCTION
void ThreadManagement::threadFunction()
{
	Console::WriteLine("TMT Thread is starting.");
	ThreadPropertiesList = gcnew array<ThreadProperties^> {
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_, SM_DISPLAY_), &Laser::threadFunction), true, bit_LASER, "Laser Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_GNSS_, SM_DISPLAY_), &GNSS::threadFunction), false, bit_GNSS, "GNSS Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_VC_, SM_DISPLAY_), &Controller::threadFunction), true, bit_CONTROLLER, "Controller Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew VehicleControl(SM_TM_, SM_VC_, SM_DISPLAY_), &VehicleControl::threadFunction), true, bit_VC, "Vehicle Control Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM_, SM_Laser_, SM_DISPLAY_), &Display::threadFunction), true, bit_DISPLAY, "Display Thread")
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

	bool exitFlag = false;

	while (!getShutdownFlag() && !exitFlag)		// Check heartbeats
	{
		//Console::WriteLine("TMT Thread is running");
		processHeartbeats();
		processSharedMemory();
		Thread::Sleep(50);
		if (Console::KeyAvailable) {
			exitFlag = (Console::ReadKey(true).Key == ConsoleKey::Q);
		}
	}

	shutdownModules();

	for (int i = 0; i < ThreadPropertiesList->Length; i++)
	{
		ThreadList[i]->Join();
	}

	Console::WriteLine("TMT Thread terminating...");
}