#include "VC.h"

#define VC_PORT  25000

VehicleControl::VehicleControl(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, SM_Display^ SM_DISPLAY)
    : NetworkedModule(SM_TM, SM_DISPLAY, gcnew String(WEEDER_ADDRESS), VC_PORT), SM_VC(SM_VC)
{
    Client = nullptr;
    Stream = nullptr;

    ReadData = gcnew array<unsigned char>(2048);
    SendData = gcnew array<unsigned char>(128);
    SendData = Encoding::ASCII->GetBytes("5309693\n");

    command = gcnew String("# <steer> <speed> <flag> #");
    flag = false;

}

error_state VehicleControl::connect(String^ hostName, int portNumber)
{
    Client = gcnew TcpClient(hostName, portNumber);
    Stream = Client->GetStream();

    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 2048;

    Stream->Write(SendData, 0, SendData->Length);

    String^ response = Encoding::ASCII->GetString(ReadData);

    if (response->Contains("OK\n")) {
        return SUCCESS;
    }
    return ERR_CONNECTION;
}

error_state VehicleControl::communicate()
{
    SendData = Encoding::ASCII->GetBytes(command);
    Stream->Write(SendData, 0, SendData->Length);


    Monitor::Enter(SM_DISPLAY->lockObject);
    SM_DISPLAY->sentCommand = command;
    Monitor::Exit(SM_DISPLAY->lockObject);


    return SUCCESS;
}

void VehicleControl::commandStr(double steer, double speed) {
    
    command = String::Format("# {0:N} {1:N} {2} #", steer, speed, static_cast<int>(flag));

    flag = !flag;
}

error_state VehicleControl::processSharedMemory()
{
    //extract data and format command string
    Monitor::Enter(SM_VC->lockObject);
    commandStr(SM_VC->Steering, SM_VC->Speed);
    Monitor::Exit(SM_VC->lockObject);

    Monitor::Enter(SM_DISPLAY->lockObject);
    SM_DISPLAY->connectionStatus[4] = Client->Connected;
    Monitor::Exit(SM_DISPLAY->lockObject);


    return error_state();
}

bool VehicleControl::getShutdownFlag() { return SM_TM->shutdown & bit_VC;}

void VehicleControl::threadFunction()
{
    Console::WriteLine("Vehicle control thread is starting");
    Watch = gcnew Stopwatch;
    connect(DNS, PORT);
    SM_TM->ThreadBarrier->SignalAndWait();
    Watch->Start();
    while (!getShutdownFlag()) {
        processHeartbeats();
        processSharedMemory();
        communicate();
        Thread::Sleep(100);
    }
    shutdownThreads();
    Console::WriteLine("Vehicle control thread is terminating");
}

error_state VehicleControl::processHeartbeats()
{
    if ((SM_TM->heartbeat & bit_VC) == 0)
    {
        SM_TM->heartbeat |= bit_VC;	// put VC flag up
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

void VehicleControl::shutdownThreads()
{
    commandStr(0, 0);
    communicate();
    Client->Close();
}
