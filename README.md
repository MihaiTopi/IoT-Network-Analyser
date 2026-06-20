# Wi-Fi Congestion Analyzer

## Introduction

The Wi-Fi Congestion Analyser is a small, compact diagnostic device designed to map local wireless traffic. Unlike standard Wi-Fi scanners that only show network names, this project utilizes promiscuous mode to sniff raw data packets in the air. It identifies unique access points, hops through all 13 Wi-Fi channels, and calculates real-time traffic volume for each network, displaying the results on a 7-segment LED display and a detailed Serial report.

## Schematics
<img width="680" height="552" alt="Untitled-layout" src="https://github.com/user-attachments/assets/470f560e-0298-47ca-96c2-45df9da2c436" />

---

## Pre-requisites

- ESP32 Dev Module

- 7-Segment LED Display

- 1x Tactile Push Button

- 1x Resistor

- Breadboard and Jumper Wires

- Arduino IDE with ESP32 Board Manager installed

**NOTE:**

  For this project I decided to stick the whole ESP32 to the breadboard for a cleaner build instead of using female to male jumper wires to connect it to the breadboard (I also did not have any).
  
  For these situations, a wider breadboard is recommended so you do not lose access to one side of the development board. That being said today's project will only use the connections on one side of the ESP32, but for future expansions, it is recommended to use a wider breadboard. I ran into this limitation myself.

---

## Setup Steps

1. Physical Wiring: Connect the 7-segment display and button according to the pin mapping in the schematics above.
2. Code Upload: Flash the script via Arduino IDE (115200 Baud).
3. Initialization: On boot, the display shows a dash (-) and waits for a button press on GPIO 32.

---

## Running The Script

1. After flashing the script using Arduino IDE, we need a place to see the output of from the ESP32.

2. Display:
   - Variant 1: You can simply use the Serial Monitor tab inside Arduino IDE to see the output of the ESP32.
   - Variant 2: Once you have the script flashed onto the ESP32, you can connect your ESP32 to other devices via usb and analyse the congestion of the networks around you by listening to the ESPs data feed. You can do this in Windows Power Shell for example by running these commands:
```
$port = New-Object System.IO.Ports.SerialPort COM3, 115200, None, 8, one
$port.Open()
while($port.IsOpen) { $port.ReadLine() }
```
3. Now that you have a terminal where you can see the results, it is time to let our IoT device do its job. To start the analysis, press the push button.

---

## Output

1. After its network discovery phase, the script will display a list of all surrounding networks and their MAC address and the total networks it discovered. The 7 digit segment on the device will also show the total number of discovered networks.

2. After its sniffing phase, the script will display how many packets destined to each discovered network, it found.
