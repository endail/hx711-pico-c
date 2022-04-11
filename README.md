# hx711-pico-c

## General Description

This is my attempt to implement reading from a HX711 via a Raspberry Pi Pico. It uses the RP2040's PIO state machine feature to be as efficient as possible.

I have used [this helpful tutorial](https://paulbupejr.com/raspberry-pi-pico-windows-development/) to setup my Windows environment in order to program the Pico via Visual Studio Code.

![resources/hx711_serialout.gif](resources/hx711_serialout.gif)

The .gif above illustrates the current example code obtaining data from a HX711 operating at 80Hz. I tapped on the connected load cell to show the change in value.
