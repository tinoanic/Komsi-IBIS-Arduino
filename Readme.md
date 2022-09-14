# Komsi-IBIS-Arduino #

## Inhalt

This Arduino Sketch connects the OMSI Bus Simulator to a LED-Display


## Hardware

I used 4 4x64 LED Matrix Displays in series and mounted them black frame.

You can find the LEDs here: https://www.az-delivery.de/products/4-x-64er-led-matrix-display

You also need an Arduino to drive the LEDs. I used a Arduino Due, but you probably can use an Arduino Nano as well - I did not test it.

## Software

Install the latest Version of KOMSI (ver. 2.4)
Arduino IDE or something similar
You will need the following Libraries in you IDE:

+ MD_Parola
+ MD_MAX72XX
+ TimerOne


## Installation

In th .ino file you will find the SETTINGS-Secion. There you have to adapt all five settings according to your personal setup.

Use your favourite compile and flash tool to upload the sketch to the Arduino board.

Connect the Arduino and open Komsi and Omsi. Please keep in mind that you neer to know the COM-Port you Arduino is connected to.

On Komsi, go to the PRINTER section, choose the right COM-Port and toggle the checkbox to on.





