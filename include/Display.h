#pragma once

#using <System.dll>
#include "NetworkedModule.h"

typedef enum {
	THREAD = 0,
	GPS,
	CMD,
	CONTROLLER,
	CONNECTION,
}cliElem;

enum window {
	INACTIVE = -1,
	MAIN,
	NETWORK,
	GPSLOGS
};

ref class cliInterface {
public:
	cliInterface(SM_ThreadManagement^ ThreadInfo, SM_Display^ displayData);
	void init(window selectedWindow);
	void update();
	void changeWindow(window requested) { requestedWindow = requested; }
	void forceRefresh();

private:

	void updateThreadStatus();
	void updateGPS();
	void updateCMD();
	void updateController();
	void updateConnectionStatus();
	void updateUptime();

	void updateNetwork();
	void updateGPSLogs();

	window activeWindow;
	window requestedWindow;
	SM_ThreadManagement^ ThreadInfo;
	SM_Display^ displayData;
	array<uint8_t, 3>^ elemPositions;
	bool reinitialise;

	uint32_t cachedCRC;
	array<String^>^ GPSLogs;
	int logIndex;

};

ref class Display : public NetworkedModule
{
public:
	Display(SM_ThreadManagement^ SM_TM, SM_Display^ SM_DISPLAY);
	void sendDisplayData(array<double>^ xData, array<double>^ yData, NetworkStream^ stream);
	error_state connect(String^ hostName, int portNumber) override;
	error_state connectionReattempt();
	error_state communicate() override;
	error_state processSharedMemory() override;
	error_state processHeartbeats();
	void shutdownThreads();
	bool getShutdownFlag() override;
	void threadFunction() override;
	void processKey();

private:
	array<unsigned char>^ SendData;
	SM_Display^ SM_DISPLAY;
	cliInterface^ cli;
	ConsoleKey pressedKey;

};

String^ getRemoteIPAddress(Socket^ s);
String^ getLocalIPAddress(Socket^ s);