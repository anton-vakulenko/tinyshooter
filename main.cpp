//    Copyright (C) 2018 Anton Vakulenko
//    e-mail: anton.vakulenko@gmail.com
//
//    This file is part of TinyShooter.
//
//    TinyShooter is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    TinyShooter is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with TinyShooter.  If not, see <http://www.gnu.org/licenses/>.

#include <exception>
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdio.h>
#include <ctime>
#include <shlobj.h> // for choose folder dialog
#include "resource.h"
#include "EDSDK.h"
#include "EDSDKErrors.h"
#include "EDSDKTypes.h"

using namespace std;

// application name
const char AppName [16] = "TinyShooter 1.0";

// variables for settings
int exposure = 300;             // default single frame exposure, seconds
int repeat = 1;                 // default number of frames to shoot
int wait = 0;                   // default pause before first frame, MINUTES!
int comport = 1;                // default serial port number
int mirror = 5;                 // default time for mirror to settle (0 is not allowed)
int pause = 30;                 // default pause between frames, seconds
int dither_distance = 3;        // default dither distance for PHD (see comments in Dither() function)
int eos = 0;                    // we don't want to save images to PC by default
bool dither_bool = false;       // we don't want to dither by default
char download_to[256] = "C:";   // default path to download images from camera

// varialbles for timer procedure
int TimeElapsed = 0;            // number of seconds, passed for each status (should reset to 0 at the end of each shooting stage)
int TimeLeft;                   // time left till the end of shooting session (initially it equals total session's time) See #define TIMELEFT
int Frame = 0;                  // number of frames taken (should reset to 0 at the end of each shooting session)
string status = "Mirror";       // status (should reset to "Mirror" at the end of each shooting session)

// misc handles
HANDLE Port;                    // Serial port
SOCKET phd;                     // Socket handle
HINSTANCE hInst;                // for winapi

// Calculates total session time in seconds
#define TIMELEFT (exposure + mirror + pause)*repeat - pause + wait*60

EdsError EDSCALLBACK handleObjectEvent( EdsObjectEvent event, EdsBaseRef object, EdsVoid * context)
{
    // this function downloads images from camera to PC
    // getting file info to download
    EdsDirectoryItemInfo dirItemInfo;
    EdsGetDirectoryItemInfo(object, &dirItemInfo);
    // create file stream to download image
    // image filename would be IMG_yyyy-MM-dd_HHmmss.CR2
    // otherwise images from new session will overwrite old ones
    EdsStreamRef stream = NULL;
    // get and format current time
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[25] = "";       // will contain filename only
    char full_path[256] = "";   // will contain full path (with filename)
    time (&rawtime);
    timeinfo = localtime(&rawtime); // current time in unreadable format
    // format filename
    strftime(buffer,sizeof(buffer),"\\IMG_%Y%m%d_%H%M%S.CR2",timeinfo);
    // append download path
    strcat(full_path,download_to);
    // append filename
    strcat(full_path,buffer);
    EdsCreateFileStream(full_path, kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
    // download image
    EdsDownload(object, dirItemInfo.size, stream);
    // done!
    EdsDownloadComplete(object);
    // free resourses
    EdsRelease(stream);
    EdsRelease(object);
    return EDS_ERR_OK;
}

EdsError CameraConnect()
{
    // function to connect to camera
    // Canon EOS only supported! And only ONE camera. Needs EDSDK.dll also!
    // if everything is OK, returns EDS_ERR_OK
    EdsError err = EDS_ERR_OK;
    EdsCameraListRef cameraList = NULL;
    EdsCameraRef camera = NULL;
    // checking if we have EDSDK.dll
    ifstream dll("EDSDK.dll");
    if (dll.fail())
    {
        // dll missing!
        MessageBox(NULL,"EDSDK.dll not found. Place it in application's folder.",AppName,MB_ICONWARNING);
        err = EDS_ERR_MISSING_SUBCOMPONENT;
        exit;
    }
    else
    {
        //dll in place, let's connect to camera
        dll.close();
        // Initialize SDK
        EdsInitializeSDK();
        // get list of connected cameras
        EdsGetCameraList(&cameraList);
        // get first connected camera
        EdsGetChildAtIndex(cameraList, 0, &camera);
        // start session with selected camera
        err = EdsOpenSession(camera);
        // save images to pc
        EdsUInt32 intSaveTo = kEdsSaveTo_Host;
        EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0, sizeof(intSaveTo), &intSaveTo);
        // tell camera that we have a plenty of free space on hdd
        EdsCapacity capacity;
        capacity.numberOfFreeClusters = 0x7FFFFFFF;
        capacity.bytesPerSector = 0x1000;
        capacity.reset = 1;
        EdsSetCapacity(camera, capacity);
        // set event handler to download images from camera's memory to pc
        EdsSetObjectEventHandler(camera, kEdsObjectEvent_DirItemRequestTransfer, handleObjectEvent, NULL);
        // checking if we got any errors
        if (err != EDS_ERR_OK)
        {
            MessageBox(NULL,"Can't connect to camera",AppName,MB_ICONWARNING);
        }
        return err;
    }
}

