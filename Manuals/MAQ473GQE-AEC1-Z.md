# MAQ

# 9 - Bit to 1 4 - Bit, MagAlpha Automotive

## Angle Sensor with ABZ Incremental

## and PWM Outputs

MAQ473 Rev. 1. 1 MonolithicPower.com 1
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## DESCRIPTION

##### The MAQ473 detects the absolute angular

##### position of a permanent magnet, typically a

##### diametrically magnetized cylinder on a rotating

##### shaft. Fast data acquisition and processing

##### provide accurate angle measurement at speeds

##### from 0rpm to 60, 000 rpm.

##### The MAQ473 supports a wide range of magnetic

##### field strengths and spatial configurations. Both

##### end-of-shaft and off-axis (side-shaft mounting)

##### configurations are supported.

##### The MAQ473 features magnetic field strength

##### detection with programmable thresholds to allow

##### sensing of the magnet position relative to the

##### sensor for creation of functions such as the

##### sensing of axial movements or for diagnostics.

##### The on-chip non-volatile memory provides

##### storage for configuration parameters, including

##### the reference zero-angle position, ABZ encoder

##### settings, and magnetic field detection

##### thresholds.

##### The MAQ473 is AEC-Q100 qualified, and is

##### available in a QFN-16 (3mmx3mm) package.

## FEATURES

#####  9 - Bit to 14 - Bit Resolution Absolute Angle

##### Encoder

#####  Contactless Sensing for Long Lifespan

#####  SPI Serial Interface with Parity Bit for Angle

##### Readout and Chip Configuration

#####  Configuration Programmable Magnetic Field

##### Strength Detection for Diagnostic Checks

#####  Incremental 1 2 - Bit ABZ Quadrature

##### Encoder Interface with Programmable

##### Pulses Per Turn from 1 to 1024

#####  14 - Bit PWM Output

#####  3.3V, 12 mA Supply Current

#####  - 40°C to + 150 °C Operating Temperature

#####  Available in a QFN-16 (3mmx3mm)

##### Package with Wettable Flanks

#####  Available in AEC-Q100 Grade 1

## APPLICATIONS

#####  Automotive

#####  Angle Encoders

#####  Automotive Angle or Speed Sensors

#####  Robotics

```
All MPS parts are lead-free, halogen-free, and adhere to the RoHS directive.
For MPS green status, please visit the MPS website under Quality
Assurance. “MPS”, the MPS logo, and “Simply, Easy Solutions” are
registered trademarks of Monolithic Power Systems, Inc. or its subsidiaries.
```
## TYPICAL APPLICATION


MAQ473 Rev. 1. 1 MonolithicPower.com 2
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### ORDERING INFORMATION

```
Part Number* Package Top Marking MSL Rating
MAQ473GQE-AEC1 QFN-16 (3mmx3mm) See Below 1
```
```
* For Tape & Reel, add suffix -Z (e.g. MAQ473GQE–AEC1-Z).
```
### TOP MARKING

### ____

### BNXY

### LLLL

```
BNX: Product code of MAQ473GQE
Y: Year code
LLLL: Lot number
```
### PACKAGE REFERENCE

(^) TOP VIEW
QFN-16 (3mmx3mm)


MAQ473 Rev. 1. 1 MonolithicPower.com 3
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### PIN FUNCTIONS

```
Pin # Name Description
1 SSD Data out (SSI).
2 A Incremental output.
3 Z Incremental output.
4 MOSI Data in (SPI). The MOSI pin has an internal pull-down resistor.
5 CS Chip select (SPI). The CS pin has an internal pull-up resistor.
6 B Incremental output.
```
```
7 MISO
```
```
Data out (SPI). MISO has an internal pull-down resistor that is enabled when the device is
in a high-impedance state.
8 GND Supply ground.
9 PWM PWM output.
10 TEST Connect to ground.
11 MGL Digital output indicating field strength below the MGLT level.
12 SCLK Clock (SPI). The SCLK pin has an internal pull-down resistor.
13 VDD 3.3V supply.
14 NC No connection. Leave the NC pin unconnected.
15 SSCK Clock (SSI). The SSCK pin has an internal pull-down resistor.
16 MGH Digital output indicating field strength above the MGHT^ level.^
```
```
17
```
```
Exposed
pad
```
```
Recommend not to solder. Leave floating.
```
### ABSOLUTE MAXIMUM RATINGS (1)

##### Supply voltage ............................ - 0.5V to + 4. 6 V

##### Input pin voltage (VI) ................... - 0.5V to +6.0V

##### Output pin voltage (VO) ............... - 0.5V to +4.6V

##### Continuous power dissipation (TA = 25°C) (2)

##### .................................................................. 2.0W

##### Junction temperature ............................... 160 °C

##### Lead temperature .................................... 260°C

#### Storage temperature ................ - 65°C to + 160 °C

#### ESD Ratings

##### Human body model (HBM) .......................... 2kV

##### Charged device model (CDM) .................... 750 V

#### Thermal Resistance (^3 ) θ JA θ JC

##### QFN-16 (3mmx3mm) ............ 50 ....... 12 ... °C/W

```
Notes:
1) Exceeding these ratings may damage the device.
2) The maximum allowable power dissipation is a function of the
maximum junction temperature TJ (MAX), the junction-to-
ambient thermal resistance θJA, and the ambient temperature
TA. The maximum allowable continuous power dissipation at
any ambient temperature is calculated by PD (MAX) = (TJ
(MAX) - TA) / θJA.
3) Measured on JESD51-7, 4 - layer PCB.
```

MAQ473 Rev. 1. 1 MonolithicPower.com 4
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### ELECTRICAL CHARACTERISTICS

```
Parameter Symbol Condition Min Typ Max Units
Recommended Operating Conditions
Supply voltage VDD 3.0 3.3 3.6 V
```
##### Supply current IDD From -40°C to +125°C 10.2 12 13.8 mA

```
Operating (ambient)
temperature
```
###### TA - 40 + 150 °C

```
Applied magnetic field B 30 60 mT
```

MAQ473 Rev. 1. 1 MonolithicPower.com 5
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### GENERAL CHARACTERISTICS

##### VDD = 3.3V, 45 mT < B < 100mT, TA = - 40 °C to +125°C, unless otherwise noted.

```
Parameter Symbol Condition Min Typ Max Units
Absolute Output (Serial)
```
##### Effective resolution (±3σ)

