#include "Display.h"

Display::Display(SM_ThreadManagement^ SM_TM)
{
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

void Display::sendDisplayData(array<double>^ xData, array<double>^ yData, NetworkStream^ stream) {
	// Serialize the data arrays to a byte array
	//(format required for sending)
	array<Byte>^ dataX =
		gcnew array<Byte>(SM_Laser_->x->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->x, 0, dataX, 0, dataX->Length);
	array<Byte>^ dataY =
		gcnew array<Byte>(SM_Laser_->y->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->y, 0, dataY, 0, dataY->Length);
	// Send byte data over connection
	Stream->Write(dataX, 0, dataX->Length);
	Thread::Sleep(10);
	Stream->Write(dataY, 0, dataY->Length);
}

error_state Display::connect(String^ hostName, int portNumber)
{
	return error_state();
}

error_state Display::communicate()
{
	return error_state();
}

error_state Display::processSharedMemory()
{
	return error_state();
}

error_state Display::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_DISPLAY) == 0)
	{
		SM_TM_->heartbeat |= bit_DISPLAY;	// put laser flag up
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

void Display::shutdownThreads()
{
	throw gcnew System::NotImplementedException();
}

bool Display::getShutdownFlag() { return SM_TM_->shutdown & bit_DISPLAY; }

void Display::threadFunction()
{
	Console::WriteLine("Display thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("Display thread is running");
		processHeartbeats();
		Thread::Sleep(20);
	}
	Console::WriteLine("Display thread is terminating");
}
