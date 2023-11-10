#include "Controller.h"

constexpr int steeringScalar{-40};

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VC^ SM_VC_, SM_Display^ SM_DISPLAY) : SM_VC_{SM_VC_}
{
	SM_TM_ = SM_TM;
	SM_DISPLAY_ = SM_DISPLAY;
	Watch = nullptr;

	connectedController = new ControllerInterface(1, 0);
	currentState = connectedController->GetState();
	input = false;
	SM_DISPLAY_->Controller = connectedController;
}

Controller::~Controller()
{
	delete connectedController;
}

void Controller::updateVC(bool controlling) {
	if (!controlling) {
		SM_VC_->Speed = 0;
		SM_VC_->Steering = 0;
	}
	else {
		SM_VC_->Speed = currentState.rightTrigger - currentState.leftTrigger;
		SM_VC_->Steering = currentState.rightThumbX*steeringScalar;
	}
}

error_state Controller::processSharedMemory()
{
	bool connected = connectedController->IsConnected();
	Monitor::Enter(SM_DISPLAY_->lockObject);
	if (connected) {
		SM_DISPLAY_->connectionStatus[3] = true;
		currentState = connectedController->GetState();
		SM_DISPLAY_->Controller = connectedController;
	}
	else { 
		SM_DISPLAY_->connectionStatus[3] = false;
	}
	Monitor::Exit(SM_DISPLAY_->lockObject);

	Monitor::Enter(SM_VC_->lockObject);
	updateVC(connected);
	Monitor::Exit(SM_VC_->lockObject);

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
		processHeartbeats();
		processSharedMemory();
		Thread::Sleep(20);
	}
	Console::WriteLine("Controller thread is terminating");
}