bool PHD(const string& s)
{
    // connect and disconnect to/from PHD
    // check firewall setting in case of error
    if (s == "Connect")
    {
        //Start up Winsock v.2
        WSADATA wsadata;
        WSAStartup(0x0202, &wsadata);
        //Fill out the information needed to initialize a socket
        SOCKADDR_IN target;                                 //Socket address information
        target.sin_family = AF_INET;                        // address family Internet
        target.sin_port = htons (4300);                     //Port to connect on
        target.sin_addr.s_addr = inet_addr ("127.0.0.1");   //Target IP
        //Create socket
        phd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        //Try connecting...
        if (connect(phd, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR)
        {
            MessageBox(GetActiveWindow(),"Can't connect to PHD",AppName,MB_ICONWARNING);
            return false; //Couldn't connect
        }
        else
        {
            return true; //Success
        }
    }
    if (s == "Disconnect")
    {
        //Close the socket if it exists
        if (phd)
            closesocket(phd);
        //Clean up Winsock
        WSACleanup();
        return true;
    }
}

void Dither ()
{
    // say dither to PHD. more details here:
    // https://github.com/OpenPHDGuiding/phd2/wiki/SocketServerInterface
    //MSG_MOVE1 	3 	Dither a random amount, up to +/- 0.5 x dither_scale
    //MSG_MOVE2 	4 	Dither a random amount, up to +/- 1.0 x dither_scale
    //MSG_MOVE3 	5 	Dither a random amount, up to +/- 2.0 x dither_scale
    //MSG_MOVE4 	12 	Dither a random amount, up to +/- 3.0 x dither_scale
    //MSG_MOVE5 	13 	Dither a random amount, up to +/- 5.0 x dither_scale
    char cmd; // contains command
    // select appropriate PHD code to command
    switch (dither_distance)
    {
    case 0:
    {
        cmd = 3;
        break;
    }
    case 1:
    {
        cmd = 4;
        break;
    }
    case 2:
    {
        cmd = 5;
        break;
    }
    case 3:
    {
        cmd = 12;
        break;
    }
    case 5:
    {
        cmd = 13;
        break;
    }
    }
    send(phd,&cmd,strlen(&cmd),0); // send command to PHD
}

bool ComPort (const string& s)
{
    // open-close serial port routines
    if (s == "Open")
    {
        //open serial port
        char com [13];
        sprintf(com,"\\\\.\\COM%d",comport);

        Port = CreateFile(com, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (Port == INVALID_HANDLE_VALUE)
        {
            MessageBox(NULL,"Can't open serial port. Check settings.",AppName,MB_ICONWARNING);
            // no serial port
            return false;
        }
        else
        {
            // serial port opened successfully, we can continue
            DCB dcb;
            FillMemory(&dcb, sizeof(dcb), 0);
            // get current DCB
            GetCommState(Port, &dcb);
            // Update DCB
            dcb.BaudRate = CBR_9600 ;   // unsure is dcb.BaudRate is nessessary?
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
            dcb.fDtrControl = DTR_CONTROL_DISABLE;
            // Set new state
            SetCommState(Port, &dcb);
            return true;
        }
    }
    if (s == "Close")
    {
        //close serial port
        CloseHandle(Port);
        return true;
    }
}

void EmptyCheck (HWND hwnd)
{
    // check if all fields in settings window are filled and not equal to 0
    // if not disables Save button
    // obviously not a perfect solution, but it works ok
    BOOL bSuccess1, bSuccess2, bSuccess3;
    GetDlgItemInt(hwnd,ID_COM,&bSuccess1,false);

    if ( GetDlgItemInt(hwnd,ID_MIRROR,&bSuccess2,false) == 0)
    {
        SetDlgItemInt(hwnd,ID_MIRROR,1,false);
    }
    if ( GetDlgItemInt(hwnd,ID_PAUSE,&bSuccess3,false) == 0)
    {
        SetDlgItemInt(hwnd,ID_PAUSE,1,false);
    }
    if (bSuccess1 & bSuccess2 & bSuccess3)
    {
        EnableWindow( GetDlgItem( hwnd, IDOK ), TRUE);
    }
    else
    {
        EnableWindow( GetDlgItem( hwnd, IDOK ), FALSE);
    }
}

string FormatTime (int seconds)
{
    // prepare text for timing information

    char ft[55] = "";
    int te, h1, h2, m1, m2, s1, s2;

    te = exposure * repeat; // total exposure

    h1 = te / 3600;
    m1 = (te % 3600) / 60;
    s1 = (te % 3600) % 60;

    h2 = seconds / 3600;
    m2 = (seconds % 3600) / 60;
    s2 = (seconds % 3600) % 60;

    sprintf(ft,"Total exposure: %02d:%02d:%02d     Time left: %02d:%02d:%02d", h1, m1, s1, h2, m2, s2);
    string str(ft);

    return str;
}

void Statistics(const string& t)
{
    // put text at the bottom of the main window
    char line1 [55];
    char line2 [39];
    int te, h1, h2, m1, m2, s1, s2;

    te = exposure * repeat; // total exposure (this is not a total session time!)

    // convert seconds in human readable format
    h1 = te / 3600;
    m1 = (te % 3600) / 60;
    s1 = (te % 3600) % 60;
    h2 = TimeLeft / 3600;
    m2 = (TimeLeft % 3600) / 60;
    s2 = (TimeLeft % 3600) % 60;

    sprintf(line1,"Total exposure: %02d:%02d:%02d     Time left: %02d:%02d:%02d", h1, m1, s1, h2, m2, s2);

    if (t == "Wait")
    {
        sprintf(line2, "Wait to start session... %d sec", TimeElapsed);
        SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_STEPIT, 0, 0);
    }
    if (t == "Mirror")
    {
        sprintf(line2, "[Frame %d] Mirror settling... %d sec", Frame + 1, TimeElapsed);
        SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_STEPIT, 0, 0);
    }
    if (t == "Capture")
    {
        sprintf(line2, "[Frame %d] Capturing... %d sec", Frame + 1, TimeElapsed);
        SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_STEPIT, 0, 0);
    }
    if (t == "Pause")
    {
        sprintf(line2, "[Frame %d] Pause... %d sec", Frame + 1, TimeElapsed);
        SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_STEPIT, 0, 0);
    }
    if (t == "Idle")
    {
        strcpy(line2, "Idle");
    }
    if (t == "Empty")
    {
        strcpy(line1, "Fill in all data!");
        strcpy(line2, "Idle");
    }
    if (t == "Aborted")
    {
        strcpy(line2, "Session aborted");
        SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_SETPOS, 0, 0);
    }
    if (t == "Complete")
    {
        strcpy(line2, "Session complete");
        SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_SETPOS, 0, 0);
    }
    // update text lines
    SetDlgItemText(GetActiveWindow(), ID_TIME, line1);
    SetDlgItemText(GetActiveWindow(), ID_STATUS, line2);
}

