#include "Controller.h"

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VC^ SM_VC_)
{
	SM_TM_ = SM_TM;
	SM_VC_ = SM_VC_;
	Watch = nullptr;

	connectedController = new ControllerInterface();
	currentState = connectedController->GetState();
	input = false;
}

Controller::~Controller()
{
	delete connectedController;
}

error_state Controller::processSharedMemory()
{
	currentState = connectedController->GetState();
	return SUCCESS;
}

bool Controller::getShutdownFlag() { return SM_TM_->shutdown & bit_CONTROLLER; }


error_state Controller::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0)
	{
		SM_TM_->heartbeat |= bit_CONTROLLER;	// put laser flag up
		Watch->Restart();						// Restart stopwatch
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

void Controller::shutdownThreads()
{}

void Controller::threadFunction()
{
	Console::WriteLine("Controller thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		//Console::WriteLine("Controller thread is running");
		processHeartbeats();
		
		if (connectedController->IsConnected()) {
			connectedController->printControllerState(currentState);

		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Controller thread is terminating");
}