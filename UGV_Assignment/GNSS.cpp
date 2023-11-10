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


GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GNSS^ SM_GNSS, SM_Display^ SM_DISPLAY)
{
	SM_GNSS_ = SM_GNSS;
	SM_TM_ = SM_TM;
	SM_DISPLAY_ = SM_DISPLAY;
	Watch = nullptr;

	Client = nullptr;
	Stream = nullptr;
	TcpPort = GNSS_PORT;
	DNS = gcnew String(WEEDER_ADDRESS);

	GNSSData = gcnew array<unsigned char>(108);

}

error_state GNSS::setupSharedMemory()
{
	return SUCCESS;
}


error_state GNSS::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_GNSS) == 0)
	{
		SM_TM_->heartbeat |= bit_GNSS;	// put gnss flag up
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

void GNSS::shutdownThreads()
{
}

bool GNSS::getShutdownFlag() { return SM_TM_->shutdown & bit_GNSS; }

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
	/*
	CRC
	*/
	return SUCCESS;
}

error_state GNSS::processSharedMemory()
{
	double Northing = BitConverter::ToDouble(GNSSData, 40);
	double Easting = BitConverter::ToDouble(GNSSData, 48);
	double Height = BitConverter::ToDouble(GNSSData, 56);

	Monitor::Enter(SM_GNSS_->lockObject);
	SM_GNSS_->Northing = Northing;
	SM_GNSS_->Easting = Easting;
	SM_GNSS_->Height = Height;
	SM_GNSS_->CRC = BitConverter::ToUInt32(GNSSData, 80);
	Monitor::Exit(SM_GNSS_->lockObject);
	for (int i = 0; i < 108; i++) { GNSSData[i] = 0; }

	SM_DISPLAY_->GPSData[0] = Northing;
	SM_DISPLAY_->GPSData[1] = Easting;
	SM_DISPLAY_->GPSData[2] = Height;
	return SUCCESS;
};




error_state GNSS::connect(String^ hostName, int portNumber)
{
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();

	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 2048;

	return SUCCESS;
}

void GNSS::threadFunction()
{
	Console::WriteLine("GNSS thread is starting");
	Watch = gcnew Stopwatch;
	connect(DNS, TcpPort);
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		
		processHeartbeats();
		SM_DISPLAY_->connectionStatus[2] = Client->Connected;
		/*
		if (communicate() == SUCCESS && checkData() == SUCCESS)
		{
			processSharedMemory();
		}
		*/
		Thread::Sleep(20);
	}
	Console::WriteLine("GNSS thread is terminating");
}