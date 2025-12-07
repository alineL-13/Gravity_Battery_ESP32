# Gravity Battery

This project is a prototype of a gravity battery, built to demonstrate the process of harvesting energy from potential mass and converting it into electrical energy. The ESP32 reads real-time voltage values generated during the descent of a weight and exposes the data through a local web server.

All measurements are stored in a binary tree data structure, including a timestamp for each reading. The web UI displays a graph showing the voltage evolution over time while the weight is descending.

## Features

- ESP32 microcontroller for real-time data acquisition  
- Voltage measurement from a sensor connected to the generator  
- Lightweight local web server to display live values  
- Binary tree data structure to store readings with timestamps  
- Real-time chart visualization (Voltage × Time)  
- Simple and low-cost prototype architecture  

## Project Context

A gravity battery stores energy by lifting a mass and releases it when the mass descends. During the descent, mechanical energy is converted into electrical energy.  
This prototype focuses on:

- Data acquisition
- Real-time visualization
- Data structure for storage

## Web Interface

Once powered, the ESP32 creates a web server that shows:

- Current voltage (live)
- A graph showing voltage vs. time
- Total samples captured
- Last timestamp

Data is pushed dynamically using JavaScript to draw the curve during the test.


