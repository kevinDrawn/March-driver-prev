#### PRELIMINARY SPECIFICATIONS SUBJECT TO CHANGE

EVKT MagAlpha MagDiff User Guide Rev. 0.1 MonolithicPower.com 1
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.
Preliminary Specifications Subject to Change © 2023 MPS. All Rights Reserved.

# User Guide

## EVKT-MagAlpha-MagDiff


## 10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

- EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com
- Overview Table of Contents
   - Introduction
   - Kit Contents
   - Sensors Supported
   - Features and Benefits
   - Kit Specifications
- Section 1. Hardware
   - PCB Overview
   - Schematic
   - Bill of Materials
   - Mechanical Drawing
- Section 2. Software
   - Installation
   - Application Layout
   - Start-Up Page (Home Page)
   - Angle Page
   - Settings
   - Registers
   - Fields
   - Memory
   - Parameters
   - Measures
   - Common Errors
   - No EVKT Board Found........................................................................................................................
   - No Compatible Sensor Found
   - Updating the Firmware
   - Calibration and Correction Table Generation
- Section 3. Ordering Information


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 3
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## Overview

### Introduction

The EVKT-MagAlpha-MagDiff is a communication kit for the MagAlpha and MagDiff magnetic position
sensor family (see Figure 1). The EVKT-MagAlpha-MagDiff offers a seamless connection and operation
with all MagAlpha and MagDiff test boards (TBMA). The EVKT-MagAlpha-MagDiff kit contains a
microcontroller (MCU) board, two flat ribbon cables, and a USB cable. It provides an easy and convenient
way to read the sensor’s angle and read/write to all configuration registers. Using this kit, it is also possible
to compute and configure the angle correction table to further improve the sensor accuracy.

```
Figure 1 : Overview of the EVKT Board
```
Figure 2 shows the EVKT-MagAlpha-MagDiff.

