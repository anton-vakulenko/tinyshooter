# TinyShooter

Simple GUI utility for shooting astrophoto with DSLR cameras. Supports dithering (via [PHD2](https://openphdguiding.org/)) and saving images to PC (Canon EOS only). 

## Getting Started

These instructions will get you a copy of the project up and running on your local machine.

### Prerequisites

#### Hardware

Theoretically any modern DSLR camera with Bulb port (saving to PC works on Canon EOS only). 

RS-232 serial port or USB-to-serial port adapter cable using RTS or DTS line. DIY examples of such cables could be found [here](http://www.beskeen.com/projects/dslr_serial/dslr_serial.shtml).

#### Software

OS Windows XP-10 32-64bit

[PHD2](https://openphdguiding.org/) (for interframe dithering feature only)

Edsdk.dll (32-bit version) from [Canon EDSDK](https://www.didp.canon-europa.com/) (for saving to PC feature only).
You should obtain a copy of Edsdk.dll by yourself applying to Canon Developers Program. I'm not allowed to redistribute this DLL. 

That's all for regular user. For development purposes you should have [CodeBlocks](http://www.codeblocks.org/) (MinGW version) installed.

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
