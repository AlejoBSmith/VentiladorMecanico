# VentiladorMecanico

This repository has been created to share the results of the mechanical ventilator developped in Panama, in response to the COVID-19 pandemic.

The mechanical ventilator is comprised by an electronic system, a pneumatic system, an embedded control algorithm, and a Human-Machine-Interface.

1. Electronic System
The circuit board files are held in folder "Circuito". There are 5 files:
PCB_Pressure_Sensor.json is the pcb for the pressure sensor MPX4006 and its conditioning circuit.
PCB_Main.json is the main circuit where the microcontroller is held and connection point between all sensors and actuators.
PCB_Flyback_diodes.json is an auxiliary circuit for adding the flyback diodes for th electrovalves.
PCB_Electrovalves_Sensors.json is the pcb containing the signals and power cables for the electrovalve's activation and the I2C sensors (flow and differential pressure)
PCB_Schematic is the complete system's electric schematic.

2. Pneumatic System

3. Embedded control algorithm
The system is controlled via a Teensy 4.1 microcontroller. It can be programmed with the Arduino IDE available at https://www.arduino.cc/en/software. The Teensy boards need and additional addon for the Arduino IDE, available at: https://www.pjrc.com/teensy/td_download.html.

Code is available at the "Arduino" folder. "Ventilator_main.ino" is the main code while "Variables.h" is the header file for variable declarations and initialization.

Additional libraries needed are:

4. Human-machine-interface (HMI)
A HMI is needed for data visualization and operating values change. Files for the external case are sized for a Raspberry Pi 10.1 Inch Touchscreen (available at: https://www.amazon.com/gp/product/B095R6SXX1/ref=asin_title?ie=UTF8&th=1). A Windows-based single-board computer (Seeed Studio Odyssey - X86J4125864) for interface hosting via a LabVIEW code available in the "LabVIEW" folder).

External case
The files for acrylic laser cutting of the ventilator's case is in the "Cortadora Láser" folder.

Instructions and a video describing the assembly and operation of the mechanical ventilator will be added soon...
