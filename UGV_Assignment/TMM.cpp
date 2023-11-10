#using <System.dll>

#include "TMM.h"

ThreadManagement::ThreadManagement()
	: UGVModule(gcnew SM_ThreadManagement)
{
	SM_TM = gcnew SM_ThreadManagement;
	SM_LASER = gcnew SM_Laser;
	SM_GPS = gcnew SM_GNSS;
	SM_VC = gcnew SM_VehicleControl;
	SM_DISPLAY = gcnew SM_Display;
}

error_state ThreadManagement::setupSharedMemory()
{
	return SUCCESS;
}

error_state ThreadManagement::processSharedMemory()
{
	for (int i=1; i < ThreadList->Length; i++) {
		SM_DISPLAY->threadStatus[i] = ThreadList[i]->ThreadState;
	}

	SM_DISPLAY->sentCommand = SM_VC->formattedCMD;

	SM_DISPLAY->GPSData[0] = SM_GPS->Northing;
	SM_DISPLAY->GPSData[1] = SM_GPS->Easting;
	SM_DISPLAY->GPSData[2] = SM_GPS->Height;

	return SUCCESS;
}

void ThreadManagement::shutdownModules()
{
	SM_TM->shutdown = 0xFF;
}

bool ThreadManagement::getShutdownFlag() { return SM_TM->shutdown & bit_PM; }


error_state ThreadManagement::processHeartbeats()
{
	for (int i = 0; i < ThreadList->Length; i++)
	{
		// Check for high value of (i)th thread
		if (SM_TM->heartbeat & ThreadPropertiesList[i]->BitID) {
			SM_TM->heartbeat ^= ThreadPropertiesList[i]->BitID;
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
					SM_TM->ThreadBarrier = gcnew Barrier(1);
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
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM, SM_LASER, SM_DISPLAY), &Laser::threadFunction), true, bit_LASER, "Laser Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM, SM_GPS, SM_DISPLAY), &GNSS::threadFunction), false, bit_GNSS, "GNSS Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM, SM_VC, SM_DISPLAY), &Controller::threadFunction), true, bit_CONTROLLER, "Controller Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew VehicleControl(SM_TM, SM_VC, SM_DISPLAY), &VehicleControl::threadFunction), true, bit_VC, "Vehicle Control Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM, SM_LASER, SM_DISPLAY), &Display::threadFunction), true, bit_DISPLAY, "Display Thread")
	};

	ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);		// Create list of threads
	StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);	// Create list of Stopwatches

	SM_TM->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);// Make thread barriers

	for (int i = 0; i < ThreadPropertiesList->Length; i++)					// Start threads and init stopwatches
	{
		StopwatchList[i] = gcnew Stopwatch;
		ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
		ThreadList[i]->Start();
	}
	SM_TM->ThreadBarrier->SignalAndWait();		// Wait at TMT Thread Barrier

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