```
Figure 2: The EVKT-MagAlpha-MagDiff
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 4
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### Kit Contents

```
# Part Number Item Quantity
1 EVKT-MA-RP-PICO-V2 MagAlpha and MagDiff evaluation board with MCU 1
2 Ribbon cable 8 conductors, flat ribbon cable 2
3 USB cable USB cable, A Male to Micro B Male 1
4 Online resources Include GUI, python library, and supplemental documents -
```
```
Figure 3 : Kit Contents
```
### Sensors Supported

```
Sensor Generation Sensor Part Number
```
```
MA GEN 3
```
```
MA102, MA302, MA310, MA330, MA702, MA704, MA710, MA730, MA731, MA732,
MA735, MA800, MA820, MA850, MAQ430, MAQ470, MAQ473, MAQ800, MAQ820,
MAQ850, MAP790-xxxx, MAP791-xxxx, MP
MA GEN 4 MA780, MA781, MA782, MA734, MA73 6
MA GEN 6 MA
```
```
MA GEN 7 MA900GQE-S (SPI), MA900GQE-C (I
```
(^2) C), MAQ79 010 FSGQE-S (SPI),
MAQ79010FSGQE-C (I^2 C)

### Features and Benefits

- USB Interface
- RP2040: 32 - Bit Dual ARM Cortex-M0+ Microcontroller (MCU) from Raspberry Pi
- Connectors Compatible with MagAlpha and MagDiff Tests Boards (TBMA)
- Supports Serial Peripheral Interface (SPI) and I^2 C Interface
- Connection for a Reference Optical Encoder (Supports 5V or 3.3V) with Footprints to Add Pull-Up
    Resistors or RC Low-Pass Filters If Needed
- Correction Table Computation and Configuring when Used Alongside the GUI or Python Library


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 5
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

Figure 4 shows an overview of the kit. The components are listed below:

1. Sensor’s SPI connector
2. Sensor’s I^2 C connector
3. Sensor’s ABZ/SSI/UVW/PWM connector
4. Reference encoder connector header
5. Reference encoder supply voltage selection (5V by default)
6. Reference encoder with optional pull-up resistors and low-pass filters
7. USB connector
8. Raspberry Pi Pico (SC0915 or SC0916)
9. RP2040 32-bit MCU

```
Figure 4 : Kit Feature Overview
```
### Kit Specifications

```
Features Specifications
Supply for Test Board Board 3. 3 V
Operating Input Voltage 3.3V
Operating Systems Supported Windows 10 , or later
System Requirements Minimum 351 MB free
EVB Size (LxW) 9 cmx4cm
```
7 2 1

3

4

9

6 5

8


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 6
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## Section 1. Hardware

### PCB Overview

```
Figure 5 : PCB Overview
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 7
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### Schematic

```
Figure 6 : Schematic
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 8
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### Bill of Materials

```
Designator Description Manufacturer Manufacturer PN
CN1, CN2, CN3 WR-MM female SMT connector Wurth 690367280876
J1, J2 2.54mm (100”) pitch, 20 - pin SMT header Samtec TSM- 120 - 01 - F-SV
J3 2.54mm (100”) pitch, 8 - pin SMT header Samtec TSM- 108 - 01 - F-SV
J4 2.54mm (100”) pitch, 5 - pin SMT header Samtec TSM- 105 - 01 - F-SV
R1, R2, R8, R10,
R11, R12 0Ω resistor, 0603^ Yageo^ RC0603JR-070RL^
R3, R4 1.5kΩ resistor, 0603 Vishay CRCW06031K50FKTA
U2 Raspberry Pi Pico Raspberry Pi SC0915 or SC
U3, U4, U5 Dual supply translating transceiver Nexperia 74LVC1T45GW
```
### Mechanical Drawing

```
Figure 7 : Mechanical Drawing
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 9
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## Section 2. Software

### Installation

To install the software, follow the steps below.

1. Launch the app installer then click “Next” (see Figure 8).

```
Figure 8 : Start the Installation Wizard Application
```
2. Agree to the license agreement and click on “Next” (see Figure 9).

```
Figure 9 : Installation Wizard (License Agreement)
```
3. Choose the destination folder and click “Next” (see Figure 10). Ensure that there is enough available
    storage for the installation.

```
Figure 10: Installation Wizard (Destination Location)
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 10
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

4. Select whether to add a shortcut on your desktop (see Figure 11).

```
Figure 11 : Installation Wizard (Additional Tasks)
```
5. Click “Install” (see Figure 12).

```
Figure 12 : Installation Wizard (Installation Overview)
```
6. Wait for installation to be complete (see Figure 13).

```
Figure 13 : Installation Wizard (Installing)
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 11
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

7. Click “Finish” to end the installation and launch the application (see Figure 14).

```
Figure 14 : Installation Wizard (Complete)
```
### Application Layout

The application window is divided into different sections (see Figure 15). The sections are explained
below.

1. The center area shows charts and knob visualization.
2. The menu to switch between the different visualizations in the center area.

```
a. Home Page
b. Angle Page
```
3. The dock widgets can be moved, resized, pinned, and stacked on the right, left, top, and bottom side
    of the center area. These widgets can also be made floatable (separate dialog window).

```
Figure 15 : Application Layout
```
## 2 1 3

## 2a

## 2b


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 12
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

The application has the following dock widgets:

1. Measures
2. Settings
3. Registers
4. Fields
5. Memory
6. Parameters

### Start-Up Page (Home Page)

At start-up, a screen appears (see Figure 16). The screen shows a knob and a rolling chart showing the
current angle value in the center area of the window. This page is well-suited for human machine interface
(HMI) applications with a slow rotation speed. It only shows the averaged angle value of the latest
acquisition.

```
Figure 16 : Home Page
```
### Angle Page

Users can switch to the angle page by clicking on the “Angle” button on the menu in the top left corner of
the window. This page is recommended for applications with a higher rotation speed or if the user would
like to visualize each measured angle (see Figure 17 on page 13).


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 13
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Figure 17 : Angle Page
```
### Settings

Figure 18 shows the settings. These settings are described in greater detail below.

```
Figure 18 : Settings
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 14
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

1. Communication: Defines the port used by the communication board and triggers a
    connection/disconnection to the computer. The correct COM port address is automatically detected
    by the application. If a board is already connected at the application’s start-up, the connection is
    automatically triggered, and no action is required from the user to start reading the sensor. An error
    message is displayed if no compatible board is detected. See the No EVKT Board Found section on
    page x for more information.
2. Sensor Generation: Defines the sensor generation connected to the board through the CN1 or CN
    connector. Sensor generation is automatically detected by the application, and no user input is
    required. An error message is displayed if no compatible sensor is detected. See the No Compatible
    Sensor Found section on page x for more information.
3. Sensor Interface: Defines the serial communication interface to use with the connected sensor. The
    application and evaluation kit support both SPI and I^2 C interfaces (most sensors use SPI). The
    interface is automatically detected by the application at start-up. The test board’s flat ribbon cable
    must be connected to the correct connector on the EVKT-MagAlpha-MagDiff. The test board using
    the SPI must be connected on CN1, while the board using the I^2 C interface must be connected on
    CN3. An error message is displayed if no compatible sensor or interface is detected. See the No
    Compatible Sensor Found section on page x for more information.
4. SPI Mode: Defines the clock polarity and clock phase to communicate with the sensor. Refer to the
    relevant sensor’s datasheet for more information.
5. SPI Clock Frequency: Defines the SPI clock frequency used to communicate with the sensor. The
    input frequency might not be the actual frequency used by the MCU. The MCU firmware tries to use
    the closest frequency available by using the available system clocks and frequency dividers. Users
    are advised to measure the SPI SCLK clock frequency used by the system if a precise value is
    required.
6. I^2 C Clock Frequency: Defines the I^2 C clock frequency used to communicate with the sensor. The
    input frequency might not be the actual frequency used by the MCU. The MCU firmware tries to use
    the closest frequency available by using the available system clocks and frequency dividers. Users
    are advised to measure the I^2 C SCL clock frequency used by the system if a precise value is required.
7. I^2 C Device Address: Defines the I^2 C device address used by the MCU to communicate with the
    sensor. The I^2 C device address of each sensor can be defined by setting some sensor pins to a
    specific logic level at the sensor’s start-up. Refer to the relevant sensor’s datasheet for more
    information.
8. Wait Time Between Samples: Defines the duration of the additional waiting time inserted between
    each angle measurement. The evaluation kit board reads the sensor’s angle output as fast as
    possible. The actual sensor’s sampling rate is influenced by the selected serial interface and its clock
    frequency. This parameter allows users to reduce the angle’s sampling rate. This is especially useful
    for measurements that require at least one full rotation to compute the results. The measurements
    include the angle error, INL, calibration, and harmonics extraction. The median sampling period can
    be read on the Measures dock widget (see Figure 15 on page 11).
9. Number of samples to acquire: Defines the number of angles to acquire from the sensor with each
    new measurement. This value also defines the number of data points that are used to compute the
    noise root mean square (RMS) and the angle’s resolution. The maximum value of this parameter is
    2048 samples. If this value is still too small to capture a full angle rotation, it is recommended to
    increase the wait time between samples.
10. Enable Trigger: Enables the zero-crossing condition to initiate a trigger on the angle output using the
    angle difference defined on the spinbox. When enabled, this feature only displays angular values
    between two zero-crossings. Zero-crossing is defined as the position at which the absolute difference


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 15
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
between two consecutive angle measurements is above or equal to the value defined in the angle
trigger spinbox.
```
11. PPT Reference Encoder: Defines the number of pulses per turn (PPT) (per channel) used by the
    reference encoder. A PPT of 1024 means that the encoder has 2048 edges per channel and 4096
    edges in total on the A and B signals. This parameter is only required when a refence encoder is
    connected and enabled.
12. Enable Reference Encoder: Enables the reference encoder input of the evaluation kit. Once this
    feature is enabled, the MCU reads the reference encoder angle before each sensor angle
    measurement. This allows the application to compute the angle error and perform the angle
    calibration.
13. Magnet Pole Pairs Number: Defines the number of pole pairs of the sensed magnet (also called the
    target magnet). Most set-ups use a single pole pair magnet. This means that a full mechanical rotation
    of the magnet will produce a single magnetic rotation (0° to 360°, measured by the sensor). A two
    pole pairs magnet produces two magnetic rotations for each single mechanical rotation. This
    parameter computes angular errors and performs calibration.
14. Data Acquisition Start, Stop: Allows users to stop and restart data acquisition. Data acquisition
    automatically starts at the application’s start-up if a sensor is detected.
15. Recorded Data: Saves the acquired data into a file.
16. Correction Table:

```
a. Launch Calibration: Performs the sensor calibration to minimize angle errors. This feature
requires a reference encoder to be connected and correctly configured. At least one full
mechanical rotation of the magnet is required to compute the correction table. The calibration
procedure starts by resetting any zero settings and correction table previously written into the
sensor registers, then measures the sensor’s angle output. The algorithm computes the correction
table, writes it into the sensor registers, and stores these registers into the multiple-time
programmable (MTP) memory. This feature is only available for the MA600.
b. Reset Correction Table: Resets the correction table that has been written into the sensor
registers. This feature is only available on the MA600.
c. Save Correction Table: Saves the angle correction table into a file. This feature is available on all
sensors.
```
17. Show Error With Theoretical Correction Table: Displays the preview of the angle error after
    calibration. This feature emulates the correction table. It does not write to any registers. This feature
    is available on all sensors.
18. Show Error Before Calibration: Displays the angle error measured before calibration. This information
    is only available after having performed a sensor calibration.
19. Show Locked Registers: Shows the locked registers and fields on the Registers and Fields dock
    widgets (see Figure 15 on page 11). By default, these registers are hidden.

### Registers

Users can read and write to the register values in decimal, hexadecimal, or binary format. Once the
register value editing is done, the user click “Enter” twice to finalize the write operation (once to finish the
edition process and again to send the entered value to the sensor).

Click “Read Registers” to read all sensor registers (see Figure 19 on page 16).


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 16
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Figure 19 : Registers
```
### Fields

Users can write field values by editing the value of the target field. Additional information about each field
can be displayed by hovering the mouse cursor on top of the desired field.

Click “Read Fields” to read all the sensor fields (see Figure 20).

```
Figure 20 : Fields
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 17
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### Memory

The Memory panel allows users to control the sensor’s non-volatile memory (NVM) storing and
restoration operation (see Figure 21). The available features depend on the generation of the sensor
(generation 1, generation 2, and so on) connected to the evaluation kit.

```
Figure 21 : Memory
```
### Parameters

This panel provides simple and convenient access to some of the sensor’s parameters and serves as an
alternative to the Registers and Fields panels. It allows users to modify the current angle value as the
new zero, or to set the zero and BCT settings using sliders (refer to the related sensor datasheet for more
information).

The available features depend upon the generation of the sensor connected to the evaluation kit.

```
Figure 22 : Parameters
```
### Measures

This panel shows all measurements performed by the application. In addition to the angle value (in
degrees) and the LSB, it also shows the noise RMS and resolution. If a reference encoder is connected
to the evaluation kit board, this section can also display the angle error, its harmonics decomposition,
offset, and INL (see Figure 23 on page 18).


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 18
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Figure 23 : Measures
```
### Common Errors

This section describes the most common issues that could be encountered by the users.


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 19
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### No EVKT Board Found........................................................................................................................

This error message is displayed if the application is not able to detect a compatible evaluation kit board
(see Figure 24).

Usually, this error is caused by an incorrect USB connection or a defective USB port on the host
computer. In some cases, this issue can be solved by using a shorter USB cable.

```
Figure 24 : No EVKT Board Found Error
```
### No Compatible Sensor Found

This error message is displayed if no compatible sensor is found or if no sensor is connected to the
evaluation kit board.

This error is usually caused by an incorrect connection related to the flat ribbon cable connecting the
evaluation kit to the sensor’s test board. Check the cable connection to the CN1 (for the SPI sensor) or
CN3 (for I^2 C sensor) connectors.

This error might also be caused by a newly released sensor that is not yet supported by the current
evaluation kit firmware. It is recommended to download the latest version of the application and update
the evaluation kit firmware with the latest available version within the application.

See the Updating the Firmware section on page 20 for more instructions.

```
Figure 25 : No Compatible Sensor Found Error
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 20
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### Updating the Firmware

To update the firmware on the Raspberry Pi Pico board, follow the step below:

1. Disconnect the USB cable from the Raspberry Pi Pico.
2. Press the BOOTSEL button and hold it while reconnecting the USB cable between the Raspberry Pi
    Pico and the host computer (see Figure 26).
3. The Raspberry Pi Pico should be recognized as a USB mass storage device by the host computer.
    The BOOTSEL button can be released.
4. Drag and drop the firmware to upload the firmware to the new USB mass storage device.
5. The firmware is automatically uploaded, and the device should restart in normal operating mode once
    the update is complete.

```
Figure 26 : BOOTSEL Button on the Raspberry Pi Pico
```
The firmware can be found in the application installation folder. An example of the installation path is
below.

```
C:\Users\username\AppData\Local\Programs\MPS\MagAlpha-Evaluation-App\bin\ressources\firmware
```
### Calibration and Correction Table Generation

To perform the sensor calibration and/or generate a correction table, the following items are required:

- A motor rotating the set-up to calibrate at a constant speed. At least one full rotation should be
    measured.
- A high-accuracy reference encoder (typically an optical encoder) connected to the evaluation kit. It is
    recommended to use an encoder with 1024 PPT or higher.

To access the calibration feature, follow the steps below:

1. Click on “Angle Page” on the menu.
2. Display the “Settings” panel.
3. Set the number of PPT used by the reference encoder in the settings panel.
4. Set the number of pole pairs of the magnet measured by the sensor.

```
a. If a multiple pole pairs magnet is used, the calibration algorithm computes the average error for a
full mechanical turn. This may limit the accuracy of the calibrated system, especially if the pole
pairs have a different angle error (asymmetrical).
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 21
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

5. Set the number of samples to acquire to have enough data points to be able to compute the angle
    error accurately. The maximum number of samples supported is 2048. It is recommended to start by
    using 2048 samples. It is possible to reduce the number of samples used to optimize the calibration
    time.
6. Enable the Trigger checkbox. The default trigger value of 100° is sufficient for most set-ups.
7. Set the wait time between samples to observe at least one full mechanical rotation. This value
    depends on the magnet’s rotation speed and of the SPI or I^2 C clock speed. For example, a slower
    clock speed reduces the sampling rate.
8. Enable “Enable Reference Encoder”.
9. Set the zero settings of the sensor to 0.
10. Click “Save Correction Table” to save the correction table into an Excel or CSV file.

```
a. On the MA600, which has an embedded correction table, it is also possible to click “Launch
Calibration”. This will reset the zero settings of the sensor to 0 and reset any previously written
correction table. Then the calibration algorithm will acquire a new set of angle data, compute the
error using the reference encoder, and generate the correction table. This correction table is
written on the registers and finally stored in the sensor’s MTP.
```
Other features are listed below:

- It is possible to compare the charts before and after calibration by enabling the “Show Error Before
    Calibration” checkbox.
- The correction table for the MA600 can be reset manually by clicking “Reset Correction Table”. This
    does not reset the value stored in the MA600’s MTP, and it only resets the register values.


EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 22
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Figure 27 : Before MA600 Calibration
```
```
Figure 28 : After MA600 Calibration
```

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 23
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## Section 3. Ordering Information

```
Part Number Description
EVKT-MagAlpha-MagDiff Complete evaluation kit
Boards Compatible with the Evaluation Kit
TBMAxxx-Q-LT All angle position sensor test boards
Online resources Include GUI and supplemental documents
```
```
Order directly from MonolithicPower.com or our distributors.
```

Notice: The information in this document is subject to change without notice. Please contact MPS for current specifications.
Users should warrant and guarantee that third-party Intellectual Property rights are not infringed upon when integrating MPS
products into any application. MPS will not assume any legal responsibility for any said applications.

EVKT-MagAlpha-MagDiff User Guide Rev. 1. 0 MonolithicPower.com 24
10/24/2023 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## REVISION HISTORY

```
Revision # Revision Date Description Pages Updated
1.0 10 / 24 / 2023 Initial Release -
```

