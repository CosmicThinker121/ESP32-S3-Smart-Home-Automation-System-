# Testing and Troubleshooting

Testing was carried out iteratively as the prototype was assembled, with each fault traced to a specific cause before a corrective change was made and re-verified.

## Test Case Summary

| Test Case | Expected Behaviour | Observed Behaviour (Before Fix) | Result After Fix |
|---|---|---|---|
| Relay switching direction | Relay energises when firmware state = ON | Relay energised when state = OFF (inverted) | Corrected with active-LOW macros; ON/OFF now match physical relay state |
| Dashboard virtual pin mapping | Each widget reflects its intended sensor/actuator | Fan, Pump, Fridge, Temp, Water, Safety widgets showed wrong or static values | Firmware VPIN defines realigned to dashboard; all widgets verified live |
| Water pump relay activation | Pump relay energises when water level ≤20% | No relay click, no LED, on that channel only | Traced to missing shared ground on pump relay supply; corrected by tying grounds |
| WiFi outage at boot | System starts and automation runs without WiFi | System hung indefinitely awaiting WiFi | 30-second non-blocking timeout added; verified local automation runs offline |
| WiFi recovery | Blynk reconnects automatically once WiFi returns | Required manual reboot | `WiFi.setAutoReconnect` + periodic retry added; reconnects without reboot |
| Motion alarm sound | Audible pulsed beep while armed and motion present | Continuous single tone | Buzzer output toggled per loop cycle to produce a beep pattern |

## Fault Diagnosis Narratives

### 1. Active-LOW Relay Inversion

The most disruptive early fault: every relay's ON/OFF behaviour was completely inverted — pressing the bulb button to indicate OFF on both the LCD and the Blynk dashboard caused the physical bulb to switch ON, and vice versa. Root-cause analysis confirmed the relay module itself was active-LOW, a detail not always specified clearly on generic relay board datasheets. It was easy to miss because the firmware's state variables, LCD text, and Blynk dashboard were all internally consistent with each other — only the final physical output stage was inverted. The fix was isolated to the `digitalWrite()` calls feeding the relay pins, leaving all higher-level logic untouched.

### 2. Dashboard/Firmware Virtual Pin Mismatch

After the Blynk.Console dashboard was rebuilt with widgets in a different order than the firmware's original virtual pin assignment, six of the seven dashboard widgets displayed incorrect or static data despite the underlying sensors and relays functioning correctly. Resolved by systematically comparing dashboard widget-to-pin bindings against the firmware's `VPIN_*` macro definitions and realigning the firmware to match the dashboard — editing seven macro definitions was faster than rebuilding seven dashboard widgets.

### 3. Unresponsive Water Pump Relay

A single relay channel — the water pump — showed no response whatsoever: no audible click, no indicator LED activity, while the other three channels operated normally. Because the fault was isolated to exactly one channel while the GPIO signal, firmware logic, and wiring pattern were otherwise identical to the working channels, the most probable cause was a missing common ground between the ESP32-S3's logic ground and the separate supply feeding that section of the relay board — a fault mode that produces exactly this symptom: the control signal toggles correctly at the microcontroller but never reaches a usable logic level at the relay's opto-isolator input.

### 4. Blocking WiFi Connection at Boot

The original use of `Blynk.begin()`, which blocks until WiFi connects, meant the entire system — including safety-critical local automation with no genuine dependency on the internet — would not start at all if the configured WiFi network was unavailable. This was identified as an availability risk inappropriate for a system with a safety function (motion alarm, water overflow prevention), and corrected by separating the WiFi/Blynk connection process from the rest of system initialisation.

![Ground fault troubleshooting](../images/fig6.1-ground-fault-troubleshooting.png)

*Interior wiring inspected while diagnosing the water pump relay's missing common-ground fault.*
