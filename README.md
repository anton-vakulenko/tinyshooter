# TinyShooter

Simple GUI utility for shooting astrophoto with DSLR cameras. Supports dithering (via [PHD2](https://openphdguiding.org/)) and saving images to PC (Canon EOS only). 

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

Theoretically any modern DSLR camera with Bulb port (saving to PC works on Canon EOS only). 

RS-232 serial port or USB-to-serial port adapter cable using RTS or DTS line. DIY examples of such cables could be found [here](http://www.beskeen.com/projects/dslr_serial/dslr_serial.shtml).

[PHD2](https://openphdguiding.org/) (for interframe dithering feature only)

Edsdk.dll (32-bit version) from [Canon EDSDK](https://www.didp.canon-europa.com/) (for saving to PC feature only)

### Installing

A step by step series of examples that tell you how to get a development env running

Say what the step will be

```
Give the example
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [CodeBlocks 17.12](http://www.codeblocks.org/)
* [TDM-GCC MinGW Compiler](http://tdm-gcc.tdragon.net/)
* [ResEdit 1.6.6](http://www.resedit.net/)

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Authors

[**Anton Vakulenko**](https://github.com/anton-vakulenko) - *Initial work*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details. Portions of this code copyrighted by [Canon](https://www.didp.canon-europa.com/developer/didp/didp_cfg.nsf/webpages/Terms+and+Conditions) and [c_07](https://www.codeproject.com/script/Membership/View.aspx?mid=2600768) (code from c_07 licensed under CC BY-SA 2.5 License)

## Acknowledgments

* Canon for it's SDK and API
* c_07 for tutorial about coding Windows TCP Sockets
* Microsoft for it's [docs.microsoft.com](https://docs.microsoft.com/)
* [Sublihim](https://ru.stackoverflow.com/users/216179/sublihim) for finding stupid bug
* and everyone who helped on forums answering my questions...

