# Software for Pecan Balloon Trackers

This is the software, that runs the [Pecan Pico 7 balloon trackers](https://github.com/DL7AD/pecanpico7). Since the new trackers can be used in many ways, this software is designed for multiple purpose. It can be used for:
- Image transmissions
   - [SSDV](https://ukhas.org.uk/guides:ssdv) as 2FSK modulation (RTTY)
   - [APRS](http://aprs.org)-encoded [SSDV](https://ukhas.org.uk/guides:ssdv) as AFSK or 2GFSK modulation
- Position/Telemetry transmission (GPS)
   - by [APRS](http://aprs.org) AFSK or 2GFSK
   - 2FSK (RTTY)
   - CW (morse)

This software may also feature
- Amateur satellite communication links
- Full duplex repeater for digital modulations (up 70cm, down 2m)
- Ground control access via radio link

###### Transmitted Test Images
Here are some images which were transmitted by [SSDV](https://ukhas.org.uk/guides:ssdv). These QVGA-images were captured by an [Omnivision OV9655](http://www.waveshare.com/wiki/OV9655_Camera_Board).

![Test image 1](https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/sample_pictures/test2.jpg)
![Test image 2](https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/sample_pictures/test3.jpg)<br>

###### Targets
Currently there is only one target supported by the Software. Pecan Pico 7a will be succeeded by Pecan Pico 7b due to some mayor errors ([See known PCB errors](https://github.com/DL7AD/pecanpico7/tree/7a)).

<img src="https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/targets/pecanpico7a.jpg" width="320" alt="Pecan Pico 7a"><br>[Pecan Pico 7a](https://github.com/DL7AD/pecanpico7/tree/7a)

###### Features
All features can be combined as required for the user. Currently the software is incomplete but basic features can be used:

- Image capturing (just QVGA at the moment) and transmission by SSDV
- Reception of positions (by ublox MAX8 GNSS module)
- Measurement of temperature/air-pressure/humidity (by onboard BME280)
- Measurement of temperature/air-pressure/humidity (by external BME280)
- Measurement of the power consumption (battery charging and discharging)
- Position and telemetry transmissions by 2FSK (or RTTY)
- High altitude mode (up to 50km altitude)
- Debugging (serial connection)
- APRS encoding (AFSK, 2GFSK)
- APRS SSDV-encoded Image Transmission

What currently doesn't work (but planned):
- High resolution image capture (just QVGA at the moment)
- Logging and log transmissions
- Amateur satellite orbit calculations and transmissions
- Power save modes
- Storage of telemetry and image data on the microSD card

It is currently not planned to implement video capture. The STM32F4 doesn't have sufficient computing power for compression encoding. However the new pin-compabile [STM32F7x7](http://www.st.com/web/en/catalog/mmc/FM141/SC1169/SS1858) supports JPEG encoding. So video capture may be possible in future.

###### Dependency installations/ Compiling/ Flashing
In order to get the hardware working, you'll need a programmer. There are several devices on the market but I personally use the [STM32F429I-DISCOVERY](http://www.st.com/st-web-ui/static/active/en/resource/technical/document/user_manual/DM00093903.pdf) demo board. The discovery board is basically a programmer with a demo target (microcontroller) attached to it. It's designed in a way so you can either flash the demo target or the Pecan Pico 7 by switching specific jumpers on the board. You may get the programmer at [Mouser](http://www2.mouser.com/search/ProductDetail.aspx?R=0virtualkey0virtualkeySTM32F429I-DISC1), [Farnell](http://www.newark.com/stmicroelectronics/stm32f429i-disc1/dev-board-stm32f429zi-advanced/dp/72Y1169), [Digikey](http://www.digikey.de/product-detail/de/stmicroelectronics/STM32F429I-DISCO/497-13898-ND/4310131) or on Ebay for roughly $30. You'll need to connect the programmer as follows. Don't forget to remove the jumpers which connect the programmer with the demo target.

You'll also need some software dependencies.
- _GCC_ to build binary files
- _ST-Flash_ to flash the binaries to your target
There's a [great guide](http://www.wolinlabs.com/blog/linux.stm32.discovery.gcc.html) on the Web which I've followed myself to set up everything. They are using a different demo board but it works similarily.

Then you'll need the source code for the tracker:
```
git clone --recursive https://github.com/DL7AD/pecan-stm32f429.git
cd pecan-stm32f429
make
make flash
```
A _build_ folder will be created automatically. The software can be configured/customized in the [config.h](config.h).

###### How to connect the programmer to the Pecan Pico 7 PCB
<img src="https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/programmer.jpg" width="400" alt="Programmer connected to Pecan Pico 7">




# Technical Description
Technical description below...

# Modules
This section describes the different function blocks in the software and how they interact which each other. The software provides multiple modules, which supply the main functionality of the tracker like transmitting positions or images. Modules are basically [threads in the ChibiOS RTOS](http://www.chibios.org/dokuwiki/doku.php?id=chibios:howtos:createthread). Modules can be:

- POSITION - for transmining position and telemetry
- IMAGE - for transmitting images
- LOG - for transmitting logs

There are two more specific modules which are started automatically be the software at run time. They are necessary to handle input/output jobs.
- TRACKING (or tracking manager) - collects all the data from the periphery (BME280, GPS, etc.) and provides them (as struct) to other modules in the software.
- RADIO - provides the two radios which are available. This module receives internal messages from other modules which intend to send packets via VHF or UHF.

The following diagram shows how the modules interact with each other.

```
   DRIVER   |                  DATA PROCESSING MODULES                     |     DRIVER
------------+--------------------------------------------------------------+------------------

 MPU9250 ---+                           +---> SATELLITE ---+
            |                           |                  |
 BME280 ----+---> TRACKING Manager -----+---> POSITION ----+----> RADIO ---+---> RADIO1 (2m)
            |                           |                  |               |
 MAX -------+                           +-----> LOG -------+               +---> RADIO2 (70cm)
                                                           |
 OV9655 or ----------------------------------> IMAGE ------+
 OV2640
```

## Module configuration
In order to use these modules, they can be configured in the MOULDES() macro in the [configuration file](config.h). All modules can be configured except of TRACKING and RADIO. They have no configurable parameters because they get their parameters from other modules in run time. The following example shows how an image module and a position module can be configured.
```
MODULES() { \
    MODULE_IMAGE   (TX_CONTINUOSLY,          SLEEP_WHEN_BATT_BELOW_3V5, CUSTOM_FREQ, 10, PROT_SSDV_2FSK ); \
    MODULE_POSITION(WAIT_FOR_TRACKING_POINT, SLEEP_WHEN_BATT_BELOW_3V0, CUSTOM_FREQ, 10, PROT_UKHAS_2FSK); \
}
```
As you can see, there are several parameters which can be used to configure a specific module:

**Parameter 1** specifies in which cycle the modules shall send their data. This can be a time in seconds. It's also allowed to indicate *TX_CONTINUOSLY* or *WAIT_FOR_TRACKING_POINT* in this field.<br>
*TX_CONTINUOSLY* - Transmit new data as soon as the previous frame is compled<br>
*WAIT_FOR_TRACKING_POINT* - Transmit when the tracking manager generates a new track point<br>
**Parameter 2** specifies the sleep mode. Note this function is currently not implemented. The modules will disregard this field.<br>
**Parameter 3** specifies a frequency method. Currently *CUSTOM_FREQ* or *APRS_REGION_FREQ* can be used. The frequencies are dynamic and set in [radio.c](radio.c). *CUSTOM_FREQ* is set to 434.500MHz and *APRS_REGION_FREQ* is set to 144.800MHz as the default. A geofencing algorithm is implemented for *APRS_REGION_FREQ* in order to find out the specific APRS region frequency by the actual location.<br>
**Parameter 4** specifies the transmission power in dBm. If the value is set to a point which the radio is unable to reach (or would damage the PCB) the actual value may be limited by the RADIO module.<br>
**Parameter 5** specifies the protocol and modulation. While you can use *PROT_UKHAS_2FSK*, *PROT_APRS_AFSK* or *PROT_RAW_CW* for the POSITION module, the IMAGE module can accept *PROT_SSDV_2FSK*, *PROT_SSDV_APRS_AFSK* and *PROT_SSDV_APRS_2GFSK* at the moment.

Note not all of the parameters can be used for every individual module because some configurations may not make sense e.g. CW modulation for transmitting images. You can find an exact documentation in the [configuration file](config.h) which specific parameters can be applied to the the modules.

## Concurrent use of modules
Modules can be merged in various combinations. However modules can be also understood as classes whose instances can be used multiple times. So if you want to transmit telemetry and positions by RTTY and APRS at the same time, you may configure modules multiple times. The following example shows, how the position/telemetry is sent on a custon frequency while it's also transmitted by APRS on the regional APRS frequency and CW.
```
MODULES() { \
    MODULE_POSITION (WAIT_FOR_TRACKING_POINT, SLEEP_WHEN_BATT_BELOW_2V5, CUSTOM_FREQ,      10, PROT_UKHAS_2FSK); \
    MODULE_POSITION (WAIT_FOR_TRACKING_POINT, SLEEP_WHEN_BATT_BELOW_2V5, APRS_REGION_FREQ, 10, PROT_APRS_AFSK ); \
    MODULE_POSITION (WAIT_FOR_TRACKING_POINT, SLEEP_WHEN_BATT_BELOW_3V0,CUSTOM_FREQ,       10, PROT_RAW_CW    ); \
}
```

## Module TRACKING (or tracking manager)
The tracking manager keeps an eye on all peripheral components that provide data. With exception of the camera, the tracking manager collects the data of:
- BME280 (airpressure, humidity, temperature)
- MAX7/8 (GPS/GLONASS receiver)
- PAC1720 (Power consumption tracking)
The data of these components will be collected at a specific cycle time and will be provided to other modules in a struct [trackPoint_t](modules/tracking.h). The most recently sampled data can be accessed by other modules by [getLastTrackPoint()](modules/tracking.h).

## Module RADIO
The module RADIO handles the two radios which are available on the Pecan Pico 7. It initializes a mailbox in which messages (or data packets) can be written by other functions in order to use the radios. If a module wants to send a packet on HF, it may generate a [radioMSG_t](modules/radio.h) struct and pass is over to [transmitOnRadio()](modules/radio.h). The radio will determine a suitable radio by the frequency which is set in the struct. Once the radio is free, the module will send the message. If the radio is already in use, the RADIO module may buffer up to 3 more messages in a message queue. The queue is cleared by the FIFO concept. While only 3 messages can be stored in the queue, the maximum generation-transmission delay is quite small (<15sec usually). [transmitOnRadio()](modules/radio.h) may return *true* if the message is sent to the queue. *False* indicates the queue is full. If a module wants to send the message, it has to try again by calling transmitOnRadio() again. While there is no priority management at the moment, the modules have to try more or less aggressivly to insert its messages to the radio queue. This strategy more a workaround than a solution. Priority management would be better.

###### 2FSK modulation
2FSK is the name for RTTY in the software because this modulation type also transmits start and stop bit (8n2). The modulation is managed by a software timer because the timing dont have to be that accurate (compared to AFSK and GFSK). The modulation is passed over to the Si446x by a GPIO pin. As the pin is toggled, the transmitter will change its frequency.

###### CW modulation
CW is moduled like 2FSK. The transmitter is just set into the mode OOK (On Off Keying).

###### APRS AFSK modulation
The APRS modulation is a Si446x-specific hack because the Si Chip is unable to do FM natively. But Si446x chips are able to modulate 2GFSK. GFSK has a much smoother frequency change than its the case at FSK. The modulation is afftected on a GPIO pin at the transmitter which is connected to the Microcontroller. As this GPIO pin is switched the transmitter also changes its frequency. This characteristic knowlegde can be used to implement FM. If the pin is toggled 4400 times in a second, a 2200Hz tone can be modulated at FM. The 1200Hz tone can be modulated by toggling the pin 2400 times in a second. The modulation is additionally filtered in the Si446x chips with a FIR filter, so harmonics above 2200Hz are filtered.
Unfortunately the modulation requires an accurate timing. If the timing is not managed correctly at the microcontroller, the modulation gets distorted. To ensure accurate timings, all threads and interrupts are disabled at the microcontroller so the pin is toggled at the right time.

###### APRS 2GFSK modulation
The 2GFSK modulation is quite simple because the Si446x supports 2 and 4GFSK natively. Like it`s done at AFSK, the modulation is generated by the microcontroller which requires timing. So the microcontroller cant do anything else at the actual transmission.

## Module POSITION
This module handles the radio transmission encoding for all position/telemetry packets. It is able to encode CW, APRS and 2FSK (or RTTY). A cycle can be continuosly, time-specific or be triggered by the TRACKING manager when a new track point is published. This modules can be configured in the MODULES() marco: `MODULE_POSITION(trigger, sleep, frequency, power, protocol)`.

| Field     | Meaning | Possible options
| --------- | ------- | ----------------
| trigger   | Cycle time or trigger type for transmission | `TX_CONTINUOSLY` or `WAIT_FOR_TRACKING_POINT` or Integer (in seconds)
| sleep     | Sleep method | `SLEEP_WHEN_BATT_BELOW_2V9` or `SLEEP_WHEN_BATT_BELOW_3V0` ... `SLEEP_WHEN_BATT_BELOW_4V1` or `SLEEP_WHEN_ISS_NOT_VISIBLE`
| frequency | Frequency method | `CUSTOM_FREQ` or `APRS_REGION_FREQ` or `APRS_ISS_FREQ`
| power     | Radio power (in dBm) | Integer
| protocol  | Protocol | `PROT_APRS_2GFSK` or `PROT_APRS_AFSK` or `PROT_UKHAS_2FSK` or `PROT_RAW_CW` or `PROT_APRSCONFIG_AFSK` or `PROT_APRSCONFIG_2GFSK`

For a detailed explanation about the option macros see [config.h](config.h).

###### APRS Encoding
This software makes use of the amateur radio [APRS system (Automatic Packet Reporting system)](http://aprs.org), which has been invented by Bob Bruninga in 1992. Since APRS has a [great coverage](http://aprs.fi/#!mt=roadmap&z=3) in the US and Europe, is it very useful for balloon tracking. At the moment its also the most convenient system which can be used.

The software itself implements the compressed format. This software is actually not recommended but supported by all devices. Since there has been no replacement and the raw format is quiet big, this is still the best encoding option. The format also supports 5 different 13bit-telemetry transmissons which are used for:
- Battery voltage
- Solar voltage
- Temperature (in Celcius)
- Battery charge (in mW)
- Battery discharge (in mW)

The software also might display the GPS-satellite count, the TTFF (time to first fix) and GPS-LOSS counter in the comment field. Note the telemetry fields are mathematically encoded and the receives have to know, how they can decode this telemetry. In order to transmit the configuration by the tracker itself, place an additional POSITION modules in the MODULES() marco in the [config.h](config.h) with the protocol `PROT_APRSCONFIG_AFSK`. It will transmit all the necessary decoding information for the receiver.

`MODULE_POSITION (3600, SLEEP_WHEN_BATT_BELOW_3V0, APRS_REGION_FREQ, 10, PROT_APRSCONFIG_AFSK);`

All APRS messages will be encoded by the POSITION module for the RADIO module in a bitstream (1 bit = 1 Symbol). The packets already contain the AX.25 specific bitstuffing, NRZ-I encoding (for AX.25 AFSK) and scrambling (for AX.25 2GFSK).

![APRS Packet](https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/aprs.jpg)<br>
Transmitted APRS packet displayed by [APRS.fi](http://aprs.fi)

###### 2FSK Encoding
2FSK messages will be encoded especially for the [UKHAS system](https://ukhas.org.uk/guides:tracking_guide). Unlike AFSK, this modulation can be only received with a SSB receiver. See on the [UKHAS Tracking Guide](https://ukhas.org.uk/guides:tracking_guide) which receivers can be used. While UKHAS is very flexible, the packet format can be configured in the configuration file by the UKHAS_FORMAT marco. The format contains a set of variables which can be changed user specific.

`#define UKHAS_FORMAT "<CALL>,<ID>,<TIME>,<LAT>,<LON>,<ALT>,<SATS>,<TTFF>,<VBAT>,<VSOL>,<CHARGE>,<IPRESS>,<ITEMP>,<IHUM>,<EPRESS>,<ETEMP>,<EHUM>"`

The preamble ($$$$$) in the front and CRC calculation at the end will be added by the software automatically. The output could be:

`$$$$$D-9,5,00:03:15,52.4999910,13.5000084,19948,9,16,4.33,0.00,-1.114,100271,36.1,22,0,0.0,0*8B0B`

| Data        | Explanation
| ----------- | -----------
| $$$$$       | Snchronisation (added by the software)
| D-9,        | Payload name (See UKHAS_CALLSIGN info [config.h](config.h))
| 5,          | Sequence ID (incremented each TRACKING manager cycle)
| 00:03:15,   | GPS Time (when this fix has been sampled)
| 52.4999910, | GPS Latitude
| 13.5000084, | GPS Longitude
| 19948,      | GPS altitude in meters
| 9,          | GPS satellites
| 16,         | TTFF (in seconds)
| 4.33,       | Battery voltage (in volts)
| 0.00,       | Solar voltage (in volts)
| -1.114,     | Power consumption (in Watts)
| 100271,     | Air Pressure of onbard BME280 (in Pascal)
| 36.1,       | Temperature (in Celcius)
| 22,         | Humidity (in %)
| 0,          | Air Pressure of offbard BME280 (in Pascal)
| 0.0,        | Temperature of offbard BME280 (in Celcius)
| 0           | Humidity of offbard BME280 (in %)
| *8B0B       | CRC (Calculated by software)

The CRC calculation uses crc16-ccitt (0x1021). A detailed documentation about the tags can be found in the [config.h](config.h).

![2FSK Packet](https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/2fsk.jpg)<br>
Transmitted 2FSK packet displayed by [HABHUB](http://tracker.habhub.org)

###### CW Encoding (morse)
CW was implemented while some persons are not able to do APRS/2FSK or simply don't like it. Since no additional equipment is needed for decoding, the signal can be possibly picked up by someone else who don't know about the balloon launch and how to decode it. CW might be also useful for [ARDF (Amateur radio direction finding)](https://en.wikipedia.org/wiki/Amateur_radio_direction_finding) if the GPS failed to receive.
Like 2FSK, CW encoding can be configured flexible by the CW_FORMAT macro. All configuration tags which are known for 2FSK can be also used for CW. The POSITION module may encode A-Z, 0-9, "." and space. All other characters will be dismissed e.g. "D-9" => "D9". `#define CW_FORMAT "<CALL> <LOC> <ALT>M PSE QSL"` will encode a tracking point as `D9 JO62RM 19948M PSE QSL`. The message will be encoded binary as 1/10sec bitstream (1: Tone, 0: No Tone). This message is then sent to the RADIO module.

Note CW is no format which is very useful for computer-enabled processing, so there is no service like [APRS.fi](http://aprf.fi) or the tracker service by [HABHUB](http://tracker.habhub.org) for CW.

![CW Packet](https://raw.githubusercontent.com/DL7AD/pecan-stm32f429/master/doc/cw.jpg)<br>
Transmitted CW message contains [Maidenhead Locator](https://en.wikipedia.org/wiki/Maidenhead_Locator_System) and altitude displayed by [DL-FLDigi](https://ukhas.org.uk/projects:dl-fldigi)

## Module IMAGE
The module IMAGE handles the image transmission of the Pecan Pico 7 and encodes it for the radio. At the moment, there is only enough memory available to handle one image thread. It's actually possible to run multiple instances but the instances will interfere each other due to multiple use of the same memory. There are 2 (and a half) methods which can be used to transmit the pictures.

- SSDV transmitted by 2FSK
- Modified SSDV-packet in an APRS packet transmitted by AFSK or 2GFSK

Raw SSDV is described [here](https://ukhas.org.uk/guides:ssdv). Mostly SSDV is transmitted by 2FSK 600baud which is quiet slow. So this tracker is also able to transmit a modified SSDV packet in an APRS packet. Copared to 2FSK 600baud, APRS can be quiet fast or good in coverage (see the strategies below). The raw SSDV packet cotains 256byte on each packet and contains:

- Sync byte
- Packet type
- Callsign
- Image ID
- Packet ID
- Width
- Height
- Flags
- MCU offset
- MCU index
- Payload
- Checksum (CRC)
- FEC

If we put SSDV into an APRS packet, no everything must be transmitted because some fields get useless it the APRS packet. So dont transmit the _Sync byte_, the _Checksum_ and the _FEC_. The packet ends up with 219byte of payload data in the APRS packet. But transmitting binary data in an APRS packet could cause interference with the APRS protocol itself (and APRS-IS), so the data is then encoded by base91. By encoding the payload data is enlarged to ~270byte. The enlargement by base91 and protocol overhead by APRS sounds quiet awful. But RTTY also needs additional start and stop bits which is not neccessary at AFSK and 2GFSK, so you wont be able to transmit 600bit/s at 600baud. The packet overhead of the SSDV/APRS packets is rougly 26% depending on the preamble. This includes base91 encoding, additional protocol and preamble. RTTY therefore needs one start- and one stop-bit which generates 20% encoding overhead.

**Transmission Improvements:**

| Method     | Symbol rate | Bit rate   | Packet size\* | Packets per sec. | Transmission time, per image\*\*
| ---------- | ------------| ---------- | ------------- | ---------------- | --------------------------------
| RTTY       | 600 baud    | 480 bit/s  | 256byte       | 0.23             | 260 sec
| APRS AFSK  | 1200 baud   | 1200 bit/s | 350byte       | 0.43 (+87%)      | 140 sec
| APRS 2GFSK | 1200 baud   | 9600 bit/s | 350byte       | 3.43 (+1391%)    | 17 sec

\* includes preamble
\*\* 60 packets per image, high duty cycle

Even if APRS encoded SSDV is incredible fast, it also has disadvantages. Reliability for example and the need of receiving equiment.
Here is a list of advantages and disadvantages of all the different methods and transmission strategies:

###### Transmit SSDV 2FSK modulated at 600baud with high duty cycle
Transmission speed: low, 4...5min for one QVGA image<br>
Reliability: very high due to FEC<br>
Equipment requirements: low, RTL-SDR and antenna<br>
Receiver coverage: low (stations must be informed by UKHAS usually)<br>
Other: Requires software which most people dont know how to use it

###### Transmit specialized SSDV APRS protocol, AFSK modulated with high duty cycle on a separate frequency
Transmission speed: faster, 2min for one QVGA image<br>
Reliability: low, it has no FEC (just CRC), packet size quiet large<br>
Equipment requirements: low, RTL-SDR or simple FM receiver and antenna<br>
Receiver coverage: low (stations must be informed)<br>
Other: Many people know how to receive APRS, special Habitat-Igating software necessary for SSDV server (software which sends the received packets to the server)

###### Transmit specialized SSDV APRS protocol, AFSK modulated with very low duty cycle on the navive APRS frequencies (144.800 / 144.390)
Transmission speed: very very slow, 30-45min for one QVGA image<br>
Reliability: low, it has no FEC (just CRC), packet size quiet large<br>
Equipment requirements: none (let the available stations receive)<br>
Receiver coverage: very very high (Europe, USA, Canada, Australia, China, Japan, etc.)<br>
Other: Making use of existing APRS netework, Igating software necessary which receives the packets from the APRS-IS an relay them to the SSDV-server

###### Transmit specialized SSDV APRS protocol, AFSK modulated with high duty cycle on a separate frequency
Transmission speed: very very fast, 15sec for one QVGA image, 8min for one Full HD image<br>
Reliability: medium, it has no FEC (just CRC)<br>
Equipment requirements: medium/high, RTL-SDR or special APRS9k6 TNC and receiver<br>
Receiver coverage: low (stations must be informed)<br>
Other: Many people know how to receive APRS (even 9k6), special Habitat-Igating software necessary for SSDV server (software which sends the received packets to the server)



# Included software
This software includes some other packages which I have used to make this project possible:

[JPEG Ant - JPEG compression for microcontrollers](https://sourceforge.net/projects/jpegant.berlios/)<br>
[ChibiOS - Real time operating system](http://chibios.org) ([Source](https://github.com/ChibiOS/ChibiOS))<br>
[SSDV - Slow Scan Digital Video](https://ukhas.org.uk/guides:ssdv) ([Source](https://github.com/fsphil/ssdv))
