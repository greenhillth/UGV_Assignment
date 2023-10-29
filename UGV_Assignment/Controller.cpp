#include "Controller.h"

error_state Controller::processSharedMemory()
{
	return error_state();
}

bool Controller::getShutdownFlag() { return SM_TM_->shutdown & bit_CONTROLLER; }

void Controller::threadFunction()
{
	throw gcnew System::NotImplementedException();
}
