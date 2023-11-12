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
    SM_DISPLAY->connectionHandles[3] = Client;

    command = gcnew String("# <steer> <speed> <flag> #");
    flag = false;

}

error_state VehicleControl::connect(String^ hostName, int portNumber)
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
    SM_DISPLAY->connectionHandles[3] = Client;
    SM_DISPLAY->connectionStatus[4]->Start();
    Stream = Client->GetStream();

    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 2048;

    Stream->Write(SendData, 0, SendData->Length);

    for (int i = 0; !Stream->DataAvailable && (i < 5); i++) {
        System::Threading::Thread::Sleep(10);
    }
    
    if (!Stream->DataAvailable) { return ERR_NO_DATA; }
    Stream->Read(ReadData, 0, ReadData->Length);
    String^ response = Encoding::ASCII->GetString(ReadData);

    if (response->Contains("OK\n")) {
        return SUCCESS;
    }
    return ERR_VALIDATION;
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
    commandStr(SM_VC->Steering, SM_VC->Speed);

    return SUCCESS;
}

bool VehicleControl::getShutdownFlag() { return SM_TM->shutdown & bit_VC;}

//Reattempt connection 5 times in intervals of 10s before timeout
error_state VehicleControl::connectionReattempt() {

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

void VehicleControl::threadFunction()
{
    Console::WriteLine("Vehicle control thread is starting");
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
        Console::WriteLine("Connection attempt failed after {0} attempts, terminating VC thread", connectionAttempts);
    }

    while (!getShutdownFlag() && connection == SUCCESS) {
        processHeartbeats();
        processSharedMemory();
        communicate();
        Thread::Sleep(100);
    }

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

void VehicleControl::shutdownThreads() { SM_TM->shutdown = 0xFF; }

