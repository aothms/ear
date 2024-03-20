E.A.R: Evaluation of Acoustics using Ray-tracing
================================================

E.A.R is an application that uses Ray-tracing to conceive an impression of the acoustics of a space that has been modeled in 3d. It is a stand-alone application written in C++, but comes with an exporter interface for [Blender](http://www.blender.org). Originally, it was created to aid architects in designing the auditory experience of their buildings, but it has since then evolved into a more all-round tool, which can find its use in filmmaking and special effects as well. E.A.R is free and open source for your enjoyment.

Installation
------------
Grab the Blender E.A.R add-on from the [downloads section](https://github.com/aothms/ear/downloads). The Blender E.A.R add-on allows you to model your scene, which will get rendered in an included binary application. The addon package comes with binaries for most operating systems, but you can also compile the binary yourself. If you have not installed a recent version of Blender on your system, this would be a good time to do so, please get it from the [Blender website](http://www.blender.org/download/get-blender/).

Place the downloaded E.A.R Add-on, which is named render_EAR, in your Blender addons folder. Now you are good to go!

Building E.A.R from source
--------------------------
This guide is directed at **Ubuntu** users, but with minor modifications the concepts apply to other *nix operating systems as well. Windows users can compile the binary using the included Visual Studio project.

Install some prerequisites (don't copy the dollar sign, it simply represents the prompt):

    $ sudo apt-get install build-essential cmake libboost-thread-dev
    
Create a directory in your home folder for E.A.R:  

    $ cd ~
    $ mkdir EAR
    $ cd EAR   
    
Get the [Graphics Math Template Library](http://ggt.sourceforge.net/): 
 
    $ wget -O gmtl-0.6.1.zip http://sourceforge.net/projects/ggt/files/Generic%20Math%20Template%20Library/0.6.1/gmtl-0.6.1.zip/download
    $ unzip gmtl-0.6.1.zip
    $ sudo mv gmtl-0.6.1/gmtl /usr/include/    
    
Now download, build and install E.A.R:

    $ wget -O EAR.zip https://github.com/aothms/ear/zipball/master
    $ unzip EAR.zip
    $ cd aothms-ear*
    $ mkdir build
    $ cd build
    $ cmake ../cmake
    $ sudo make install
    
To test whether everything is installed successfully you can invoke E.A.R from the command line by issuing: 

    $ EAR
    
If everything went well, you should be issued the following welcome message

      ______                      _____  
     |  ____|         /\         |  __ \ 
     | |__           /  \        | |__) | 
     |  __|         / /\ \       |  _  / 
     | |____       / ____ \      | | \ \ 
     |______| (_) /_/    \_\ (_) |_|  \_\
     Evaluation of Acoustics using Ray-tracing
     version 0.1.4b

    Usage:
     EAR render <filename>
     EAR calc T60 <filename>