```
filter window τ = 64μs 9.0 9.8 10.5 bits
filter window τ = 16ms 13.0 13.8 14.5 bits
```
```
Noise root mean square
(RMS)
```
```
filter window τ = 64μs 0.04 0.07 0.12 deg
filter window τ = 16ms 0.003 0.004 0.007 deg
Refresh rate 850 980 1100 kHz
Data output length 16 16 bits
Response Time
```
```
Start-up time (4)
```
```
filter window τ = 64μs 0.6 ms
filter window τ = 16ms 260 ms
Latency (4) Constant speed propagation delay 8 10 μs
```
```
Filter cutoff frequency (4)
```
```
fCUTOFF filter window τ = 64μs 6 kHz
fCUTOFF filter window τ = 16ms 23 Hz
Accuracy
Integral nonlinearity (INL) at
25°C
```
```
At room temperature across the full
field range 0.7^ deg^
INL between -40°C and
+125°C (5)
```
```
Across the full temperature range
and field range
```
```
1.1 deg
```
```
INL at 150°C Across the full field range 1.16 deg
Output Drift
Temperature-induced drift
at room temperature (^5 )
```
```
0.015 deg/°C
```
```
Temperature-induced
variation (^5 )
```
```
From 25°C to 85°C 0.5 deg
From 25°C to 125°C 1.0 deg
Magnetic field induced (^5 ) 0.005 deg/mT
Voltage supply induced (^5 ) 0.3 deg/V
Absolute Output (PWM)
PWM frequency fPWM 840 970 1090 Hz
PWM resolution 13 13.8 14.0 bits
Incremental Output (ABZ)
ABZ update rate 16 MHz
Resolution (edges per turn) Programmable 4 4096
Pulses per channel per turn PPT + 1 Programmable 1 1024
ABZ hysteresis (5) H Programmable 0.08 2.8 deg
```
```
Systematic jitter (5)
```
```
For PPT = 1023, up to 60mT 11 %
```
```
For PPT = 127 7 %
```

MAQ473 Rev. 1. 1 MonolithicPower.com 6
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### GENERAL CHARACTERISTICS (continued)

##### VDD = 3.3V, 45 mT < B < 100mT, TA = - 40 °C to +125°C, unless otherwise noted.

```
Parameter Symbol Condition Min Typ Max Units
Magnetic Field Detection Thresholds
Accuracy (^5 ) 5 mT
Hysteresis (^5 ) MagHys 6 mT
Temperature drift (^5 ) - 600 ppm/°C
Digital I/O
```
##### Input high voltage VIH 2. 5 5.5 V

Input low voltage VIL (^) - 0.3 +0.8 V

##### Output low voltage^ (5) VOL IOL = 4mA 0.4 V

Output high voltage^ (5) VOH IOH = 4mA (^) 2.4 V
Pull-up resistor RPU 46 66 97 kΩ
Pull-down resistor RPD 43 55 97 kΩ
Rising edge slew rate^ (4) tR CL = 50pF 0.7 V/ns
Falling edge slew rate (4) tF CL = 50pF 0.7 V/ns
Notes:
4) Guaranteed by design.
5) Guaranteed by characterization.


MAQ473 Rev. 1. 1 MonolithicPower.com 7
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### TYPICAL CHARACTERISTICS

##### VDD = 3.3V, TA = 25°C, unless otherwise noted.

##### ABZ Jitter

##### PPT = 255, τ = 1ms Current Consumption^

##### Filter Transfer Function

```
τ^ = 16ms^
```
##### Error Curve

##### 50mT

##### Nonlinearity (INL and

##### Harmonics)

##### Effective Resolution

##### 3σ

##### Noise Spectrum

```
50mT and τ = 16ms
```

MAQ473 Rev. 1. 1 MonolithicPower.com 8
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### FUNCTIONAL BLOCK DIAGRAM

```
Figure 1 : Functional Block Diagram
```

MAQ473 Rev. 1. 1 MonolithicPower.com 9
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### OPERATION

##### Sensor Front End

##### The magnetic field is detected with integrated

##### Hall devices located in the center of the package.

##### The angle is measured using MPS’s proprietary

##### SpinAxisTM method, which directly digitizes the

##### direction of the field without the need for complex

##### arctangent computation or feedback loop-based

##### circuits (interpolators).

##### The SpinAxisTM method is based on phase

##### detection, and generates a sinusoidal signal with

##### a phase that represents the angle of the

##### magnetic field. The angle is then obtained by a

##### time-to-digital converter, which measures the

##### time between the zero crossing of the sinusoidal

##### signal and the edge of a constant waveform (see

##### Figure 2). The time-to-digital is outputted from

##### the front end to the digital conditioning block.

##### Start Stop

```
Figure 2 : Phase Detection Method across the
Sine Waveform (Top) and Time-to-Digital
Converter Clock (Bottom)
```
##### The output of the front end delivers a digital

##### number proportional to the angle of the magnetic

##### field at the rate of 1MHz in a straightforward,

##### open-loop manner.

##### Digital Filtering

##### The front-end signal is further treated to achieve

##### the final effective resolution. This treatment does

##### not add any latency in steady conditions. The

##### filter transfer function can be calculated with

##### Equation (1):

(^2)

#### 1+ 2τs

#### H(s) =

#### (1+τs)

##### (1)

##### Where τ is the filter time constant related to the

##### cutoff frequency by: τ = 0.38 / fCUTOFF.

##### See the General Characteristics section on page

##### 5 for the value of fCUTOFF.

##### Sensor Magnet Mounting

##### The MAQ473’s sensitive area (where the Hall

##### devices are placed) is confined within a region

##### less than 100μm wide and has multiple

##### integrated Hall devices. This volume is located

##### both horizontally and vertically within 50μm of

##### the center of the QFN package. The sensor

##### detects the angle of the magnetic field projected

##### in a plane parallel to the package’s upper

##### surface. This means that the only relevant

##### magnetic field is the in-plane component (X and

##### Y components) in the mid-point of the package.

##### By default, when looking at the top of the

##### package, the angle increases when the magnetic

##### field rotates clockwise. Figure 3 shows the zero

##### angle of the unprogrammed sensor, where the

##### plus sign indicates the sensitive point. Both the

##### rotation direction and the zero angle can be

##### programmed.

```
Figure 3 : Detection Point and Default Positive
```
#### Direction

##### This type of detection provides flexibility for

##### angular encoder design. The sensor only

##### requires the magnetic vector to lie within the

##### sensor plane with a field amplitude of at least

##### 30 mT. The MAQ473 can work with fields smaller

##### than 30mT, but the linearity and resolution

##### performance may deviate from the

##### specifications. The most straightforward

##### mounting method is to place the MAQ473 sensor

##### on the rotation axis of a permanent magnet (e.g.

##### a diametrically magnetized cylinder) (see Figure

##### 4 on page 10).


MAQ473 Rev. 1. 1 MonolithicPower.com 10
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### The recommended magnet is a Neodymium

##### alloy (N35) cylinder with dimensions Ø5x3mm,

