#include "Controller.h"

constexpr int steeringScalar{-40};

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, SM_Display^ SM_DISPLAY)
	: UGVModule(SM_TM), SM_VC(SM_VC), SM_DISPLAY(SM_DISPLAY), input(false)
{
	connectedController = new ControllerInterface(1, 0);
	currentState = connectedController->GetState();
	SM_DISPLAY->Controller = connectedController;
}

Controller::~Controller()
{
	delete connectedController;
}

void Controller::updateVC(bool controlling) {
	if (!controlling) {
		SM_VC->Speed = 0;
		SM_VC->Steering = 0;
	}
	else {
		SM_VC->Speed = currentState.rightTrigger - currentState.leftTrigger;
		SM_VC->Steering = currentState.rightThumbX*steeringScalar;
	}
}

error_state Controller::processSharedMemory()
{
	bool connected = connectedController->IsConnected();
	Monitor::Enter(SM_DISPLAY->lockObject);
	if (connected) {
		SM_DISPLAY->connectionStatus[3]->Start();
		currentState = connectedController->GetState();
		SM_DISPLAY->Controller = connectedController;
	}
	else { 
		SM_DISPLAY->connectionStatus[3]->Reset();
	}
	Monitor::Exit(SM_DISPLAY->lockObject);

	Monitor::Enter(SM_VC->lockObject);
	updateVC(connected);
	Monitor::Exit(SM_VC->lockObject);

	return SUCCESS;
}

bool Controller::getShutdownFlag() { return SM_TM->shutdown & bit_CONTROLLER; }


error_state Controller::processHeartbeats()
{
	if ((SM_TM->heartbeat & bit_CONTROLLER) == 0)
	{
		SM_TM->heartbeat |= bit_CONTROLLER;	// put laser flag up
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
{
	SM_TM->shutdown = 0xFF;
}

void Controller::threadFunction()
{
	Console::WriteLine("Controller thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		processHeartbeats();
		processSharedMemory();
		if (currentState.buttonA) { shutdownThreads(); }
		Thread::Sleep(20);
	}
	Console::WriteLine("Controller thread is terminating");
}