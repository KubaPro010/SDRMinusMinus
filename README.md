# SDRMinusMinus
SDRPlusPlus but does not need to use glfw, currently it does do MiriSDR only and server mode, it is made to be in server mode, from what i can see: it decreased the file lenght of the core module: (with gui) from 2.7M to (wait for it).....485 kilobytes, chatgpt says its 82.45% diffrence

note that this release does not have a gui mode, you **need** to run it with `-s` to do anything
# Requirements
- a linux pc
- ability to control it
- internet
- git
- a mirisdr sdr, a sdrplay can work as a mirisdr
- libmirisdr-5
- cmake
- libvolk
- fftw3
- zstd
- libusb
# how to compile
clone this: `git clone https://github.com/KubaPro010/SDRMinusMinus`<br>
### For SDRPlay Users
libmirisdr-5 has a sdrplay flavour option, by default its gonna be the default flavour (shocking, right?), so clone flavour, however if something isnt working with the sdrplay then you can go to: `source_modules/mirisdr_source/src` and then open the main.cpp file, at the start you'll have ``//#define SDRPlay``, remove the prefix slashes so its ``#define SDRPlay`` and (re)compile, just from the `make -j`.... (with that too, to not be confused for some reason that confuses me always)
<br><br>
anyways now just navigate to the dir create the go to the build dir initialize cmake and make, make install
```
cd SDRMinusMinus
mkdir build
cd build
cmake ..
make -j$(nproc)
sudo make install
```