##### inserted into an aluminum shaft with a 1.5mm air

##### gap between the magnet and the sensor

##### (surface of package). For good linearity, position

##### the with a precision of 0.5mm.

```
Figure 4 : End-of-Shaft Mounting
```
##### If the end-of-shaft position is not available, the

##### sensor can be positioned away from the rotation

##### axis of a cylinder or ring magnet (see Figure 5 ).

##### In this case, the magnetic field angle is not

##### directly proportional to the mechanical angle.

##### The MAQ473 can be adjusted to compensate for

##### this effect and recover the linear relation

##### between the mechanical angle and the sensor

##### output. With multiple pole pair magnets, the

##### MAQ473 indicates multiple rotations for each

##### mechanical turn.

```
Figure 5 : Side-Shaft Mounting
```
##### Electrical Mounting and Power Supply

##### Decoupling

##### It is recommended to place a 1μF decoupling

##### capacitor close to the sensor with a low-

##### impedance path to GND (see Figure 6 ).

```
Figure 6 : Supply Decoupling Connection
```
##### In general, the MAQ473 works well with or

##### without the exposed pad connected. For

##### optimum electrical, thermal, and mechanical

##### conditions, it is recommended that the exposed

##### pad be connected to ground.

##### Serial Interface

##### The sensor supports the serial peripheral

##### interface (SPI) standard for angle reading and

##### register programming. Alternatively, the

##### synchronous serial interface (SSI) bus can be

##### used for angle reading (programming through

##### the SSI is not supported).

##### The data length is 16 bits. For checking the

##### integrity of the data received (angle or register

##### content) the master sends a 17th clock count

##### and receives a parity bit.

#### SPI

##### The SPI is a four-wire, synchronous, serial

##### communication interface. The MAQ473 supports

##### SPI Mode 3 and Mode 0 (see Table 1 and Table

##### 2 ). The SPI mode (0 or 3) is detected

##### automatically by the sensor, and therefore does

##### not require any action from the user. The

##### maximum clock rate supported on the SPI is

##### 25MHz. There is no minimum clock rate. Real-

##### world data rates depend on the PCB layout

##### quality and signal trace length. See Figure 7 and

##### Table 3 on page 11 for SPI timing.

```
Table 1 : SPI Specification
Mode 0 Mode 3
SCLK Idle State Low High
Data Capture On SCLK rising edge
Data Transmission On SCLK falling edge
CS Idle State High
Data Order MSB first
```
```
Table 2 : SPI Standard
Mode 0 Mode 3
CPOL 0 1
CPHA 0 1
Data Order (DORD) 0 (MSB first)
```
##### All commands to the MAQ473 (whether for

##### writing or reading register content) must be

##### transferred through the SPI MOSI pin and must

##### be 16 bits long.


MAQ473 Rev. 1. 1 MonolithicPower.com 11
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### See the SPI Communication section on page 12

##### for details.

```
Figure 7 : SPI Timing Diagram
```
```
Figure 8 : Minimum Idle Time
```
```
Table 3 : SPI Timing
```
```
Parameter (6) Description^ Min^ Max^
```
```
tIDLE_ANGLE Idle time between two subsequent angle transmissions 150 ns -
tIDLE_REG Idle time before and after a register readout 750 ns -
```
```
tNVM
```
```
Idle time between a write command and a register readout
(delay necessary for non-volatile memory update) 20 ms^ -^
```
```
tCSL Time between the CS falling edge and SCLK falling edge 80 ns -
tSCLK SCLK period 40 ns -
tSCLKL Low level of SCLK signal 20 ns -
tSCLKH High level of SCLK signal 20 ns -
tCSH Time between SCLK rising edge and CS rising edge 25 ns -
tMISO SCLK setting edge to data output valid - 15 ns
tMOSI Data input valid to SCLK reading edge 15 ns -
```
Note:

6) Guaranteed by design.


MAQ473 Rev. 1. 1 MonolithicPower.com 12
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### SPI Communication

##### The sensor supports three types of SPI

##### operation:

#####  Read angle

#####  Read configuration register

#####  Write configuration register

##### Each operation has a specific frame structure,

##### described below.

##### SPI Read Angle

##### Every 1μs, new data is transferred into the output

##### buffer. The master device triggers the reading by

##### pulling CS low.

##### When a trigger event is detected, the data

##### remains in the output buffer until the CS signal is

##### de-asserted (see Table 4).

```
Table 4 : Sensor Data Timing
Event Action
```
```
CS falling edge Start reading and freeze the
output buffer
CS rising edge Release the output buffer
```
##### Figure 9 shows a diagram of a full SPI angle

##### reading. Figure 10 shows a partial SPI angle

##### reading. A full angle reading requires 16 clock

##### pulses. The sensor MISO line returns:

(^) MSB LSB
MISO Angle[15:0]
MOSI 0

##### The MagAlpha family has sensors with different

##### features and levels of resolution. See the data

##### output length section in the General

##### Characteristics section on page 5 for the number

##### of useful bits delivered at the serial output. If the

##### data length is less than 16, the rest of the bits

##### sent are 0s. For example, a data output length of

##### 12 bits means that the serial output delivers a 12-

##### bit angle value with 4 bits of 0s padded at the end

##### (the MISO state remains 0 ). If the master sends

##### 16 clock counts, the MAQ473 replies with:

(^) MSB LSB
MISO Angle[15:4] 0 0 0 0
MOSI 0

##### Angle reading can be therefore optimized

##### without any loss of information by reducing the

##### number of clock counts. In the case of a 12-bit

##### data output length, only 12 clock counts are

##### required to get the full sensor resolution.

(^) MSB LSB
MISO (^) Angle[15:4]
MOSI 0

##### If less resolution is needed, the angle can be

##### read by sending even fewer clock counts (since

##### the MSB is first).

##### In case of fast reading, the MAQ473 continues

##### sending the same data until the data is

##### refreshed. See the refresh rate section in the

##### General Characteristics section on page 5.

```
Figure 9 : Full 16-Bit SPI Angle Reading Diagram
```
```
Figure 10 : Partial 8-Bit SPI Angle Reading
Diagram
```

```
MAQ473 Rev. 1. 1 MonolithicPower.com 13
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.
```
##### SPI Read Register

##### A read register operation consists of two 16-bit

##### frames. The first frame sends a read request,

##### which contains the 3 - bit read command (010)

##### followed by the 5 - bit register address. The last 8

##### bits of the frame must all be set to 0. The second

##### frame returns the 8 - bit register value (MSB byte).

##### First 16-bit SPI frame (read request):

MSB LSB

MISO Angle[15:0]

