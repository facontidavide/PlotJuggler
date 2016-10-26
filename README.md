[![Build Status](https://travis-ci.org/facontidavide/PlotJuggler.svg?branch=master)](https://travis-ci.org/facontidavide/PlotJuggler)

# PlotJuggler

[![Join the chat at https://gitter.im/PlotJuggler/Lobby](https://badges.gitter.im/PlotJuggler/Lobby.svg)](https://gitter.im/PlotJuggler/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

QT5 based application to display time series in plots. This is under heavy development. 

To understand what PlotJuggler can do for you, take a look to the following video.

<iframe src="https://player.vimeo.com/video/174120477" width="640" height="360" frameborder="0" webkitallowfullscreen mozallowfullscreen allowfullscreen></iframe> <p><a href="https://vimeo.com/174120477">PlotJuggler: a desktop application to plot time series.</a></p>

![PlotJuggler](/PlotJuggler.gif)

# How to build

First of all you need to clone the repository and its submodules either using the command:

      git clone --recursive https://github.com/facontidavide/PlotJuggler.git

or

      git clone https://github.com/facontidavide/PlotJuggler.git
      git submodule update --init --recursive


The only binary dependencies that you need installed on your system are Boost and Qt5. On Ubuntu the debans can be installed with the command:

    sudo apt-get -y install qtbase5-dev libboost-dev
    
The proceed as you would do with any cmake based project

     mkdir build; cd build
     cmake ..
     make
     sudo make install
 
 Note: you should not skip the last installation step. Currently the plugins need to be installed in the folder __/usr/local/lib/PlotJuggler/__ otherwise PlotJuggle will not find them.
 
# Note for ROS users
 
If you use CATKIN to build this project, the ROS related plugins will be automatically included into the compilation.
You will also need to download and build this package: https://github.com/facontidavide/ros_type_introspection 
