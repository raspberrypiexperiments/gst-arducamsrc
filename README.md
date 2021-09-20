# gst-arducamsrc

## Overview

Some ArduCAM camera boards (e.g. [B0162](https://www.arducam.com/product/arducam-ov9281-mipi-1mp-monochrome-global-shutter-camera-module-raspberry-pi/)) are avaialable with 1-lane MIPI configuration where the only software interface avialable is closed source Arducam SDK. This repository contains **arducamsrc** plugin for GStreamer that wraps up Arducam SDK.

## Dependencies

This repository depends on:
* [MIPI_Camera](https://github.com/ArduCAM/MIPI_Camera.git).

## Known supported Raspberry Pi

* Raspberry Pi 4B.

## Installation

Installation procedure:

```bash
git clone https://github.com/raspberrypiexperiments/gst-arducamsrc.git
cd gst-arducamsrc
./autogen.sh --prefix=/usr/local --libdir=/usr/local/lib/arm-linux-gnueabihf/
sudo make install
```

## Uninstalaltion

Uninstallation procedure:

```bash
sudo make uninstall
cd ..
sudo rm -rf gst-arducamsrc
```

## License

MIT License

Copyright (c) 2021 Marcin Sielski <marcin.sielski@gmail.com>
