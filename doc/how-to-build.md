# Building Kaba on Linux

## Preparations

Install libraries (the developer version):
* **required**: zlib
* **optional**: unwind
* **optional - useful for packages**: gtk4, opengl, vulkan

### Arch / Manjaro

```
# optionally...
sudo pacman -S gtk4
```

### Ubuntu / Debian

```
sudo apt-get install git make g++ cmake-curses-gui
# optional:
sudo apt-get install libgtk-4-dev libgl-dev libunwind-dev
```

## Building, installing

Assuming you are in the repository's root folder, type
```
mkdir build
cd build
ccmake .. -GNinja
# here, probably press C, set CMAKE_INSTALL_PREFIX to something like ~/.local, press C again, then G
ninja
ninja install
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


## Package manager

Run in the kaba repository:
```
bash tools/initialize-package-manager.sh
```

This will add a repo of [common packages](https://github.com/momentarylapse/common-kaba-packages) to the list of sources, clone and automatically install (copy tp `~/.kaba/packages`) the package manager.

Now you can use `kaba package list` to show available packages and `kaba package install NAME` to install.