void CleanUp()
{
    // clean up things at the and of shooting session
    // stop main timer
    KillTimer (GetActiveWindow(), 1);
    // drops all variables for main timer procedure to defaults
    status = "Mirror";
    TimeElapsed = 0;
    Frame = 0;
    // disconnect from PHD
    PHD("Disconnect");
    // close serial port
    EscapeCommFunction(Port, CLRRTS);
    EscapeCommFunction(Port, CLRDTR);
    ComPort("Close");
    // do cosmetics
    SetDlgItemText(GetActiveWindow(),ID_START_BUTTON, "Start" );
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_EXP ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_COUNT ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_PHD ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_WAIT ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_SETTINGS_BUTTON ), TRUE);
}

void TimerProcKillESDK(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4)
{
    // if we terminate edsdk just after last frame is done, we abort downloading of the last image
    // so we need to wait for some time (5 sec should be enough I think)
    // also not a perfect solution but it works ok
    // 5 seconds left, lets kill edsdk
    EdsTerminateSDK();
    // stop timer
    KillTimer(GetActiveWindow(),2);
}

void TimerProc(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4)
{
    // main shooting procedure
    // here we open and close shutter, dither and update statistics

    // at first we need to wait till initial pause time (wait) is left
    if (wait != 0)
    {
        TimeLeft -= 1;
        TimeElapsed += 1;
        // update statistics
        Statistics("Wait");
        // checking if time to wait before session is elapsed
        if (TimeElapsed == wait*60)
        {
            // reset
            wait = 0;
            TimeElapsed = 0;
        }
        // we exit now from ticking cycle or we skip one second and get to next "if"
        goto end_tick_cycle;
    }

    // now it's time to check our status
    // it could be mirror, capture and pause
    if (status == "Mirror")
    {
        if (TimeElapsed == 0)
        {
            // shutter button down
            EscapeCommFunction(Port, SETRTS);
            EscapeCommFunction(Port, SETDTR);
            // wait a bit
            Sleep(50);
            // shutter button up
            EscapeCommFunction(Port, CLRRTS);
            EscapeCommFunction(Port, CLRDTR);
            // now we just made mirror lock-up
        }

        TimeLeft -= 1;
        TimeElapsed += 1;
        // update statistics
        Statistics("Mirror");

        // checking if mirror settle time elapsed
        if (TimeElapsed == mirror)
        {
            // change status
            status = "Capture";
            TimeElapsed = 0;
        }

        // we exit now from ticking cycle or we skip one second and get to next "if"
        goto end_tick_cycle;
    }

    if (status == "Capture")
    {
        if (TimeElapsed == 0)
        {
            // shutter button down (we open shutter)
            EscapeCommFunction(Port, SETRTS);
            EscapeCommFunction(Port, SETDTR);
        }

        TimeLeft -= 1;
        TimeElapsed += 1;

        // update statistics
        Statistics("Capture");

        // checking if exposure time elapsed
        if (TimeElapsed == exposure)
        {
            // change status
            status = "Pause";
            TimeElapsed = 0;
        }
        // we exit now from ticking cycle or we skip one second and get to next "if"
        goto end_tick_cycle;
    }

    if (status == "Pause")
    {
        if (TimeElapsed == 0)
        {
            // shutter button up (close shutter, mirror down)
            EscapeCommFunction(Port, CLRRTS);
            EscapeCommFunction(Port, CLRDTR);
            // this is the end of the single frame shooting
            // now we say "dither" to PHD (if needed)
            if ((IsDlgButtonChecked(GetActiveWindow(),ID_PHD) == BST_CHECKED) && (Frame + 1 != repeat)) // we don't dither after last frame!
            {
                Dither();
            }
        }

        if (Frame + 1 == repeat)
        {
            // all frames are done
            Frame = 0;
            // cosmetics
            TimeLeft = TIMELEFT;
            Statistics("Complete");
            // call this to stop our shooting session
            CleanUp();
            // wait 5 seconds to download last image from camera to pc
            // more detail in TimerProcKillEDSDK() function
            SetTimer(GetActiveWindow(), 2, 5000, (TIMERPROC) TimerProcKillESDK);
        }
        else
        {
            // not all frames are done
            TimeLeft -= 1;
            TimeElapsed += 1;
            // cosmetics
            Statistics("Pause");
        }

        // checking if pause time elapsed
        if (TimeElapsed == pause)
        {
            TimeElapsed = 0;
            // change status
            status = "Mirror";
            Frame += 1;
        }
    }
// we get here when status changes
end_tick_cycle:
    ;
}

