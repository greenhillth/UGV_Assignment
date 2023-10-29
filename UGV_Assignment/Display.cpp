#include "Display.h"

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

bool Display::getShutdownFlag() { return SM_TM_->shutdown & bit_DISPLAY; }

void Display::threadFunction()
{
	throw gcnew System::NotImplementedException();
}
