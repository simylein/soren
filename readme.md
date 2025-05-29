# soren

### rp2040 data sheet

https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf

### install build deps

```sh
brew install --cask gcc-arm-embedded
```

### clone build deps

```sh
git clone git@github.com:raspberrypi/pico-sdk.git
git clone git@github.com:raspberrypi/pico-extras.git
```

### setup cmake deps

```sh
cmake .
```

### compile the code

```sh
make
```

### copy executable

```sh
cp soren.uf2 /Volumes/RPI-RP2
```
