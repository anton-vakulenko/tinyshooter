// TODO (av#1#): сделать download неактивным, если не выбрано сохранять в комп
// TODO (av#1#): проверить на актуальность все getactivewindow
// TODO (av#1#): сделать красные цвета
// TODO (av#1#): Объединить formattime & statistics
// TODO (av#1#): не работает статистика при старте!!!!!!!!!!
// TODO (av#1#): один лишний dither в конце съемки

#include <exception>
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdio.h>
#include <shlobj.h> // for choose folder dialog
#include "resource.h"
using namespace std;

// variables for settings
int exposure = 300, repeat = 20, comport = 1, mirror = 5, pause = 30, dither_distance = 4, eos = 0;
bool dither_bool = false;
char download_to[256] = "C:\\";

// varialbles for timer procedure
int TimeElapsed = 0; // number of seconds (ticks) passed
int TimeLeft; // total time left
int Frame = 0; // number of frames taken
string status = "Mirror"; // initial status for timer1_tick procedure

HANDLE Port; // com port
SOCKET phd; //Socket handle

HINSTANCE hInst;
TIMERPROC Timerproc;

bool PHD(string s)
{
    // connect and disconnect to/from PHD2
    if (s == "Connect")
    {
        //Start up Winsock v.2
        WSADATA wsadata;
        WSAStartup(0x0202, &wsadata);
        //Fill out the information needed to initialize a socket
        SOCKADDR_IN target; //Socket address information
        target.sin_family = AF_INET; // address family Internet
        target.sin_port = htons (4300); //Port to connect on
        target.sin_addr.s_addr = inet_addr ("127.0.0.1"); //Target IP
        //Create socket
        phd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        //Try connecting...
        if (connect(phd, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR)
        {
            MessageBox(GetActiveWindow(),"Can't connect to PHD2","TinyShooter",MB_ICONWARNING);
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
// say dither to PHD2
// here we say "dither" to PHD2
// PHD don't understand "," in dither value, so we have to use "."
// it's necessary for non-english languages
// the target format of string is:
// {"method": "dither", "params": [10, false, {"pixels": 1.5, "time": 10, "timeout": 40}], "id": 42} ended by CR LF
// more details here:
// https://github.com/OpenPHDGuiding/phd2/wiki/EventMonitoring

    char msg [100];
    sprintf(msg,"{\"method\": \"dither\", \"params\": [%d, false, {\"pixels\": 1.5, \"time\": 10, \"timeout\": %d}], \"id\": 1}\r\n",dither_distance,pause);

    cout<<msg;
    cout<<msg;
    cout<<msg;

    send(phd,msg,strlen(msg),0);
}

bool ComPort (string s)
{
    if (s == "Open")
    {
        //open serial port
        char com [13];
        sprintf(com,"\\\\.\\COM%d",comport);

        Port = CreateFile(com, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (Port == INVALID_HANDLE_VALUE)
        {
            MessageBox(NULL,"Can't open serial port. Check settings.","TinyShooter",MB_ICONWARNING);
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
            dcb.BaudRate = CBR_9600 ;
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
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
    // check if all fields in settings are filled
    // if not disables Save button

    BOOL bSuccess1, bSuccess2, bSuccess3, bSuccess4;

    GetDlgItemInt(hwnd,ID_COM,&bSuccess1,false);
    GetDlgItemInt(hwnd,ID_MIRROR,&bSuccess2,false);
    GetDlgItemInt(hwnd,ID_PAUSE,&bSuccess3,false);
    GetDlgItemInt(hwnd,ID_DITHER,&bSuccess4,false);

    if (bSuccess1 & bSuccess2 & bSuccess3 & bSuccess4)
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

void Statistics(string t)

{
    // put text at the bottom of the main dialog
    //char line1[55] = "";
    char line1 [55];
    char line2 [39];
    int te, h1, h2, m1, m2, s1, s2;

    te = exposure * repeat; // total exposure

    h1 = te / 3600;
    m1 = (te % 3600) / 60;
    s1 = (te % 3600) % 60;

    h2 = TimeLeft / 3600;
    m2 = (TimeLeft % 3600) / 60;
    s2 = (TimeLeft % 3600) % 60;

    sprintf(line1,"Total exposure: %02d:%02d:%02d     Time left: %02d:%02d:%02d", h1, m1, s1, h2, m2, s2);

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
    // stop ticking
    KillTimer (GetActiveWindow(), 1);
    // drops all variables for timer1_tick procedure to defaults
    status = "Mirror";
    TimeElapsed = 0;
    Frame = 0;
    // diconnect from sdk
    //If blnCameraConnected = True Then
    //    EdsTerminateSDK()
    //    blnCameraConnected = False
    //End If
    // disconnect from PHD
    //If (Not client Is Nothing) Then
    //    client.Close()
    //End If

    // we want to stop shooting
    // close serial port
    EscapeCommFunction(Port, CLRRTS);
    ComPort("Close");
    // do cosmetics
    SetDlgItemText(GetActiveWindow(),ID_START_BUTTON, "Start" );
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_EXP ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_COUNT ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_PHD ), TRUE);
    EnableWindow( GetDlgItem( GetActiveWindow(), ID_SETTINGS_BUTTON ), TRUE);

}

void TimerProc(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4)
{
// main shooting procedure
// at first we should check our status
// it could be mirror, capture and pause

    if (status == "Mirror")
    {
        if (TimeElapsed == 0)
        {
            // shutter button down
            EscapeCommFunction(Port, SETRTS);
            // wait a bit
            Sleep(100);
            // shutter button up
            EscapeCommFunction(Port, CLRRTS);
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
            // this is the end of the single frame shooting
            // now we say "dither" to PHD (if needed)
            if (IsDlgButtonChecked(GetActiveWindow(),ID_PHD) == BST_CHECKED)
            {
                Dither();
            }
        }

        if (Frame + 1 == repeat)
        {
            // all frames are done
            Frame = 0;
            // cosmetics
            TimeLeft = (exposure + mirror + pause)*repeat - pause;
            Statistics("Complete");

//            SetDlgItemText(GetActiveWindow(),ID_TIME,(FormatTime((exposure + mirror + pause)*repeat - pause)).c_str());
//            SetDlgItemText(GetActiveWindow(),ID_STATUS,"Session complete");
//            SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_SETPOS, 0, 0);
            // call this to stop our shooting session
            CleanUp();
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
    //CloseHandle(Port);
    ;
}

string BrowseFolder()
{
    // show browse folder dialog and get selected path
    TCHAR path[MAX_PATH];

    BROWSEINFO bi = { 0 };
    bi.hwndOwner  =  GetActiveWindow();
    bi.lpszTitle  = ("Choose folder to download images from camera");
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );

    if ( pidl != 0 )
    {
        //get the name of the folder and put it in path
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
    // save settings to ts.cfg
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
        MessageBox(NULL,"Configuration file is missing or corrupted. Defaults will be used. Check settings.","TinyShooter",MB_ICONWARNING);
        SaveSettings();
    }
}

BOOL CALLBACK DlgSettings(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
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
        SetDlgItemInt(hwndDlg,ID_DITHER,dither_distance,false);
        SetDlgItemText(hwndDlg,ID_PATH,download_to);
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
            dither_distance = GetDlgItemInt(hwndDlg,ID_DITHER,&bSuccess,false);

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

//shithappens:
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
        case ID_DITHER:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // checking if field is not empty
            {
                EmptyCheck(hwndDlg);
            }
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
    switch(uMsg)
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
        // set default values to exporuse and frames
        SetDlgItemInt(hwndDlg,ID_EXP,exposure,false);
        SetDlgItemInt(hwndDlg,ID_COUNT,repeat,false);
        // parse config :)
        LoadSettings();
        // count session duration
        TimeLeft = (exposure + mirror + pause)*repeat - pause;
        Statistics("Idle");

        //SetDlgItemText(hwndDlg,ID_TIME,(FormatTime((exposure + mirror + pause)*repeat - pause)).c_str());
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
            // recal total time
            TimeLeft = (exposure + mirror + pause)*repeat - pause;
            Statistics("Idle");
            //SetDlgItemText(hwndDlg,ID_TIME,(FormatTime((exposure + mirror + pause)*repeat - pause)).c_str());
            break;
        }
        // start or stop shooting
        // this is main part of code
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
                EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), FALSE);
                TimeLeft = (exposure + mirror + pause)*repeat - pause;
                // Set the range and increment of the progress bar.
                SendMessage(GetDlgItem(hwndDlg,ID_PROGRESSBAR), PBM_SETRANGE, 0, MAKELPARAM(0, TimeLeft));
                SendMessage(GetDlgItem(hwndDlg,ID_PROGRESSBAR), PBM_SETSTEP, (WPARAM) 1, 0);

                // try to connect to PHD2
                if (IsDlgButtonChecked(GetActiveWindow(),ID_PHD) == BST_CHECKED)
                {
                    if (!PHD("Connect"))
                    {
                        // no PHD - no shooting
                        Statistics("Idle");
                        goto no_port;
                    }
                }

                //open serial port
                if (ComPort("Open"))
                {
                    SetTimer(hwndDlg, 1, 1000, (TIMERPROC) TimerProc);
                }
                else
                {
                    // no port - no shooting
                    Statistics("Idle");
                    goto no_port;
                }

            }
            else
            {
                // we want to stop shooting
                // close serial port
                if (status == "Mirror")
                {
                    // shutter button down
                    EscapeCommFunction(Port, SETRTS);
                    // wait a bit
                    Sleep(100);
                    // shutter button up
                    EscapeCommFunction(Port, CLRRTS);
                }
                if (status == "Capture")
                {
                    // close shutter, mirror down
                    EscapeCommFunction(Port, CLRRTS);
                }
                ComPort("Close");
                //CloseHandle(Port);
                // stop ticking
                KillTimer (GetActiveWindow(), 1);
                // drops all variables for timer procedure to defaults
                status = "Mirror";
                TimeElapsed = 0;
                Frame = 0;
                // do cosmetics
                TimeLeft = (exposure + mirror + pause)*repeat - pause;
                Statistics("Aborted");

//                SetDlgItemText(GetActiveWindow(),ID_TIME,(FormatTime((exposure + mirror + pause)*repeat - pause)).c_str());
//                SetDlgItemText(GetActiveWindow(),ID_STATUS,"Session aborted");
//                SendMessage(GetDlgItem(GetActiveWindow(),ID_PROGRESSBAR), PBM_SETPOS, 0, 0);
no_port:        // we go here if opening serial port failed
                SetDlgItemText(hwndDlg,ID_START_BUTTON, "Start" );
                EnableWindow( GetDlgItem( hwndDlg, ID_EXP ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_COUNT ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_PHD ), TRUE);
                EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
            }
            break;
        }
        case ID_EXP:
        {
            if (HIWORD(wParam) == EN_CHANGE)
                // exposure changed, recalculate total time
            {
                BOOL bSuccess1, bSuccess2;
                exposure = GetDlgItemInt(hwndDlg,ID_EXP,&bSuccess1,false);
                GetDlgItemInt(hwndDlg,ID_COUNT,&bSuccess2,false);
                // count session duration
                if (bSuccess1 & bSuccess2)
                {
                    Statistics("Idle");
                    //SetDlgItemText(hwndDlg,ID_TIME,(FormatTime((exposure + mirror + pause)*repeat - pause)).c_str());
                    EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), TRUE);
                    EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
                }
                else
                {
                    Statistics("Empty");
                    //SetDlgItemText(hwndDlg,ID_TIME,"Fill in all data to calculate time!");
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
                BOOL bSuccess1, bSuccess2;
                repeat = GetDlgItemInt(hwndDlg,ID_COUNT,&bSuccess1,false);
                GetDlgItemInt(hwndDlg,ID_EXP,&bSuccess2,false);
                // count session duration
                if (bSuccess1 & bSuccess2)
                {
                    Statistics("Idle");
                    //SetDlgItemText(hwndDlg,ID_TIME,(FormatTime((exposure + mirror + pause)*repeat - pause)).c_str());
                    EnableWindow( GetDlgItem( hwndDlg, ID_START_BUTTON ), TRUE);
                    EnableWindow( GetDlgItem( hwndDlg, ID_SETTINGS_BUTTON ), TRUE);
                }
                else
                {
                    Statistics("Empty");
                    //SetDlgItemText(hwndDlg,ID_TIME,"Fill in all data to calculate time!");
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
