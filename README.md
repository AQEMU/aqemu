<div align="center">
  <p>
    <a href="https://github.com/AQEMU/aqemu/">
      <img src="https://avatars.githubusercontent.com/u/161262831" alt="QEMU frontend" />
    </a>
  </p>
</div>

<h1 align="center">
  AQEMU
</h1>

AQEMU - fully 100% opensource cross-platform QEMU virtualization frontend without bloatware and proprietary features.

Alternative:
 - Red Hat Virtual Machine Manager (GNU/GPL 3 license)
 - Oracle VirtualBox (proprietary license)
 - Broadcom VMware (proprietary license)

## Prepare packages

Debian/Ubuntu:

```
sudo apt-get install cmake meson pkg-config qmake6 libqt6qml6 libqt6qmlcore6 libqt6quickcontrols2-6 libqt6core6 libqt6network6 libqt6gui6 libqt6test6 libqt6widgets6 libqt6printsupport6 libqt6dbus6
sudo apt-get install qml-qt6 libqt6qmlcompiler6 qml6-module-qtqml qt6-base-dev qml6-module-qtcore qml6-module-qtquick-controls qml6-module-qttest qt6-declarative-dev libvncclient1 libvncserver-dev qt6-webengine-dev-tools qt6-tools-dev-tools
```

## Building on CMake (GNU/Linux and BSD)
```
mkdir build
cd build
cmake ..
make -j$(nproc --all)
./aqemu
```

## Building on Meson
```
meson build
cd build
ninja
./aqemu
```

## Authors

- Andrey Rijov <[ANDronR@gmail.com](mailto:ANDronR@gmail.com)> ( Original Author, Developer, Author )
- Tobias Gläßer                    ( Maintainer, Developer, from version 0.9.0 and up )
