#pragma once
/*****************
NetworkedModule.h
ALEXANDER CUNIO 2023
*****************/

/*
This class provides additional functionality for networked modules (Laser, GPS, Vehicle control, Display) beyond the
base UGV module class. These modules should therefore be derived from this rather than the base class.
For clarification of inheritance requirements see diagram in the assignment specification.
*/

#include "UGVModule.h"

// You will need to select which address to use depending on if you are working with the simulator (127.0.0.1)
// or the physical robot in the lab (192.168.1.200).
#define WEEDER_ADDRESS "127.0.0.1"
//#define WEEDER_ADDRESS "192.168.1.200"
#define DISPLAY_ADDRESS "127.0.0.1" // Display is always running locally

ref class NetworkedModule abstract : public UGVModule
{
	public:
		NetworkedModule(SM_ThreadManagement^ SM_TM, SM_Display^ SM_DISPLAY, String^ DNS, int PORT)
			: UGVModule(SM_TM), SM_DISPLAY(SM_DISPLAY), DNS(DNS), PORT(PORT), Client(gcnew TcpClient),
			Stream(nullptr), timeout(gcnew Stopwatch), connectionAttempts(0)  {}
		virtual error_state connect(String^ hostName, int portNumber) = 0;	// Establish TCP connection
		virtual error_state communicate() = 0;								// Communicate over TCP connection (includes any error checking required on data)

		// error_state write(array<unsigned char>^ WriteData) {}

	protected:
		TcpClient^ Client;					// Handle for TCP connection
		NetworkStream^ Stream;				// Handle for TCP data stream
		array<unsigned char>^ ReadData;		// Array to store sensor Data (only used for sensor modules)
		int PORT;
		String^ DNS;

		SM_Display^ SM_DISPLAY;
		Stopwatch^ timeout;
		int connectionAttempts;
};
