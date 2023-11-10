#include "VC.h"

#define VC_PORT  25000

VehicleControl::VehicleControl(SM_ThreadManagement^ SM_TM, SM_VC^ SM_VC_, SM_Display^ SM_DISPLAY)
{
    SM_VehicleControl = SM_VC_;
    SM_TM_ = SM_TM;
    SM_DISPLAY_ = SM_DISPLAY;

    Watch = gcnew Stopwatch;

    Client = nullptr;
    Stream = nullptr;

    TcpPort = VC_PORT;
    DNS = gcnew String(WEEDER_ADDRESS);

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


    Monitor::Enter(SM_DISPLAY_->lockObject);
    SM_DISPLAY_->sentCommand = command;
    Monitor::Exit(SM_DISPLAY_->lockObject);


    return SUCCESS;
}

void VehicleControl::commandStr(double steer, double speed) {
    
    command = String::Format("# {0:N} {1:N} {2} #", steer, speed, static_cast<int>(flag));

    flag = !flag;
}

error_state VehicleControl::processSharedMemory()
{
    //extract data and format command string
    Monitor::Enter(SM_VehicleControl->lockObject);
    commandStr(SM_VehicleControl->Steering, SM_VehicleControl->Speed);
    Monitor::Exit(SM_VehicleControl->lockObject);

    Monitor::Enter(SM_DISPLAY_->lockObject);
    SM_DISPLAY_->connectionStatus[4] = Client->Connected;
    Monitor::Exit(SM_DISPLAY_->lockObject);


    return error_state();
}

bool VehicleControl::getShutdownFlag() { return SM_TM_->shutdown & bit_VC;}

void VehicleControl::threadFunction()
{
    Console::WriteLine("Vehicle control thread is starting");
    Watch = gcnew Stopwatch;
    connect(DNS, TcpPort);
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();
    while (!getShutdownFlag()) {
        processHeartbeats();
        processSharedMemory();
        communicate();
        Thread::Sleep(100);
    }
    Console::WriteLine("Vehicle control thread is terminating");
}

error_state VehicleControl::processHeartbeats()
{
    if ((SM_TM_->heartbeat & bit_VC) == 0)
    {
        SM_TM_->heartbeat |= bit_VC;	// put VC flag up
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
}