string BrowseFolder()
{
    // show browse folder dialog and get selected path
    BROWSEINFO bi = { 0 };
    bi.hwndOwner  =  GetActiveWindow();
    bi.lpszTitle  = ("Choose folder to download images from camera:");
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );

    if ( pidl != 0 )
    {
        //get the name of the folder and put it in path
        TCHAR path[MAX_PATH];
        SHGetPathFromIDList ( pidl, path );

        //free memory used
        IMalloc * imalloc = 0;
        if ( SUCCEEDED( SHGetMalloc ( &imalloc )) )
        {
            imalloc->Free ( pidl );
            imalloc->Release ( );
        }

        return path;
    }
    return "";
}

void SaveSettings()
{
    // save settings to ts.cfg - configuration file
    // note, we don't save values from EDITBOXES, RADIOBUTTONS and so on. We save values of the VARIABLES!
    // so one should always update variable's values before run this function
    // ts.cfg has fixed format:
    // 1st line: "don't edit..."
    // 2nd line: comport
    // 3nd line: mirror
    // 4th line: pause
    // 5th line: dither_distance
    // 6th line: eos
    // 7th line: download_to
    ofstream config_file ("ts.cfg", ios::out | ios::trunc);
    config_file << "DO NOT EDIT THIS FILE! USE SETTINGS BUTTON IN APPLICATION" << endl;
    config_file << comport << endl;
    config_file << mirror << endl;
    config_file << pause << endl;
    config_file << dither_distance << endl;
    config_file << eos << endl;
    config_file << download_to << endl;
    config_file.close();
}

