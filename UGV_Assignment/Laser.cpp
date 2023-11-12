#include "Laser.h"

#define LASER_PORT  23000

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_LASER, SM_Display^ SM_DISPLAY)
	: NetworkedModule(SM_TM, SM_DISPLAY, gcnew String(WEEDER_ADDRESS), LASER_PORT), SM_LASER(SM_LASER)
{
	ReadData = gcnew array<unsigned char>(2048);
	SendData = gcnew array<unsigned char>(128);
	SendData = Encoding::ASCII->GetBytes("5309693\n");
}

error_state Laser::processSharedMemory()
{
	int numPoints;
	array<String^>^ Frags;
	if (ReadData->Length < 1544) { return ERR_NO_DATA; }
	String^ Resp = Encoding::ASCII->GetString(ReadData);
	Frags = Resp->Split(' ');

	numPoints = Convert::ToInt32(Frags[25], 16);
	Monitor::Enter(SM_LASER->lockObject);
	for (int i = 0; i < numPoints; i++)
	{
		SM_LASER->x[i] = Convert::ToInt32(Frags[26 + i], 16) * Math::Sin(i * 0.5 * Math::PI / 180);
		SM_LASER->y[i] = -1*Convert::ToInt32(Frags[26 + i], 16) * Math::Cos(i * 0.5 * Math::PI / 180);
	}
	Monitor::Exit(SM_LASER->lockObject);
	if (Frags->Length == 400)
	{
		SM_LASER->valid = true;
		return error_state::SUCCESS;
	}
	else
	{
		SM_LASER->valid = false;
		return error_state::ERR_INVALID_DATA;
	}
}

error_state Laser::processHeartbeats()
{
	if ((SM_TM->heartbeat & bit_LASER) == 0)
	{
		SM_TM->heartbeat |= bit_LASER;	// put laser flag up
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

void Laser::shutdownThreads() { SM_TM->shutdown = 0xFF; }


bool Laser::getShutdownFlag() { return SM_TM->shutdown & bit_LASER; }


// Networking Functions

error_state Laser::connect(String^ hostName, int portNumber)
{
	try
	{
		Client = gcnew TcpClient(hostName, portNumber);
	}
	catch (System::Net::Sockets::SocketException^ e)
	{
		timeout->Start();
		return ERR_CONNECTION;
	}

	Stream = Client->GetStream();

	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 2048;
	Client->SendBufferSize = 128;
	
	communicate();

	String^ response = Encoding::ASCII->GetString(ReadData);
	
	if (response->Contains("OK\n")) {
		return SUCCESS;
	}
	return ERR_VALIDATION;
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
	if (!Stream->DataAvailable) { return ERR_NO_DATA; }

	Stream->Read(ReadData, 0, ReadData->Length);
	return SUCCESS;
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

error_state Laser::connectionReattempt() {

	if (connectionAttempts >= 5) {
		return CONNECTION_TIMEOUT;
	}
	else if (timeout->Elapsed.Seconds > 10) {
		timeout->Reset();
		connectionAttempts++;
		return connect(DNS, PORT);
	}
	return ERR_CONNECTION;
}

//THREAD FUNCTION
void Laser::threadFunction()
{
	Console::WriteLine("Laser thread is starting");
	Watch = gcnew Stopwatch;
	auto connection = connect(DNS, PORT);

	SM_TM->ThreadBarrier->SignalAndWait();
	Watch->Start(); 

	while (connection == ERR_CONNECTION) {
		processHeartbeats();
		connection = connectionReattempt();
		Thread::Sleep(500);
	}

	if (connection == CONNECTION_TIMEOUT) {
		Console::WriteLine("Connection attempt failed after {0} attempts, terminating laser thread", connectionAttempts);
	}

	while (!getShutdownFlag() && connection == SUCCESS) {
		processHeartbeats();
		sendCommand("sRN LMDscandata");
		processSharedMemory();
		SM_DISPLAY->connectionStatus[0] = Client->Connected;

		if (SM_LASER->valid) {}

		//if (communicate() == SUCCESS && checkData() == SUCCESS)		// Communicate and validate
		//{
			//processSharedMemory();									// Print memory as per spec
		//}
		Thread::Sleep(20);
	}
	
	Client->Close();
	Console::WriteLine("Laser thread is terminating");
}
