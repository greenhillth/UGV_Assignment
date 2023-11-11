#include "GNSS.h"

#define GNSS_PORT 24000

#define CRC32_POLYNOMIAL 0xEDB88320L

unsigned long CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}

unsigned long CalculateBlockCRC32(unsigned long ulCount, unsigned char* ucBuffer)
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}


GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GNSS^ SM_GPS, SM_Display^ SM_DISPLAY)
	: NetworkedModule(SM_TM, SM_DISPLAY, gcnew String(WEEDER_ADDRESS), GNSS_PORT), SM_GPS(SM_GPS)
{
	GNSSData = gcnew array<unsigned char>(108);
}


error_state GNSS::processHeartbeats()
{
	if ((SM_TM->heartbeat & bit_GNSS) == 0)
	{
		SM_TM->heartbeat |= bit_GNSS;	// put gnss flag up
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

void GNSS::shutdownThreads() { SM_TM->shutdown = 0xFF; }

bool GNSS::getShutdownFlag() { return SM_TM->shutdown & bit_GNSS; }

error_state GNSS::communicate()
{
	unsigned int header = 0;
	Byte data;
	while (header != 0xAA44121C) {
		data = Stream->ReadByte();
		if (static_cast<int>(data) == -1) { return ERR_INVALID_DATA; }
		header = (header << 8) | data;
	}

	if (Stream->Read(GNSSData, 0, 108) == 108) { return SUCCESS; }

	return ERR_NO_DATA_RECEIVED;
}

error_state GNSS::checkData()
{
	if (CRC == 0) { return ERR_NO_DATA; }
	pin_ptr<unsigned char> charPtr = &GNSSData[0];
	auto calculation = CalculateBlockCRC32(GNSSData->Length, charPtr);
	auto crc = CRC;
	bool result = calculation == CRC;
	if (result) {
		return SUCCESS;
	}
	return ERR_INVALID_DATA;
}

error_state GNSS::processSharedMemory()
{
	double Northing = BitConverter::ToDouble(GNSSData, 40);
	double Easting = BitConverter::ToDouble(GNSSData, 48);
	double Height = BitConverter::ToDouble(GNSSData, 56);

	Monitor::Enter(SM_GPS->lockObject);
	SM_GPS->Northing = Northing;
	SM_GPS->Easting = Easting;
	SM_GPS->Height = Height;
	Monitor::Exit(SM_GPS->lockObject);
	CRC = BitConverter::ToUInt32(GNSSData, 104);
	for (int i = 0; i < 108; i++) { GNSSData[i] = 0; }

	SM_DISPLAY->GPSData[0] = Northing;
	SM_DISPLAY->GPSData[1] = Easting;
	SM_DISPLAY->GPSData[2] = Height;
	return SUCCESS;
};




error_state GNSS::connect(String^ hostName, int portNumber)
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

	return SUCCESS;
}

error_state GNSS::connectionReattempt()
{
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

void GNSS::threadFunction()
{
	Console::WriteLine("GNSS thread is starting");
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
		SM_DISPLAY->connectionStatus[2] = Client->Connected;
		if (communicate() == SUCCESS && checkData() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("GNSS thread is terminating");
}