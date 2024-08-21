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
 - Oracle VirtualBox
 - Broadcom VMware
 - Red Hat Virtual Machine Manager

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
