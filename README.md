# VentiladorMecanico

If used for research purposes, please cite this work as:
A. Von Chong et al., "Low-cost, rapidly deployable emergency mechanical ventilators during the COVID-19 pandemic in a developing country: Comparing development feasibility between bag-valve and positive airway pressure designs," 2021 43rd Annual International Conference of the IEEE Engineering in Medicine & Biology Society (EMBC), 2021, pp. 7629-7635, doi: 10.1109/EMBC46164.2021.9630676.

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

A descriptive video of the whole system is available at: https://youtu.be/rELKO5cxeyU (english) and https://youtu.be/uDOBed04stI (spanish)

Componentes utilizados
Cantidad
Electroválvula de acción directa Burkert        1
Electroválvula proporcional Burkert             2
Microcontroladora Teensy 4.1                    1
Sensor de flujo SFM3000                         1
Sensor diferencial de presión SDP811            1
Sensor de presión MPXV4006                      1
Convertidor analogico – digital ADS 1115        1
Convertidor de Nivel Lógico Bidireccional       1
Fuente DC NDR 120 24                            1
Regulador de Voltaje TrippLite                  1
Pantalla Táctil RPI Touch                       1
Filtro Antibacterial Armstrong                  1
Pulmón Inteligente IMT Analytics Rigel Medical  1
Adaptadores de cobre                            3
Tuberías de cobre                               3
Mosfet 15A 400W XY-MOS                          4
Extensores Micro-USB                            1
Extensores USB                                  2
Cable HDMI                                      1
Cable tipo-C                                    1
Odyssey Mini PC                                 1
Tornillos 6-32mm                                100
Tuercas 6-32mm                                  100
Separadores 30+6mm M.2                          15
Tuercas M.2                                     15
Tornillos 6mm M.2                               15
RielDIN                                         1
Soportes impresos en PLA 3D                     30
Acrílico 6mm espesor /cm2                       9000
Mezclador de Oxígeno                            1

Instructions and a video describing the assembly and operation of the mechanical ventilator will be added soon...
