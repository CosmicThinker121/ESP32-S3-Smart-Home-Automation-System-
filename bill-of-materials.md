# Bill of Materials

Complete 28-item BOM used in the prototype build.

| # | Component | Qty | Purpose / Notes |
|---|---|---|---|
| 1 | ESP32-S3 DevKit Board (N16R8) | 1 | Main controller; WiFi + Blynk connectivity |
| 2 | 4-Channel 5V Relay Module | 1 | Switches bulb, fan, pump, fridge (mains side) |
| 3 | 20x4 LCD Display with I2C Module | 1 | Local status display |
| 4 | 830 Tie-Point Breadboard | 1 | Prototyping |
| 5 | 5V DC Power Supply Adapter | 1 | Powers relay module / logic side |
| 6 | Jumper Wire Set | 1 Set | Prototyping connections |
| 7 | 5mm LEDs | 4 | Status indicators: Safety Mode, Fan, Pump, plus one spare |
| 8 | 220 Ohm Resistors (1/4W) | 4 | Current limiting for LEDs in item 7 |
| 9 | 4.7k Ohm Resistor (1/4W) | 1 | DS18B20 1-Wire pull-up |
| 10 | 10k Ohm Resistors (1/4W) | 5 | Spare / general-purpose (buttons use INPUT_PULLUP) |
| 11 | Tactile Push Buttons | 4 | Bulb, Fridge, Safety Mode, and Mute overrides |
| 12 | 18W AC LED Bulbs | 4 | Simulated loads: bulb, fan, pump, fridge |
| 13 | 2-Way Screw Terminal Blocks | 6 | Secure mains wiring connections |
| 14 | Plastic Project Enclosure Box | 1 | Houses electronics safely |
| 15 | USB Cable (ESP32-S3) | 1 | Programming / power during development |
| 16 | PVC Electrical Insulation Tape | 1 Roll | Insulating mains-side connections |
| 17 | DS18B20 Temperature Sensor | 1 | Automatic fan control input |
| 18 | Water Level Sensor Module | 1 | Automatic water pump control input |
| 19 | PIR Motion Sensor (HC-SR501) | 1 | Motion detection for Safety Mode |
| 20 | Active Buzzer Module (5V) | 1 | Safety Mode alert output |
| 21 | Inline Fuse Holder with 1A Fuse | 1 | Mains-side overcurrent protection |
| 22 | BC547 NPN Transistor | 2 | Buzzer driver; second unit held as spare |
| 23 | 1N4007 Diodes | 6 | Flyback protection (3 required minimum for fan/pump/fridge) |
| 24 | Breadboard Power Supply Module | 1 | Regulated 5V/3.3V for breadboard prototyping |
| 25 | Heat Shrink Tubing Set | 1 Set | Insulating soldered joints |
| 26 | DS3231 RTC Module | 1 | Offline real-time scheduling, survives power loss |
| 27 | 5mm LED (WiFi/Blynk status) | 1 | Dedicated WiFi connectivity indicator |
| 28 | 220 Ohm Resistor (spare, for #27) | 1 | Current limiting for WiFi status LED |

## Relay & Power Wiring

Each relay input channel (IN1–IN4) is driven from GPIOs G4, G5, G6, and G7 respectively. The relay module's VCC and GND are supplied from a 5V DC adapter, with GND commoned back to the ESP32-S3's own ground reference — this shared ground is essential (its absence was the root cause of one fault; see [testing-and-troubleshooting.md](testing-and-troubleshooting.md)).

On the mains side, each relay's COM terminal carries the incoming live conductor and its NO terminal switches the load. A 1N4007 flyback diode is fitted across each inductive load (fan, pump, fridge compressor windings) to absorb the reverse-EMF spike generated when the relay opens. Mains-voltage wiring is physically separated from 5V logic wiring inside the enclosure, insulated with heat-shrink tubing and PVC tape, with a 1A inline fuse on the incoming mains supply line.

**Safety note:** Relay COM/NO polarity must be verified before any mains load is connected — reversing these terminals can leave a load permanently energised. Every channel was tested first with a low-voltage LED load before any mains appliance was connected.

## Pre-Power Safety Checklist

- Double-check relay COM/NO wiring before connecting any mains load
- Confirm flyback diodes are present across the fan, pump, and fridge compressor coils
- Verify no bare mains conductors are exposed inside the enclosure; use heat-shrink tubing and terminal blocks throughout
- Test each relay channel individually with a low-voltage load (e.g. an LED) before connecting mains-powered appliances
- Confirm the inline 1A fuse is installed on the mains supply line before first power-up
- Confirm all grounds (ESP32-S3, relay module, sensors, LCD, RTC, buzzer, LEDs) are tied to one common reference before powering the system