(^) Command Reg. Address
MOSI 0 1 0 A 4 A 3 A 2 A 1 A 0 0 0 0 0 0 0 0 0

##### Second 16-bit SPI frame (response):

```
Register Value
MISO V 7 V 6 V 5 V 4 V 3 V 2 V 1 V 0 0 0 0 0 0 0 0 0
```
```
MSB LSB
MOSI 0
```
##### Figure 11 shows a complete transmission

##### overview.

##### For example, to get the value of the magnetic

##### level high and low flags (MGH and MGL), read

##### register 27 (bit[ 6 ], bit[ 7 ]) by sending the following

##### first frame:

```
MSB LSB
MISO Angle[15:0]
```
(^) Command Reg. Address
MOSI 0 1 0 1 1 0 1 1 0 0 0 0 0 0 0 0

##### In the second frame, the MagAlpha replies:

(^) Register Value
MISO MGH MGL X X X X X X 0 0 0 0 0 0 0 0
MSB LSB
MOSI 0

##### Figure 12 shows a complete example overview.

```
Figure 11 : Two 16-Bit Frames Read Register Operation
```
```
Figure 12 : Example Read Magnetic Level Flags High and Low (MGH, MGH) on Register 27, Bits[6:7]
```
##### SPI Write Register

##### The Register Map section on page 18 shows the

##### programmable 8 - bit registers. Data written to

##### these registers is stored in the on-chip non-

##### volatile memory (NVM) and reloaded

##### automatically at start-up.


```
MAQ473 Rev. 1. 1 MonolithicPower.com 14
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.
```
##### A write register operation consists of two 16-bit

##### frames. The first frame sends a write request,

##### which contains the 3-bit write command (100),

##### followed by the 5-bit register address and the 8-

##### bit value (MSB first). The second frame returns

##### the newly written register value (acknowledge).

##### The NVM is guaranteed to endure 1,000 write

##### cycles at 25°C.

##### It is critical to wait 20ms between the first and the

##### second frame. This is the time taken to write to

##### the NVM. Failure to implement this wait period

##### results in the register’s previous value being

##### read. Note that this delay is only required after a

##### write request. A read register request or read

##### angle does not require this wait time.

##### The first 16 - bit SPI frame (write request) is:

MSB LSB

MISO Angle[15:0]

Command Reg. Address Register Value

MOSI 1 0 0 A 4 A 3 A 2 A 1 A 0 V 7 V 6 V 5 V 4 V 3 V 2 V 1 V 0

##### The second 16 - bit SPI frame (response) is:

```
Register Value
MISO V 7 V 6 V 5 V 4 V 3 V 2 V 1 V 0 0 0 0 0 0 0 0 0
```
```
MSB LSB
MOSI 0
```
##### The readback register content can be used to

##### verify the register programming. Figure 13

##### shows a complete transmission overview.

##### For example, to set the value of the output

##### rotation direction (RD) to counterclockwise

##### (high), write register 9 by sending the following

##### first frame:

```
MSB LSB
MISO Angle[15:0]
```
```
Command Reg. Address Register Value
MOSI 1 0 0 0 1 0 0 1 1 0 0 0 0 0 0 0
```
##### Send the second frame after a 20ms wait time. If

##### the register is written correctly, the reply is:

(^) Register Value
MISO 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
MSB LSB
MOSI 0

##### See Figure 14 for a complete example.

```
Figure 13 : Overview of Two 16-Bit Frames Write Register Operation
```

MAQ473 Rev. 1. 1 MonolithicPower.com 15
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Figure 14 : Example Write Output Rotation Direction (RD) to Counterclockwise (High), on Register 9, Bit 7
```
##### SSI

##### The SSI is a two-wire, synchronous serial

##### interface for data reading only. The sensor

##### operates as a slave to the external SSI master

##### and only supports angle reading. It is not

##### possible to read or write registers via the SSI.

##### SSI Communication

##### Unlike the SPI, the sensor SSI only supports

##### angle reading. It is not possible to read or write

##### registers using the SSI. Figure 15 and Table 5

##### show the SSI timing communication details.

###### SSD

###### SSCK

```
MSB
```
```
tSSCKH tSSCKL
```
```
tSSCK
```
```
tP
```
```
LSB
```
```
tM
```
```
MSB- 1
```
```
tSSD
```
```
MSB
```
```
Figure 15 : SSI Timing
```
```
Table 5 : SSI Timing
```
```
Parameter Description Min Max
```
```
tSSD Delay between the SSCK rising edge and the start of data transfer - 15ns
tSSCK SSCK period 0. 2 μs 16μs
tSSCKL Low level of the SSCK signal 0. 1 μs 8μs
tSSCKH High level of the SSCK signal 0. 1 μs 8μs
tM Transfer timeout (monoflop time) 25μs -
tP Dead time: SSCK high time for next data reading 40μs -
```
##### SSI Read Angle

##### The bit order of the transmitted data is MSB first

##### and LSB last. Every 1μs, new data is transferred

##### into the output buffer. The master device triggers

##### the reading by driving SSCK high. A full reading

##### requires up to 17 clock counts (see Figure 16).

##### The first clock is a dummy clock to start the

##### transmission. The data length is up to 16 bits

##### long. See the data output length section in the

##### General Characteristics section on page 5 for the

##### number of useful bits delivered at the serial

##### output.


MAQ473 Rev. 1. 1 MonolithicPower.com 16
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### The first MSB are transmitted on the second

##### clock count. If the data length is less than 16 bits,

##### the 16-bit output word is completed by 0s.

##### Therefore, the reading can also be performed

##### with fewer than 16 clock counts. For example, for

##### a part with a 12-bit data length, it is only

##### necessary to send the first dummy clock to start

##### the transmission + 12 clocks to read the angle

##### data.

##### When a trigger event is detected, the data

##### remains in the output buffer until the clock falling

##### edge for the LSB bit = 0 and the transfer timeout

##### time has passed (see Table 6).

```
Table 6 : Sensor Data Timing
Trigger Event Output Buffer Release
First SSCK pulse rising edge SSCK falling edge + timeout tM
```
```
Figure 16 : Diagram of a Full 16-Bit SSI Angle Reading (with First Dummy Clock)
```
##### Figure 17 shows the timing for consecutive angle readings.

```
Figure 17 : Diagram of Two Consecutive 16 - Bit SSI Angle Readings with the Required Dead Time between
the Frames
```
##### Parity Bit

##### The parity bit, or check bit, is added to the output string to ensure that the total number of 1s in the string

##### is even. It is used as error detecting code for angle or register reading. The MAQ473 transmits the parity

##### bit at the 17th clock edge (see Table 7 and Figure 18).

```
Table 7 : Example of Parity Bit
```
```
16 - Bit Output
```
```
Number of
Bits Set to 1 Output with the Parity Bit^
0000000000000000 0 00000000000000000
1000110001100010 6 10001100011000100
0101110100000000 5 01011101000000001
```

MAQ473 Rev. 1. 1 MonolithicPower.com 17
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Figure 18 : SPI Angle Reading with Parity Bit
```

