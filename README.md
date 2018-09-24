# TinyShooter

Simple GUI utility for shooting astrophoto with DSLR cameras. Supports dithering (via [PHD](http://www.stark-labs.com/phdguiding.html) or [PHD2](https://openphdguiding.org/)) and saving images to PC (Canon EOS only). 

## Getting Started

These instructions will get you a copy of the project up and running on your local machine.

### Prerequisites

#### Hardware

Theoretically any modern DSLR camera with Bulb port (saving to PC works on Canon EOS only). 

RS-232 serial port or USB-to-serial port adapter cable using RTS or DTS line. DIY examples of such cables could be found [here](http://www.beskeen.com/projects/dslr_serial/dslr_serial.shtml).

#### Software

OS Windows XP-10 32-64bit

[PHD](http://www.stark-labs.com/phdguiding.html) or [PHD2](https://openphdguiding.org/) (for interframe dithering feature only)

EDSDK.dll (32-bit version) from [Canon EDSDK](https://www.didp.canon-europa.com/) (for saving to PC feature only).
You should obtain a copy of EDSDK.dll by yourself applying to Canon Developers Program. I'm not allowed to redistribute this DLL. 

That's all for a regular user. For development purposes you should have [CodeBlocks](http://www.codeblocks.org/) (MinGW version) installed.

### Installing

Unzip archive to any folder (avoid non-English characters it the path):

```
unzip ts.zip
```

## Usage

Run ts.exe:

```
ts.exe
```

Application's main window will appear just after that. If you run TinyShooter for the first time, it's wise to check settings. Press "Settings" button and you'll see application's settings window. There are several items here:

* **Serial port number**. You should enter number of serial port to which shutter cable is connected. Put there only digits (for example "5", not "Com5").

* **Mirror settle**. Mirror lock-up always leads to vibration. With cheap mounts it could be very strong, with high-end mounts - insignificant. Five seconds would be enough for most situations. If you have very stable mount or telescope with very short focal length, you could reduce this value. Please note, zero is not accepted here.

* **Pause**. Duration of pause between frames. Pause reduces thermal noise of DSLR camera. Duration of pause shouldn't be shorter then duration of dithering process. Default value is 30 seconds. Please note, zero is not accepted here.

* **Dither amount**. Optimal value for this parameter is strongly related with focal length of main and guiding scopes and with pixel sizes of main and guiding cameras. Default value is 3 pixels.

* **Download images to PC**. Check this option if you have Canon EOS camera and want to save images directly on your PC. To use this option you have to put EDSDK.dll to folder with TinyShooter. You should obtain a copy of EDSDK.dll by yourself applying to Canon Developers Program. I'm not allowed to redistribute this DLL. 

* **Path to download images from camera**. Press "Browse" button and choose folder to store images. It's better to avoid non-latin characters it the path.

That's all for Settings. Press "Save" button now. Of course, you can change setting again anytime you want. You may notice that "Save" button sometimes is inactive. This happens when you left one or more fields empty. 

Now you are almost ready to start shooting. But before that you should check values in application's main window:

* **Exposure**. Enter the duration of a single frame in seconds. Please note, zero is not accepted here.

* **Repeat**. Enter how many frames do you want to shoot. Please note, zero is not accepted here.

* **Start after**. If you want to delay shooting for some period of time, enter value in *minutes* to this field. This is usefull feature to start session at specific time. To begin session immediately after pressing "Start" button, leave zero in this field (default value).

* **Dither**. Check this box to activate dithering. This feature requires PHD or PHD2 with server enabled.

To start shooting press "Start" button. You may notice that "Start" button sometimes is inactive. This happens when you left one or more fields empty. Just after pressing "Start" button, it's name will change to "Stop". Pressing on it will abort shooting proccess on any stage.

## Built With

* [CodeBlocks 17.12](http://www.codeblocks.org/)
* [TDM-GCC MinGW Compiler](http://tdm-gcc.tdragon.net/)
* [ResEdit 1.6.6](http://www.resedit.net/)

## Authors

[**Anton Vakulenko**](https://github.com/anton-vakulenko) - *Initial work*

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details. Portions of this code copyrighted by [Canon](https://www.didp.canon-europa.com/developer/didp/didp_cfg.nsf/webpages/Terms+and+Conditions) and [c_07](https://www.codeproject.com/script/Membership/View.aspx?mid=2600768) (code from c_07 licensed under CC BY-SA 2.5 License)

## Acknowledgments

* Canon for it's SDK and API
* Microsoft for it's [docs.microsoft.com](https://docs.microsoft.com/)
* [c_07](https://www.codeproject.com/script/Membership/View.aspx?mid=2600768) for tutorial about coding Windows TCP Sockets
* [Sublihim](https://ru.stackoverflow.com/users/216179/sublihim) for finding stupid bug
* CodeBlock and TDM-GCC MinGW Compiler developers
* and everyone who helped on forums answering my questions...