void LoadSettings()
{
    // check ts.cfg and read from it
    ifstream config_file("ts.cfg");
    try
    {
        string s;
        getline(config_file,s);
        getline(config_file,s);
        comport = stoi(s);
        getline(config_file,s);
        mirror = stoi(s);
        getline(config_file,s);
        pause = stoi(s);
        getline(config_file,s);
        dither_distance = stoi(s);
        getline(config_file,s);
        eos = stoi(s);
        // in case of error with stoi(), exception will be thrown automatically
        // but with the next line we have to throw it manually:
        if (getline(config_file,s))
        {
            strcpy (download_to,s.c_str());
        }
        else
        {
            throw(1);
        }
        config_file.close();
    }
    catch (...)
        // we don't find ts.cfg or it has errors
    {
        MessageBox(GetActiveWindow(),"Configuration file is missing or corrupted. Defaults will be used. Check settings.",AppName,MB_ICONWARNING);
        // the next line creates new ts.cfg with default settings
        SaveSettings();
    }
}

BOOL CALLBACK DlgSettings(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // here all messages from settings window are processed
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // set icon
        HICON hIcon;
        hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(IDI_ICON1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
        if (hIcon)
        {
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        // load settings from ts.cfg
        LoadSettings();

        // set values to dialog from ts.cfg
        SetDlgItemInt(hwndDlg,ID_COM,comport,false);
        SetDlgItemInt(hwndDlg,ID_MIRROR,mirror,false);
        SetDlgItemInt(hwndDlg,ID_PAUSE,pause,false);
        SetDlgItemText(hwndDlg,ID_PATH,download_to);

        switch (dither_distance) // details on values in Dither() function
        {
        case 0:
        {
            SendDlgItemMessage(hwndDlg,ID_05,BM_SETCHECK,BST_CHECKED,NULL);
            break;
        }
        case 1:
        {
            SendDlgItemMessage(hwndDlg,ID_1,BM_SETCHECK,BST_CHECKED,NULL);
            break;
        }
        case 2:
        {
            SendDlgItemMessage(hwndDlg,ID_2,BM_SETCHECK,BST_CHECKED,NULL);
            break;
        }
        case 3:
        {
            SendDlgItemMessage(hwndDlg,ID_3,BM_SETCHECK,BST_CHECKED,NULL);
            break;
        }
        case 5:
        {
            SendDlgItemMessage(hwndDlg,ID_5,BM_SETCHECK,BST_CHECKED,NULL);
            break;
        }
        }

        if (eos == 1)
        {
            CheckDlgButton(hwndDlg,ID_DOWNLOAD,BST_CHECKED);
        }
        else
        {
            CheckDlgButton(hwndDlg,ID_DOWNLOAD,BST_UNCHECKED);
        }
    }
    return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_PATH_BUTTON:
        {
            // choose path to download images here
            strcpy (download_to,BrowseFolder().c_str());
            SetDlgItemText(hwndDlg,ID_PATH,download_to);
            break;
        }
        case IDOK:
        {
            // save entered values to variables
            BOOL bSuccess;
            comport = GetDlgItemInt(hwndDlg,ID_COM,&bSuccess,false);
            mirror = GetDlgItemInt(hwndDlg,ID_MIRROR,&bSuccess,false);
            pause = GetDlgItemInt(hwndDlg,ID_PAUSE,&bSuccess,false);

            if (IsDlgButtonChecked(hwndDlg,ID_05) == BST_CHECKED)
            {
                dither_distance = 0;
            }
            if (IsDlgButtonChecked(hwndDlg,ID_1) == BST_CHECKED)
            {
                dither_distance = 1;
            }
            if (IsDlgButtonChecked(hwndDlg,ID_2) == BST_CHECKED)
            {
                dither_distance = 2;
            }
            if (IsDlgButtonChecked(hwndDlg,ID_3) == BST_CHECKED)
            {
                dither_distance = 3;
            }
            if (IsDlgButtonChecked(hwndDlg,ID_5) == BST_CHECKED)
            {
                dither_distance = 5;
            }

            if (IsDlgButtonChecked(hwndDlg,ID_DOWNLOAD) == BST_CHECKED)
            {
                eos = 1;
            }
            else
            {
                eos = 0;
            }
            GetDlgItemText(hwndDlg, ID_PATH, download_to, 256);
            // write variables to ts.cfg
            SaveSettings();
            EndDialog(hwndDlg, wParam);
            break;
        }
        case ID_COM:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // checking if field is not empty
            {
                EmptyCheck(hwndDlg);
            }
            break;
        }
        case ID_MIRROR:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // checking if field is not empty
            {
                EmptyCheck(hwndDlg);
            }
            break;
        }
        case ID_PAUSE:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // checking if field is not empty
            {
                EmptyCheck(hwndDlg);
            }
            break;
        }
        case ID_ABOUT:
        {
            char msg [290] = "";
            strcat(msg,AppName);
            strcat(msg,"\n\nAstrophoto utility to capture series of images with dithering support");
            strcat(msg,"\n\nCopyright (C) 2018 Anton Vakulenko\nanton.vakulenko@gmail.com\nhttps://github.com/anton-vakulenko/tinyshooter");
            strcat(msg,"\n\nThis program is licensed under the terms of the GNU General Public License version 3");
            MessageBox(hwndDlg,msg,AppName,MB_ICONINFORMATION);
            break;
        }
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // here we process messages from main window
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        // parse config
        LoadSettings();
        // set icon
        HICON hIcon;
        hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(IDI_ICON1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
        if (hIcon)
        {
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        // set title of main window
        SetWindowText(hwndDlg,AppName);
        // set default values to exporuse, frames and initial pause (wait)
        SetDlgItemInt(hwndDlg,ID_EXP,exposure,false);
        SetDlgItemInt(hwndDlg,ID_COUNT,repeat,false);
        SetDlgItemInt(hwndDlg,ID_WAIT,wait,false);
        // count session duration
        TimeLeft = TIMELEFT;
    }
    return TRUE;

    case WM_CLOSE:
    {
        EndDialog(hwndDlg, 0);
    }
    return TRUE;

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
        // open settings dialog
        case ID_SETTINGS_BUTTON:
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETTINGS),  hwndDlg, (DLGPROC)DlgSettings);
            // recal total time (maybe we changed some values in settings window)
            TimeLeft = TIMELEFT;
            Statistics("Idle");
            break;
        }
        // starts or stops shooting process
        case ID_START_BUTTON:
        {
            char s[5];
            GetDlgItemText(hwndDlg, ID_START_BUTTON, s, 6);
            if ( strcmp(s, "Start") == 0 )
            {
                // we want to shoot
                // do cosmetics
                SetDlgItemText(hwndDlg,ID_START_BUTTON, "Stop" );
                EnableWindow( GetDlgItem( hwndDlg, ID_EXP ), FALSE);
                EnableWindow( GetDlgItem( hwndDlg, ID_COUNT ), FALSE);
                EnableWindow( GetDlgItem( hwndDlg, ID_PHD ), FALSE);
                EnableWindow( GetDlgItem( hwndDlg, ID_WAIT ), FALSE);
                EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), FALSE);
                // Set the range and increment of the progress bar (1 step = 1 second)
                SendMessage(GetDlgItem(hwndDlg,ID_PROGRESSBAR), PBM_SETRANGE, 0, MAKELPARAM(0, TimeLeft));
                SendMessage(GetDlgItem(hwndDlg,ID_PROGRESSBAR), PBM_SETSTEP, (WPARAM) 1, 0);

                // try to connect to camera
                if (eos == 1)
                {
                    if (CameraConnect() != EDS_ERR_OK)
                    {
                        // no camera - no shooting
                        Statistics("Idle");
                        goto no_luck;
                    }
                }

                // try to connect to PHD
                if (IsDlgButtonChecked(GetActiveWindow(),ID_PHD) == BST_CHECKED)
                {
                    if (!PHD("Connect"))
                    {
                        // no PHD - no shooting
                        Statistics("Idle");
                        goto no_luck;
                    }
                }

                //open serial port
                if (ComPort("Open"))
                {
                    // everything is ok, we can start ticking every 1 second
                    SetTimer(hwndDlg, 1, 1000, (TIMERPROC) TimerProc);
                }
                else
                {
                    // no port - no shooting
                    Statistics("Idle");
                    goto no_luck;
                }

            }
            else
            {

no_luck:        // we go here if we can't open serial port or connect to camera of connect to phd
                // also we are here if we press stop button

                // we want to stop shooting
                // close serial port
                // we need to know, on which stage of shooting we are (or we can't close com port correctly)
                if (status == "Mirror")
                {
                    // shutter button down
                    EscapeCommFunction(Port, SETRTS);
                    EscapeCommFunction(Port, SETDTR);
                    // wait a bit
                    Sleep(100);
                    // shutter button up
                    EscapeCommFunction(Port, CLRRTS);
                    EscapeCommFunction(Port, CLRDTR);
                }
                if (status == "Capture")
                {
                    // close shutter, mirror down
                    EscapeCommFunction(Port, CLRRTS);
                    EscapeCommFunction(Port, CLRDTR);
                }
                ComPort("Close"); // now serial port is closed correctly

                // disconnect from PHD
                PHD("Disconnect");
                // disconect from edsdk
                EdsTerminateSDK();

                // stop ticking
                KillTimer (GetActiveWindow(), 1);
                // drops all variables for timer procedure to defaults
                status = "Mirror";
                TimeElapsed = 0;
                Frame = 0;
                // do cosmetics
                TimeLeft = TIMELEFT;
                Statistics("Aborted");
                SetDlgItemText(hwndDlg,ID_START_BUTTON, "Start" );
                EnableWindow( GetDlgItem( hwndDlg, ID_EXP ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_COUNT ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_PHD ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_WAIT ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
            }
            break;
        }
        // code below checks if all fields in main window are filled with correct values
        // if not it disables buttons and updates statistics
        case ID_EXP:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // exposure changed, need to recalculate total time
            {
                BOOL bSuccess1, bSuccess2, bSuccess3;
                // update exposure
                exposure = GetDlgItemInt(hwndDlg,ID_EXP,&bSuccess1,false);
                // if value is "0" changes it to "1"
                if (exposure == 0)
                {
                    SetDlgItemInt(hwndDlg,ID_EXP,1,false);
                }
                GetDlgItemInt(hwndDlg,ID_COUNT,&bSuccess2,false);
                GetDlgItemInt(hwndDlg,ID_WAIT,&bSuccess3,false);
                // count session duration
                if (bSuccess1 && bSuccess2 && bSuccess3)
                {
                    // we here because all fields are not empty
                    // recal total time
                    TimeLeft = TIMELEFT;
                    Statistics("Idle");
                    EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), TRUE);
                    EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
                }
                else
                {
                    // ups, at least one field is empty
                    Statistics("Empty");
                    EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), FALSE);
                    EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), FALSE);
                }
            }
            break;
        }
        case ID_COUNT:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // number of frames changed, recalculate total time
            {
                BOOL bSuccess1 = true, bSuccess2 = true, bSuccess3 = true;
                // update repeat
                repeat = GetDlgItemInt(hwndDlg,ID_COUNT,&bSuccess1,false);
                // if value is "0" changes it to "1"
                if (repeat == 0)
                {
                    SetDlgItemInt(hwndDlg,ID_COUNT,1,false);
                }
                GetDlgItemInt(hwndDlg,ID_EXP,&bSuccess2,false);
                GetDlgItemInt(hwndDlg,ID_WAIT,&bSuccess3,false);
                // count session duration
                if (bSuccess1 && bSuccess2 && bSuccess3)
                {
                    // we here because all fields are not empty
                    // recal total time
                    TimeLeft = TIMELEFT;
                    Statistics("Idle");
                    EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), TRUE);
                    EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
                }
                else
                {
                    // ups, at least one field is empty
                    Statistics("Empty");
                    EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), FALSE);
                    EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), FALSE);
                }


                break;
            }
        }
            case ID_WAIT:
            {
                if (HIWORD(wParam) == EN_CHANGE)
                    // ititial wait time changed, recalculate total time
                {
                    BOOL bSuccess1 = true, bSuccess2 = true, bSuccess3 = true;
                    // update wait
                    wait = GetDlgItemInt(hwndDlg,ID_WAIT,&bSuccess1,false);
                    GetDlgItemInt(hwndDlg,ID_EXP,&bSuccess2,false);
                    GetDlgItemInt(hwndDlg,ID_COUNT,&bSuccess3,false);
                    // count session duration
                    if (bSuccess1 && bSuccess2 && bSuccess3)
                    {
                        // we here because all fields are not empty
                        // recal total time

                        //TimeLeft = TIMELEFT;
                        //Statistics("Idle");
                        // I don't know why statistics doesn't work here....
                        // if I ever find why, FormatTime() could be deleted from code...
                        SetDlgItemText(hwndDlg,ID_TIME,(FormatTime(TIMELEFT)).c_str());
                        EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), TRUE);
                        EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
                    }
                    else
                    {
                        // ups, at least one field is empty
                        Statistics("Empty");
                        EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), FALSE);
                        EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), FALSE);
                    }
                }
                break;
            }
        }
        }
        return TRUE;
    }
    return FALSE;
    }


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hInst=hInstance;
    InitCommonControls();
    return DialogBox(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)DlgMain);
}
