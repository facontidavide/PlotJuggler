| Platform  | Build Status  |
|---------------------|-----------|
| Windows (Appveyor)  | [![Build status](https://ci.appveyor.com/api/projects/status/mqdmxpt0kf1cf2h3?svg=true)](https://ci.appveyor.com/project/facontidavide59577/plotjuggler)  |
| Ubuntu (Semaphore)  | [![Build Status](https://semaphoreci.com/api/v1/facontidavide/plotjuggler/branches/master/shields_badge.svg)](https://semaphoreci.com/facontidavide/plotjuggler) |
| Ubuntu ROS (Travis) | [![Build Status](https://travis-ci.org/facontidavide/PlotJuggler.svg?branch=master)](https://travis-ci.org/facontidavide/PlotJuggler) |


[![Join the chat at https://gitter.im/PlotJuggler/Lobby](https://badges.gitter.im/PlotJuggler/Lobby.svg)](https://gitter.im/PlotJuggler/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?text=I%20use%20PlotJuggler%20and%20it%20is%20amazing%0D%0A&url=https://github.com/facontidavide/PlotJuggler&via=facontidavide&hashtags=dataviz,plotjuggler,GoROS,PX4)

# PlotJuggler 2.1.7

QT5 based application to display time series in plots, using an intuitive "drag and drop" interface.

It can be used either to:

- load __static data from file__ or 
- connect to live __streaming__ of data.

Its functionality can be easily extended through __plugins__.

To understand what PlotJuggler can do for you, take a look to the following video [PlotJuggler on Vimeo](https://vimeo.com/214389001) 

![PlotJuggler](docs/images/PlotJuggler.gif)


## Supported formats

- CSV
- Rosbags / ROS topics
- ULog (PX4)
- Your custom format... [contact me to know more](https://www.plotjuggler.io/support).

## New in version 2.X

It is now possible to create custom timeseries! Simply write your own
JavaScript function and apply it to one of more existing timeseries.

Many thanks to [@1r0b1n0](https://github.com/1r0b1n0), who developed the
initial version of this feature, and to our first __sponsor__, 
a robotics company that made a donation, but prefers to remain anonymous for the time being.

Watch this video to learn how to use it.

[![Custom timeseries](docs/images/custom_functions.png)](https://vimeo.com/311245098)

## Windows Installer (experimental)

[PlotJugglerInstaller-2.1.7.exe](https://github.com/facontidavide/PlotJuggler/releases/download/2.1.7/PlotJugglerInstaller-2.1.7.exe)

## "Download and Run" (Ubuntu 16.04 Xenial or later)

For those of you that can't wait to get your hands dirty, just download this AppImage:

[PlotJuggler-2.1.5-x86_64.AppImage](https://github.com/facontidavide/PlotJuggler/releases/download/2.1.5/PlotJuggler-2.1.5-x86_64.AppImage).
   
Do not forget to make it executable with the command 

    chmod +x ./PlotJuggler-2.1.5-x86_64.AppImage

## How to build (without ROS plugins)

Clone the repository as usual:

      git clone https://github.com/facontidavide/PlotJuggler.git

The only binary dependency that you need installed in your system is Qt5. 
On Ubuntu the debians can be installed with the command:

    sudo apt-get -y install qtbase5-dev libqt5svg5-dev qtdeclarative5-dev qtmultimedia5-dev libqt5multimedia5-plugins
    
On Fedora:

    sudo dnf install qt5-qtbase-devel qt5-qtsvg-devel qt5-qtdeclarative-devel qt5-qtmultimedia-devel
    
Then compile using cmake (qmake is NOT supported):

     mkdir build; cd build
     cmake ..
     make
     sudo make install
 
 Note: the plugins need to be installed in the same folder of the executable or in __/usr/local/lib/PlotJuggler/__.

## How to build (ROS users)

 The following instructions are for __ROS Kinetic__. Adapt them accordingly if you are using a different version of ROS.
 
 The easiest way to install PlotJuggler is through the command:
 
    sudo apt-get install ros-kinetic-plotjuggler 

Nevertheless, if you want to compile it from source, for instance to try the very latest version on the master branch, 
you __must__ build PlotJuggler using __catkin__, otherwise the ROS related plugins will not be included.

Follow these instructions:

    sudo apt-get install qtbase5-dev libqt5svg5-dev ros-kinetic-ros-type-introspection 
    mkdir -p ws_plotjuggler/src; cd ws_plotjuggler/src
    git clone https://github.com/facontidavide/PlotJuggler.git
    cd ..
    catkin_make
    source devel/setup.bash
    
You should see the following message at the beginning of the compilation step:

    "PlotJuggler is being built using CATKIN. ROS plugins will be compiled"

Both the executable and the plugins will be created in __ws_plotjuggler/devel/lib/plotjuggler__.

To run the application, use the command:

    rosrun plotjuggler PlotJuggler 

# If you like PlotJuggler...

PlotJuggler required a lot of work to be developed; my goal is to build the most
intuitive and powerfull tool to visualize data and timeseries.

If you find PlotJuggler useful, consider making a donation on any of these platforms: 
[BuyMeACoffe](https://www.buymeacoffee.com/plotjuggler), 
[PayPal](https://www.paypal.me/facontidavide) or 
[LiveraPay](https://en.liberapay.com/facontidavide/).

If you use PlotJuggler at work, your company can become a __sponsor__ and support 
the development of those specific features they need.

[Contact me](https://www.plotjuggler.io/support) for more details.
 

