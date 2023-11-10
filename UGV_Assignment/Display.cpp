#include "Display.h"

#define DISPLAY_PORT  28000


Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_LASER, SM_Display^ SM_DISPLAY)
{
    SM_TM_ = SM_TM;
    SM_Laser_ = SM_LASER;
    SM_DISPLAY = SM_DISPLAY;

    Watch = gcnew Stopwatch;

    TcpPort = DISPLAY_PORT;
    DNS = gcnew String(DISPLAY_ADDRESS);

    Client = gcnew TcpClient();
    Stream = nullptr;
    cli = gcnew cliInterface(SM_TM_, SM_DISPLAY);
}

void Display::sendDisplayData(array<double>^ xData, array<double>^ yData, NetworkStream^ stream) {
    // Serialize the data arrays to a byte array
    //(format required for sending)
    array<Byte>^ dataX =
        gcnew array<Byte>(SM_Laser_->x->Length * sizeof(double));
    Buffer::BlockCopy(SM_Laser_->x, 0, dataX, 0, dataX->Length);
    array<Byte>^ dataY =
        gcnew array<Byte>(SM_Laser_->y->Length * sizeof(double));
    Buffer::BlockCopy(SM_Laser_->y, 0, dataY, 0, dataY->Length);
    // Send byte data over connection
    Stream->Write(dataX, 0, dataX->Length);
    Thread::Sleep(10);
    Stream->Write(dataY, 0, dataY->Length);
}

error_state Display::connect(String^ hostName, int portNumber)
{
    Client = gcnew TcpClient(hostName, portNumber);
    Stream = Client->GetStream();

    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    ReadData = gcnew array<unsigned char>(128);
    SendData = gcnew array<unsigned char>(128);

    return SUCCESS;
}

error_state Display::communicate()
{
    return error_state();
}

error_state Display::processSharedMemory()
{
    SM_DISPLAY_->connectionStatus[2] = Client->Connected;
    return SUCCESS;
}

