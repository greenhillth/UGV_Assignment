#include "GNSS.h"

GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GNSS^ SM_GNSS)
{
	throw gcnew System::NotImplementedException();
}

error_state GNSS::setupSharedMemory()
{
	return error_state();
}

void GNSS::threadFunction()
{
	throw gcnew System::NotImplementedException();
}

error_state GNSS::processHeartbeats()
{
	return error_state();
}

void GNSS::shutdownThreads()
{
	throw gcnew System::NotImplementedException();
}

bool GNSS::getShutdownFlag()
{
	return false;
}

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
