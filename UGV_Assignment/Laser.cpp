#include "Laser.h"

#define LASER_PORT 23000


//TODO - Add simulator address constructor functionality
Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser)
{
	SM_Laser_ = SM_Laser;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;

	Client = gcnew TcpClient;
	Stream = nullptr;



	ReadData = gcnew array<unsigned char>(128);
	SendData = gcnew array<unsigned char>(128);

	connect(WEEDER_ADDRESS, LASER_PORT);


}

error_state Laser::setupSharedMemory()
{

	return SUCCESS;
}

void Laser::threadFunction()
{
	Console::WriteLine("Laser thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start(); 
	while (!getShutdownFlag()) {
		Console::WriteLine("Laser thread is running");
		processHeartbeats();

		//if (communicate() == SUCCESS && checkData() == SUCCESS)		// Communicate and validate
		//{
			//processSharedMemory();									// Print memory as per spec
		//}
		Thread::Sleep(20);
	}
	Console::WriteLine("Laser thread is terminating");
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
    throw gcnew System::NotImplementedException();
}

bool Laser::getShutdownFlag() { return SM_TM_->shutdown & bit_LASER; }

//TODO - Implement communication
error_state Laser::communicate()
{
	//TODO - Error handling for empty send string
	if (SendData->Length <= 1) return ERR_NO_DATA;
	else {
		Console::WriteLine("Sending Command to laser: " + SendData->ToString());

		Stream->Write(SendData, 0, SendData->Length);
	}

	//TODO - Add timeout functionality
	System::Threading::Thread::Sleep(10);
	if (Stream->Read(ReadData, 0, ReadData->Length) == 0) { return ERR_NO_DATA_RECEIVED; }
	
	Console::WriteLine("Laser Response: " + ReadData->ToString());
	//TODO - clear array

	return SUCCESS;
}

error_state Laser::updateReadings()
{
	if (sendCommand("sRN LMDscandata") == SUCCESS) {
		//parse recieved info and save in sm_laser

	}

	return error_state();
}

error_state Laser::sendCommand(String^ command)
{
	command->Insert(0, " ");
	SendData = Encoding::ASCII->GetBytes(command);
	SendData[0] = 0x02;
	SendData[command->Length + 1] = 0x03;
	return communicate();
}

error_state Laser::checkData()
{
    return error_state();
}

error_state Laser::processSharedMemory()
{
    return error_state();
}

error_state Laser::connect(String^ hostName, int portNumber)
{
	String^ AuthString = gcnew String("5309693\n");
	
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;


	SendData = Encoding::ASCII->GetBytes(AuthString);
	communicate();
	String^ response = Encoding::ASCII->GetString(ReadData);
	if (response->Equals("OK\n")) { return SUCCESS;  }
	return ERR_CONNECTION;
}
