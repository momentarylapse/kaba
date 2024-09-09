# Building Kaba on Linux

## Preparations

Install libraries (the developer version):
* **required**: gtk4 (or gtk3), zlib
* **optional**: opengl, glfw3, vulkan, fftw3, unwind

### Arch / Manjaro

```
sudo pacman -S gtk4
# optionally more...
```

### Ubuntu

```
sudo apt-get install git make g++ cmake-curses-gui
sudo apt-get install libgtk-4-dev
# optional:
sudo apt-get install libgl-dev libunwind-dev
```

## Building, installing

Assuming you are in the repository's root folder, type
```
mkdir build
cd build
ccmake .. -GNinja
# here, probably press C twice, then G
ninja
sudo ninja install   # optional
```



## Running

To compile and execute a `*.kaba` source code file:
```
kaba FILE
```

For a single line of code from the command line:
```
kaba -c 'print("hi")'
```