MAQ473 Rev. 1. 1 MonolithicPower.com 18
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### REGISTER MAP

```
Table 8 : Register Map
```
```
# of
Registers
```
```
Hex Binary Bit[^7 ]^
(MSB)
```
```
Bit[ 6 ] Bit[ 5 ] Bit[4] Bit[3] Bit[ 2 ] Bit[1] Bit[^0 ]^
(LSB)
```
```
0 0x0 00000 Z[7:0]
1 0x1 00001 Z[15:8]
2 0x2 00010 BCT[7:0]
3 0x3 00011 - - - - - - ETY ETX
4 0x4 00100 PPT[1:0] ILIP[3:0] - -
5 0x5 00101 PPT[9:2]
6 0x6 00110 MGLT[2:0] MGHT[2:0] - -
9 0x9 01001 RD - - - - - - -
14 0xE 01110 FW[7:0]
16 0x10 10000 HYS[7:0]
27 0x1B 11011 MGH MGL MIR[3:0] - -
```
```
Table 9 : Factory Default Values
```
```
# of
Registers
```
```
Hex Binary
```
```
Bit[ 7 ]
(MSB)
```
```
Bit[ 6 ] Bit[ 5 ] Bit[4] Bit[3] Bit[ 2 ] Bit[1]
```
```
Bit[ 0 ]
(LSB)
```
```
0 0x0 00000 0 0 0 0 0 0 0 0
1 0x1 00001 0 0 0 0 0 0 0 0
2 0x2 00010 0 0 0 0 0 0 0 0
3 0x3 00011 0 0 0 0 0 0 0 0
4 0x4 00100 1 1 0 0 0 0 0 0
5 0x5 00101 1 1 1 1 1 1 1 1
6 0x6 00110 0 0 0 1 1 1 0 0
9 0x9 01001 0 0 0 0 0 0 0 0
14 0xE 01110 0 1 1 1 0 1 1 1
16 0x10 10000 1 0 0 1 1 1 0 0
```

MAQ473 Rev. 1. 1 MonolithicPower.com 19
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### Table 10 : Programming Parameters

```
Parameters Symbol # of Bits Description See Table
Zero setting Z 16 Sets the zero position 11
```
```
Bias current trimming BCT 8 For side-shaft configuration: reduces^ the bias
current of the X or Y Hall device
```
###### 14

```
Enable trimming X ETX 1
```
```
Biased current trimmed in the X-direction Hall
device
```
###### 15

```
Enable trimming Y ETY 1 Biased current trimmed in the Y-direction Hall
device
```
###### 15

```
Pulses per turn PPT 10 Number of pulses per turn of the ABZ output 19
Index length/position ILIP 4 Parametrization of the ABZ index pulse Figure 27
Magnetic field high
threshold
```
```
MGHT 3 Sets the field strength high threshold 16
```
```
Magnetic field low
threshold
```
```
MGLT 3 Sets the field strength low threshold 16
```
```
Rotation direction RD 1 Determines the sensor positive direction 13
Filter window FW 8 Size of the digital filter window 18
Hysteresis HYS 8 ABZ output hysteresis 21
```

```
MAQ473 Rev. 1. 1 MonolithicPower.com 20
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.
```
### REGISTER SETTINGS

##### Zero Setting

##### The MAQ473’s zero position (a 0 ) can be

##### programmed with 1 6 bits of resolution. The angle

##### streamed out by the MAQ473 (aOUT) is calculated

##### with Equation ( 2 ):

#### aOUT= aRAW- a 0 (2)

##### Where aRAW is the raw angle provided by the

##### MAQ473’s front end.

##### The parameter Z[ 1 5:0], which is 0 by default, is

##### the complementary angle of the zero setting. In

##### decimals, it can be calculated with Equation (3):

#### a = 2 - Z[15 : 0] 0 16 (3)

##### Table 11 shows the zero-setting parameter.

```
Table 1 1 : Zero-Setting Parameter
```
```
Z[15:0]
```
```
Zero Position
a 0 (16-Bit Decimal)
```
```
Zero Position
a 0 (deg)
0 65536 360.
1 65535 359.
2 65534 359.
... ... ...
65534 2 0.
65535 1 0.
```
##### Example

##### To set the zero position to 20°, the Z[ 1 5:0]

##### parameter must be equal to the complementary

##### angle, and can be calculated with Equation (4):

```

  

```
```
[15 : 0] 2^162021661895
360
```
##### Z (4)

##### In binary, it is written as 1111 0001 1100 0111.

##### Table 1 2 shows the content of registers 0 and 1.

```
Table 1 2 : Register 0 and 1 Content
```
Reg Bit[ 7 ] Bit[ 6 ] Bit[ 5 ] Bit[ 4 ] Bit[ 3 ] Bit[ 2 ] Bit[ 1 ] Bit[ 0 ]

(^0 1 1 0 0 0 1 1 )
(^1 1 1 1 1 0 0 0 )

##### Rotation Direction

##### By default, when looking at the top of the

##### package, the angle increases when the magnetic

##### field rotates clockwise (CW) (see Figure 1 9 and

##### Table 1 3 ).

```
Figure 19 : Positive Rotation Direction of the
Magnetic Field
```
```
Table 13 : Rotation Direction Parameter
RD Positive Direction
0 Clockwise (CW)
1 Counterclockwise (CCW)
```
##### BCT Settings (Bias Current Trimming)

##### Side-Shaft

##### When the MAQ473 is mounted on the side of the

##### magnet, the relationship between the field angle

##### and the mechanical angle is no longer directly

##### linear. This effect is related to the fact that the

##### tangential magnetic field is usually smaller than

##### the radial field. Calculate the field ratio (k) with

##### Equation (5):

#### k = BRAD/BTAN (5)

##### Where BRAD and BTAN are the maximum radial

##### and tangential magnetic fields (see Figure 20 ).

```
BRAD
```
```
BRAD
```
```
BTAN BTAN
```
```
Figure 20 : Side-Shaft Field
```
##### The k ratio depends on the magnet geometry

##### and the distance to the sensor. Having a k ratio

##### different than 1 results in the sensor output

##### response not being linear with respect to the

##### mechanical angle. Note that the error curve has

##### the shape of a double sinewave (see Figure 2 2

##### on page 21). E is the amplitude of this error.


MAQ473 Rev. 1. 1 MonolithicPower.com 21
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### The X-axis and Y-axis bias currents can be

