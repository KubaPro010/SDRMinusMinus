# SDRMinusMinus
SDRPlusPlus but does not need to use glfw or fftw3, currently it does do MiriSDR only and server mode, it is made to be server mode, from what i can see: it decreased the file lenght (with gui) from 2.7M to 1.6, chatgpt says its 40.7% diffrence
# Requirements
- a linux pc
- ability to control it
- internet
- git
- a mirisdr sdr, a sdrplay can work as a mirisdr
- libmirisdr-5
- cmake
# how to compile
clone this: `git clone https://github.com/KubaPro010/SDRMinusMinus`<br>
```
cd SDRMinusMinus
mkdir build
cd build
cmake ..
make -j$(nproc)
sudo make install
```
