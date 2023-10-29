#include "VC.h"

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
	throw gcnew System::NotImplementedException();
}