##### reduced by programming in order to recover an

##### equal Hall signal for all angles, and therefore

##### suppress the error. Parameter ETX and ETY

##### control the direction in which sensitivity is

##### reduced. The current reduction is set by the

##### parameter bias current trimming register,

##### BCT[7:0], which is an integer from 0 to 255.

##### In side-shaft configuration (i.e. the sensor center

##### is located beyond the magnet’s outer diameter),

##### k is greater than 1. For optimum compensation,

##### the sensitivity of the radial axis should be

##### reduced by setting the BCT parameter,

##### calculated with Equation (6):

#### 

#### 

#### 

#### 1

#### BCT[7 : 0] = 258 1

#### k

##### (6)

##### Figure 2 1 shows the optimum BCT value for a

##### particular k ratio.

```
0
```
```
50
```
```
100
```
```
150
```
```
200
```
```
1 1.5 2 2.5 3 3.5 4 4.5 5
```
```
BCT
```
##### k

```
Figure 21 : Relation between the k Ratio and the
Optimum BCT to Recover Linearity
```
##### Table 1 4 shows some typical BCT values.

```
Table 14 : Example of BCT Settings
E (deg) Magnet Ratio k BCT[7:0]
0 1.0 0
11.5 1.5 86
19.5 2.0 129
25.4 2.5 155
30.0 3.0 172
33.7 3.5 184
36.9 4.0 194
39.5 4.5 201
41.8 5.0 207
```
#### Determining k with the MagAlpha

##### It is possible to deduce the k ratio from the error

##### curve obtained with the default BCT setting (BCT

##### = 0). Rotate the magnet more than one

##### revolution and record the output. Next, plot the

##### error curve (the output minus the real

##### mechanical position vs. the real mechanical

##### position) and extract two parameters: the

##### maximum error (E) and the position of this

##### maximum with respect to a zero crossing aM (see

##### Figure 2 2 ). k can be calculated with Equation (7):

```
M
```
```
M
```
#### tan(E + a )

#### k=

#### tan(a )

##### (7)

```
-40
```
```
-20
```
```
0
```
```
20
```
```
40
```
```
0 50 100 150 200 250 300 350
```
```
Error (deg)
```
```
m
```
```
rotor angle (deg)
```
```
aM 2E
```
```
Rotor Angle (deg)
```
```
Error
```
```
(deg
```
```
)
```
```
2 E
```
```
Figure 22 : Error Curve in Side-Shaft
Configuration with BCT = 0
```
##### Table 1 4 provides some examples. Alternatively,

##### the k parameter can be obtained using Figure

##### 23.

```
Figure 23 : Relation between the Error Measured
with BCT = 0 and the Magnet Ratio k
```
```
1
```
```
1.5
```
```
2
```
```
2.5
```
```
3
```
```
3.5
```
```
4
```
```
4.5
```
```
5
```
```
0 5 10 15 20 25 30 35 40
```
```
k
```
```
E (deg)
```

MAQ473 Rev. 1. 1 MonolithicPower.com 22
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### Sensor Orientation

##### From the dot marked on the package, it is

##### possible to know whether the radial field is

##### aligned with sensor coordinate X or Y (see

##### Figure 24 ).

```
Figure 24 : Package Top View with X- and Y-Axes
```
##### Determine which axis needs to be reduced

##### based on the qualitative field distribution around

##### a ring (see Figure 20 on page 20). For example,

##### with the arrangement shown in Figure 24 , the

##### field along the sensor Y direction is tangential

##### and weaker, so the X-axis should be reduced

##### (ETX = 1 and ETY = 0). If both ETX and ETY are

##### set to 1, the current bias is reduced in both

##### directions the same way (i.e. without side-shaft

##### correction) (see Table 1 5 ).

```
Table 15 : Trimming Direction Parameters
ETX Enable Trimming of the X-Axis
0 Disabled
1 Enabled
ETY Enable Trimming of the Y-Axis
0 Disabled
1 Enabled
```
##### Magnetic Field Thresholds

##### The magnetic flags (MGL and MGH) indicate

##### that the magnetic field at the sensor position is

##### out of the range defined by the lower (MGLT)

##### and upper magnetic field thresholds (MGHT)

##### (see Figure 2 5 ).

```
Figure 25 : MGH and MGL Signals as a Function
of the Field Strength
```
##### MagHys, the typical hysteresis on the MGH and

##### MGL signals, is 6 mT. The MGLT and MGHT

##### thresholds are coded on 3 bits and stored in

##### register 6 (see Table 16 ).

```
Table 16 : Register 6
Register 6
Bit[ 7 ] Bit[ 6 ] Bit[ 5 ] Bit[ 4 ] Bit[ 3 ] Bit[ 2 ] Bit[ 1 ] Bit[ 0 ]
MGLT MGHT - -
```
##### The 3-bit MGLT and MGHT values correspond

##### to the magnetic field (see Table 17 ).

```
Table 17 : MGLT and MGHT: Binary to mT
Relationship
```
```
MGLT or
MGHT (8)
```
```
Field Threshold in mT (7)
From Low to High
Magnetic Field
```
```
From High to Low
Magnetic Field
000 26 20
001 41 35
010 56 50
011 70 64
100 84 78
101 98 92
110 112 106
111 126 120
Notes:
7) Valid for VDD = 3.3V. If different, then the field threshold is
scaled by the factor VDD / 3.3V.
8) MGLT can have a larger value than MGHT.
```
##### The MGL and MGH alarm flags are available to

##### be read in register 27 (bit[ 6 ] and bit[ 7 ],

##### respectively), and their logic state is also given

##### at digital output pins 11 and 16.

##### To read the MGL and MGH flags via the SPI,

##### send the 16-bit read command for register 27:

##### Command Reg. Address MSB Value LSB

```
0 1 0 1 1 0 1 1 0 0 0 0 0 0 0 0
```
##### The MAQ47 3 responds with the register 27

##### content in the next transmission:

```
Register 27 [7:0]
MGH MGL x x MG1L MG2L x x
```
##### The MGL and MGH flags’ logic state has no

##### effect on the angle output.

##### MGL Application Note

##### Pulses with a duration of about 1.3μs to 1.5μs

##### appear randomly in the MGL signal. They appear

##### on both the pin and register values (Register 27,

##### bit 6).


MAQ473 Rev. 1. 1 MonolithicPower.com 23
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### These pulses appear around angle values of 44,

##### 138, 224, and 318 degrees (sensor output) or in

##### an interval of ±1.5 degrees around these values.

##### These pulses have an amplitude of 3.3V (VDD).

##### The minimum interval between two pulses is

##### 100μs.

##### MGL Workarounds

##### 1. Invert the MGH signal to replace MGL. The