error_state Display::processHeartbeats()
{
    if ((SM_TM_->heartbeat & bit_DISPLAY) == 0)
    {
        SM_TM_->heartbeat |= bit_DISPLAY;	// put laser flag up
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

void Display::shutdownThreads()
{}

bool Display::getShutdownFlag() { return SM_TM_->shutdown & bit_DISPLAY; }


void Display::threadFunction()
{
    Console::WriteLine("Display thread is starting");
    Watch = gcnew Stopwatch;
    //connect(DNS, TcpPort);
    cli->init();
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();
    while (!getShutdownFlag()) {
        //Console::WriteLine("Display thread is running");
        processHeartbeats();
        //sendDisplayData(SM_Laser_->x, SM_Laser_->y, Stream);
        cli->update();
        Thread::Sleep(20);
    }
    Console::WriteLine("Display thread is terminating");
}


cliInterface::cliInterface(SM_ThreadManagement^ ThreadInfo, SM_Display^ displayData) : ThreadInfo(ThreadInfo), displayData(displayData)
{
    windowActive = false;
    elemPositions = gcnew array<uint8_t, 3>(5, 6, 2) {
        { { 8, 6 }, { 26,6 }, { 44,6 }, { 62,6 }, { 80,6 }, { 98,6 } },   // Thread co-ords
        { { 55, 22 } },                                                   // GPS co-ord
        { { 50, 13 } },                                                   // CMD co-ord
        { { 20, 18 }, { 20, 19 }, { 20, 20 }, { 4, 21 } },               // Controller co-ords
        { { 98, 17 }, { 100, 18 }, { 97, 19 }, { 103, 20 }, { 95, 21} }              // Connection co-ords
    };

}

void cliInterface::init()
{
    //TODO - set appropriate buffer and window sizes to make display more robust
    windowActive = true;
    Console::CursorVisible = false;
    Console::Write("Initialising display interface");
    Thread::Sleep(50);
    Console::Clear();

    Console::WriteLine("=======================================================================================================================\n" +
                       "                                                UGV DISPLAY WINDOW v1.00                                               \n" +
                       "=======================================================================================================================\n" +
                       "                                                    THREAD STATUS                                                      \n" +
                       "        Thread #1         Thread #2         Thread #3         Thread #4         Thread #5         Thread #6            \n" +
                       "            TMM              Laser             GNSS           Controller      Vehicle Control      Display             \n" +
                       "        Initialising      Initialising      Initialising      Initialising      Initialising      Initialising         \n" +
                       "                                                                                                                       \n" +
                       "                                                                                                                       \n" +
                       "                                                                                                                       \n" +
                       "                                                                                                                       \n" +
                       "                                                                                                                       \n" +
                       "                                                     LAST COMMAND SENT:                                                \n" +
                       "                                                                                                                       \n" +
                       "                                                                                                                       \n" +
                       "                                                                                                                       \n" +
                       "    Controller Inputs:                                          ||                        Connection Status:           \n" +
                       "    [button] : [value]                              ____________||__                      Laser - Connected            \n" +
                       "    left trigger  :                              [=|   WEEDER      |]                     Display - Connected          \n" +
                       "    right trigger :                               ~_|_____________ |                      GNSS - Connected             \n" +
                       "    right stick   :                                 //||      || ||                       Controller - Connected       \n" +
                       "                                                   (_)(_)    (_)(_)                       VC - Connected               \n" +
                       "                                               Coords:  x,y,z                        Uptime:                           \n" +
                       "=======================================================================================================================\n" +
                       "=======================================================================================================================\n" +
                       "(press Q to quit)");
}

void cliInterface::update()
{
    if (windowActive) {
        updateThreadStatus();
        updateGPS();
        updateCMD();
        updateController();
        updateConnectionStatus();
        updateUptime();
    }
}

void cliInterface::updateUptime() {
    TimeSpan time = displayData->uptime->Elapsed;
    Console::SetCursorPosition(93, 22);
    Console::Write("{0:00}:{1:00}:{2:00}.{3:000}       ", time.Hours, time.Minutes, time.Seconds, time.Milliseconds);
}

void cliInterface::updateThreadStatus()
{
    auto green = ConsoleColor::Green;
    auto red = ConsoleColor::Red;
    for (int i = 0; i < 6; i++) {
        auto status = displayData->threadStatus[i];
        Console::SetCursorPosition(elemPositions[THREAD, i, 0], elemPositions[THREAD, i, 1]);       //set cursor position
        if (status == Threading::ThreadState::Running) { Console::ForegroundColor = green; Console::Write("  Running   "); }                    //write 
        else if (status == Threading::ThreadState::Stopped) { Console::ForegroundColor = red; Console::Write("  Stopped   "); }
        else if (status == Threading::ThreadState::Suspended) { Console::Write(" Suspended  "); }
        else if (status == Threading::ThreadState::Background) { Console::Write("Background  "); }
        else if (status == Threading::ThreadState::WaitSleepJoin) { Console::Write("  Waiting   "); }
        else { Console::Write("doin stuff"); }
        Console::ResetColor();
    }
}

void cliInterface::updateGPS()
{
    double northing = displayData->GPSData[0];
    double easting = displayData->GPSData[1];
    double height = displayData->GPSData[2];
    
    Console::SetCursorPosition(elemPositions[GPS, 0, 0], elemPositions[GPS, 0, 1]);       //set cursor position
    Console::Write("{0:N}, {1:N}, {2:N}         ",northing, easting, height);
}

void cliInterface::updateCMD()
{
    if (displayData->sentCommand != nullptr) {
        Console::SetCursorPosition(elemPositions[CMD, 0, 0], elemPositions[CMD, 0, 1]);       //set cursor position
        Console::Write("   "+ displayData->sentCommand + "   ");
    }
}

void cliInterface::updateController()
{   
    auto inputs = displayData->Controller->GetState();
    if (displayData->Controller != nullptr) {
        double controllerVals[3] = { inputs.leftTrigger, inputs.rightTrigger, inputs.rightThumbX};
        bool aButton = inputs.buttonA;
        for (int i = 0; i < 3; i++) {
            Console::SetCursorPosition(elemPositions[CONTROLLER, i, 0], elemPositions[CONTROLLER, i, 1]);       //set cursor position
            Console::Write("{0:N}      ", controllerVals[i]);
        }
        Console::SetCursorPosition(elemPositions[CONTROLLER, 3, 0], elemPositions[CONTROLLER, 3, 1]);
        if (aButton) {Console::Write("Button A pressed");}
        else { Console::Write("                "); }
    }
    
}

void cliInterface::updateConnectionStatus()
{
    auto green = ConsoleColor::Green;
    auto red = ConsoleColor::Red;
    auto status = displayData->connectionStatus;
    for (int i = 0; i < status->Length; i++) {
        Console::SetCursorPosition(elemPositions[CONNECTION, i, 0], elemPositions[CONNECTION, i, 1]);
        if (status[i]) { Console::ForegroundColor = green; Console::Write("Connected   "); }
        else { Console::ForegroundColor = red; Console::Write("Disconnected"); }
    }
    Console::ResetColor();
}
