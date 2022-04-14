# hx711-pico-c

## General Description

This is my attempt to implement reading from a HX711 via a Raspberry Pi Pico. It uses the RP2040's PIO state machine feature to be as efficient as possible.

After building, copy `main.uf2` in the build directory to the Raspberry Pi Pico and then open up a serial connection.

I have used [this helpful tutorial](https://paulbupejr.com/raspberry-pi-pico-windows-development/) to setup my Windows environment in order to program the Pico via Visual Studio Code.

![resources/hx711_serialout.gif](resources/hx711_serialout.gif)

The .gif above illustrates the current example code obtaining data from a HX711 operating at 80Hz.