##### MGL and MGH magnetic thresholds only

##### differ by a small hysteresis (see Table 1 7 on

##### page 22). An inverted MGH signal can be

##### used to replace the MGL output in the

##### application.

##### 2. Read the MGL signal level twice. Using two

##### readings, which must be between 2μs and

##### 100μs apart, allows the user to distinguish

##### erroneous from real transitions. Table 1 8

##### shows examples of different cases.

##### 3. Read register 27 with the SPI and compute a

##### corrected MGL value using MG1L and

##### MG2L. The corrected MGL signal = not

##### (MG1L OR MG2L). This means that the

##### corrected MGL must be set to 1 only when

##### both MG1L and MG2L are equal to 0. See

##### the C implementation below:

##### correctedMGL = !(MG1L | MG2L)

```
Table 18 : MGL Multiple Reading Workaround
```
```
MGL
First
Reading
```
```
MGL Second
Reading (e.g.
20μs After the
First Reading)
```
```
True
MGL
Value
```
```
Case 1 0
```
```
Second reading is
not needed 0
Case 2 1 1 1
Case 3 1 0 0
```
##### Filter Window (FW)

##### The filter window (FW) affects the effective

##### resolution (defined as the ± 3 σ noise interval) and

##### the output bandwidth, which is characterized by

##### fCUTOFF.

##### Table 19 gives the resulting resolution and

##### bandwidth for each window.

```
Table 19 : FW
```
```
FW[ 7 :0] τ (μs)
```
```
Effective
Resolution
at 45mT
(Bits)
```
```
fCUTOFF
(Hz)
```
```
Start-
Up
Time
(ms)
51 64 9.5 6000 0.5
68 128 10 3000 1.1
85 256 10.5 1500 2.5
102 512 11 740 5.5
119
(default)
```
###### 1024 11.5 370 12

###### 136 2048 12 185 26

###### 153 4096 12.5 93 57

###### 170 8192 13 46 123

###### 187 16384 13.5 23 264

##### The time constant (τ) is the parameter entered in

##### the transfer function (1). This allows the user to

##### accurately model the system and analyze the

##### stability of a control loop.

##### ABZ Incremental Encoder Output

##### The MAQ473 ABZ output emulates a 1 2 - bit

##### incremental encoder (such as an optical

##### encoder), providing logic pulses in quadrature

##### (see Figure 26 ). Compared to signal A, signal B

##### is shifted by a quarter of the pulse period. During

##### one revolution, signal A pulses n times, where n

##### is programmable from 1 to 1024 pulses per

##### revolution. The number of pulses per channel

##### per revolution is programmed by setting

##### parameter PPT, which consists of 10 bits split

##### between registers 0x4 and 0x5 (see Table 8 on

##### page 18). The factory default value is 1023.

##### Table 20 describes how to program PPT[9:0] to

##### set the required resolution.

```
Table 20 : PPT
```
```
PPT[9:0]
```
```
Pulses per
Revolution
```
```
Edges per
Revolution
0000000000 1 (min) 4 (min)
0000000001 2 8
0000000010 3 12
0000000011 4 16
... ... ...
1111111100 1021 4084
1111111101 1022 4088
1111111110 1023 4092
1111111111 1024 (max) 4096 (max)
```

MAQ473 Rev. 1. 1 MonolithicPower.com 24
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### For example, to set 120 pulses per revolution

##### (i.e. 480 edges), set PPT to 120 - 1 = 119 (binary:

##### 0001110111). Table 2 1 shows how to set

##### registers 4 and 5.

```
Table 21 : Example PPT Setting for 120 Pulses
```
(^) Bit[ 7 ] Bit[ 6 ] Bit[ 5 ] Bit[ 4 ] Bit[ 3 ] Bit[ 2 ] Bit[ 1 ] Bit[ 0 ]
R4 (^1 1 0 0 0 0 0 0)
R5 (^0 0 0 1 1 1 0 1)
Figure 2 6 : ABZ Output Timing

##### Signal Z (zero or index) rises only once per turn

##### at the zero-angle position.

##### The position and length of the Z pulse is

##### programmable via bits ILIP[3:0] in register 0x 4

##### (see Figure 27 ).

```
Figure 27 : ILIP Parameter Effect on Index Shape
```
##### By default, the ILIP parameter is 0000. The index

##### rising edge is aligned with the channel B falling

##### edge. The index length is half of the A or B pulse

##### length, depending on the user’s selection.

##### ABZ Hysteresis

##### The hysteresis is set by the parameter HYS (see

##### Table 22 on page 24). To avoid spurious

##### transitions (see Figure 28 on page 24), it is

##### recommended that the hysteresis be 12 times

##### greater than the output root mean square (RMS)

##### noise (1σ). Table 2 3 on page 24 shows

##### indications of the 1σ noise

```
Table 22 : HYS
HYS[7:0] Hysteresis (deg)
200 0.08
188 0.14
148 0.18
152 0.36
156 (default) 0.52
116 0.70
120 1.4
124 2.1
84 2.8
```
##### Table 23 : RMS Noise

###### FW(7:0)

```
Effective
Resolution at
45mT (Bits)
```
```
1σ Noise
(deg)
```
```
51 9.5 0.08
68 10 0.06
85 10.5 0.04
102 11 0.03
119 (default) 11.5 0.02
136 12 0.015
153 12.5 0.01
170 13 0.007
187 13.5 0.005
```
```
Figure 28 : Hysteresis of the Incremental Output
```
##### ABZ Jitter

##### The ABZ state is updated at a frequency of

##### 16 MHz, enabling accurate operation up to a very

##### high rpm (above 10^5 rpm).

##### The jitter characterizes how far a particular ABZ

##### edge can occur at an angular position different

##### from the ideal position (see Figure 29 ).

```
Figure 29 : ABZ Jitter
```

MAQ473 Rev. 1. 1 MonolithicPower.com 25
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

##### The measurable jitter is composed by a

##### systematic jitter (i.e. always the same deviation

##### at a given angle, and given in the General

##### Characteristics section on page 5) and a random

##### jitter.

##### The random jitter reflects the sensor noise.

##### Therefore, the edge distribution is the same as

##### the SPI output noise. The random jitter is a

##### function of the rotation speed. At lower speeds,

##### the random jitter is less than the sensor noise.

##### This is a result of the fact that the probability of

##### measuring an edge at a certain distance from the

##### ideal position depends on the number of ABZ

##### updates at this position.

##### PWM Absolute Output

##### This output provides a logic signal with a duty

##### cycle proportional to the angle of the magnetic

##### field. For the PWM frequency (fPWM), see the

##### General Characteristics section on page 5. The

##### duty cycle is bound by a minimum value (1/ 514

