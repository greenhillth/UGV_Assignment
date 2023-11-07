#include "Laser.h"

#define LASER_PORT  23000

//TODO - Add simulator address constructor functionality
Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser)
{
	SM_Laser_ = SM_Laser;
	SM_TM_ = SM_TM;
	Watch = nullptr;

	Client = nullptr;
	Stream = nullptr;

	TcpPort = LASER_PORT;
	DNS = gcnew String(WEEDER_ADDRESS);

	ReadData = gcnew array<unsigned char>(2048);
	SendData = gcnew array<unsigned char>(128);
}

error_state Laser::setupSharedMemory()
{
	return SUCCESS;
}

error_state Laser::processSharedMemory()
{
	int numPoints;
	array<String^>^ Frags;
	if (ReadData->Length < 1544) { return ERR_NO_DATA; }
	String^ Resp = Encoding::ASCII->GetString(ReadData);
	Frags = Resp->Split(' ');

	numPoints = Convert::ToInt32(Frags[25], 16);
	Monitor::Enter(SM_Laser_->lockObject);
	for (int i = 0; i < numPoints; i++)
	{
		SM_Laser_->x[i] = Convert::ToInt32(Frags[26 + i], 16) * Math::Sin(i * 0.5 * Math::PI / 180);
		SM_Laser_->y[i] = -1*Convert::ToInt32(Frags[26 + i], 16) * Math::Cos(i * 0.5 * Math::PI / 180);
	}
	Monitor::Exit(SM_Laser_->lockObject);
	if (Frags->Length == 400)
	{
		SM_Laser_->valid = true;
		return error_state::SUCCESS;
	}
	else
	{
		SM_Laser_->valid = false;
		return error_state::ERR_INVALID_DATA;
	}
}

error_state Laser::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_LASER) == 0)
	{
		SM_TM_->heartbeat |= bit_LASER;	// put laser flag up
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

void Laser::shutdownThreads()
{
	Client->Close();
	Console::WriteLine("Laser thread is terminating");
}

bool Laser::getShutdownFlag() { return SM_TM_->shutdown & bit_LASER; }


// Networking Functions

error_state Laser::connect(String^ hostName, int portNumber)
{
	String^ AuthString = gcnew String("5309693\n");
	
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();

	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 2048;
	Client->SendBufferSize = 128;

	SendData = Encoding::ASCII->GetBytes(AuthString);
	
	communicate();

	String^ response = Encoding::ASCII->GetString(ReadData);
	
	if (response->Contains("OK\n")) {
		return SUCCESS;
	}
	return ERR_CONNECTION;
}

//TODO - Implement communication
error_state Laser::communicate()
{
	if (SendData->Length == 0) return ERR_NO_DATA;
	else if (SendData[0] == 0x02) {
		Stream->Write(SendData, 0, 1);
		Stream->Write(SendData, 1, SendData->Length - 2);
		Stream->Write(SendData, SendData->Length - 1, 1);
	}
	else {
		Stream->Write(SendData, 0, SendData->Length);
	}

	//TIMEOUT
	for (int i = 0; !Stream->DataAvailable && (i < 5); i++) {
		System::Threading::Thread::Sleep(10);
	}
	
	if (Stream->DataAvailable) {
		Stream->Read(ReadData, 0, ReadData->Length);
		return SUCCESS;
	}
	return ERR_NO_DATA_RECEIVED;
}

error_state Laser::sendCommand(String^ command)
{
	Array::Resize(SendData,command->Length + 2);

	Encoding::ASCII->GetBytes(command, 0, command->Length, SendData, 1);
	SendData[0] = 0x02;
	SendData[command->Length+1] = 0x03;
	communicate();

	return SUCCESS;
}

//THREAD FUNCTION
void Laser::threadFunction()
{
	Console::WriteLine("Laser thread is starting");
	Watch = gcnew Stopwatch;
	connect(WEEDER_ADDRESS, LASER_PORT);

	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start(); 
	while (!getShutdownFlag()) {
		processHeartbeats();
		sendCommand("sRN LMDscandata");
		processSharedMemory();

		if (SM_Laser_->valid) {}

		//if (communicate() == SUCCESS && checkData() == SUCCESS)		// Communicate and validate
		//{
			//processSharedMemory();									// Print memory as per spec
		//}
		Thread::Sleep(20);
	}
	shutdownThreads();
}