##### of the period) and a maximum value (513/514 of

##### the period), so it varies from 1/514 to 513/514

##### with a resolution of 14 bits (see Figure 30 ). The

##### angle can be obtained by measuring the on time.

##### Since the absolute fPWM can vary from chip to

##### chip or with the temperature, accurate angle

##### detection requires measuring the duty cycle (i.e.

##### measuring both the on time (tON) and the off time

##### (tOFF)). The angle can be calculated with

##### Equation (8):

#### 

####  

#### 

```
ON
```
```
ON OFF
```
#### 360 t

#### angle (in ) = 514 - 1

#### 512 t + t

##### (8)

##### Figure 30 shows one period of the PWM signal.

##### The period (t) is 1 / fPWM.

```
Figure 30 : PWM Output Timing, Top Signal = 0°;
Bottom Signal = Full Scale (i.e. 360° (1 - 1/16384))
```
##### Diagnostic Features

##### The following features can be used to determine

##### correct operation:

##### 1. Parity bit on the angle (see the Parity Bit

##### section on page 1 6 ).

##### 2. Magnetic field in range via MIR[3:0] (see

##### below).

##### Magnetic Field in Range

##### Bits MIR[3:0] in register 27 can be used to verify

##### that the magnetic field is between the thresholds

##### set by MGLT[2:0] and MGHT[2:0] in register 6.

##### The value of MIR[3:0] is 0011 when the field is

##### between these thresholds. This also indicates

##### that the Hall sensor front end is functioning

##### correctly and acquiring angle samples.

##### For example, assuming MGLT[2:0] is at its

##### default value of 00 (15mT), and the MGHT[2:0]

##### is at 111 (126mT), then if the field strength is

##### between these values, the MIR[3:0] value is

##### 0011.

##### Table 2 4 shows the MIR[3:0] values for the

##### default threshold settings described above.


MAQ473 Rev. 1. 1 MonolithicPower.com 26
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

```
Table 2 4 : MIR[3:0] Logic States
```
##### MIR[ 3 :0] Conditions MGLT[2:0] at default 15mT, MGHT[2:0] at default 126mT

##### 0011 Field in range and Hall sensor array operational

##### 1111 Field above MAGH and MAGL threshold (magnet too close to sensor)

##### 0000 Field below MAGH and MAGL threshold (magnet too far away/missing)

##### 0001 Invalid state: Hall array sensor functional error

##### 0010 Invalid state: Hall array sensor functional error

##### 0100 Invalid state: Hall array sensor functional error

##### 1000 Invalid state: Hall array sensor functional error

##### 1001 Invalid state: Hall array sensor functional error

##### 1010 Invalid state: Hall array sensor functional error

##### 1011 Invalid state: Hall array sensor functional error

##### 1100 Invalid state: Hall array sensor functional error

##### 1101 Invalid state: Hall array sensor functional error


MAQ473 Rev. 1. 1 MonolithicPower.com 27
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### TYPICAL APPLICATION CIRCUITS

```
Figure 31 : Typical Configurations Using SPI Interface and MGH/MGL Signals
```
```
Figure 32 : Typical Configuration Using ABZ Interface
```

MAQ473 Rev. 1. 1 MonolithicPower.com 28
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### PACKAGE INFORMATION

##### QFN-16 (3mmx3mm)


MAQ473 Rev. 1. 1 MonolithicPower.com 29
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### APPENDIX A: DEFINITIONS

##### Effective

##### Resolution (3σ

##### noise level)

##### The smallest angle increment distinguishable from the noise. The resolution is

##### measured by computing three times σ (the standard deviation in degrees) taken

##### across 1 ,000 data points at a constant position. The resolution in bits is obtained with:

##### log 2 (360 / 6σ).

##### Refresh Rate The rate at which new data points are stored in the output buffer.^

##### ABZ Update

##### Rate

##### The rate at which a new ABZ state is computed. The inverse of this rate is the minimum

##### time between two ABZ edges.

##### Latency

##### The time elapsed between the instant when the data is ready to be read and the instant

##### at which the shaft passes that position. The lag in degrees is lag = latency x v, where

##### v is the angular velocity in deg/s.

##### Start-Up Time The time until the sensor delivers valid data beginning^ at start-up.^

##### Integral

##### Nonlinearity

##### (INL)

##### Maximum deviation between the average sensor output (at a fixed position) and the

##### true mechanical angle (see Figure A1).

```
Figure A1: Resolution, INL, Lag
```
##### INL can be obtained from the error curve err(a) = out(a) - a, where out(a) is the average

##### across 1 ,000 sensor outputs and a is the mechanical angle indicated by a high-

##### precision encoder (<0.001°). INL is then calculated with Equation (A1):

```
max(err(a)) - min(err(a))
INL =
2
```
##### (A1)

##### Drift

##### The angle variation rate when one parameter is changed (e.g. temperature, VDD) but

##### all others, including the shaft angle, remain constant.


MAQ473 Rev. 1. 1 MonolithicPower.com 30
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### APPENDIX B: SPI COMMUNICATION CHEATSHEET

##### Read Angle

##### Read Register

##### Write Register


MAQ473 Rev. 1. 1 MonolithicPower.com 31
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

## CARRIER INFORMATION

```
Part Number Package
Description
```
```
Quantity/
Reel
```
```
Quantity/
Tube
```
```
Quantity/
Tray
```
```
Reel
Diameter
```
```
Carrier
Tape
Width
```
```
Carrier
Tape
Pitch
MAQ473GQE-
AEC1-Z
```
###### QFN- 16

```
(3mmx3mm) 5000 N/A^ N/A^ N/A^ 13in^ 12mm^
```
##### Pin 1 1 1 1 1

```
ABCD ABCD ABCD ABCD
```
```
Feed Direction
```

Notice: The information in this document is subject to change without notice. Users should warrant and guarantee that third-
party Intellectual Property rights are not infringed upon when integrating MPS products into any application. MPS will not assume
any legal responsibility for any said applications.

MAQ473 Rev. 1. 1 MonolithicPower.com 32
8/8/2022 MPS Proprietary Information. Patent Protected. Unauthorized Photocopy and Duplication Prohibited.

### REVISION HISTORY

```
Revision # Revision Date Description Pages Updated
1.0 5/12/2021 Initial Release -
```
###### 1.1 8/ 8 /2022

```
Updated Typical Application figure. 1
Updated “1mF” to “1μF” in Figure 6. 10
Updated minimum SSI limits for SSCK period, Low level
of SSCK signal, and High level of SSCK signal in Table
5.
```
###### 15

```
Added MGL Application Note section, MGL Workarounds
section, and Table 1 8.
```
###### 22 – 23

```
Minor formatting updates. 23
```

