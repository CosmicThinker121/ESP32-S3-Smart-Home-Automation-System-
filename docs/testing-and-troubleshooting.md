# Testing and Troubleshooting

Testing was carried out iteratively as the prototype was assembled, with each fault traced to a specific cause before a corrective change was made and re-verified.

## Test Case Summary

| Test Case | Expected Behaviour | Observed Behaviour (Before Fix) | Result After Fix |
|---|---|---|---|
| Relay switching direction | Relay energises when firmware state = ON | Relay energised when state = OFF (inverted) | Corrected with active-LOW macros; ON/OFF now match physical relay state |
| Dashboard virtual pin mapping | Each widget reflects its intended sensor/actuator | Fan, Pump, Fridge, Temp, Water, Safety widgets showed wrong or static values | Firmware VPIN defines realigned[...] 
| Water pump relay activation | Pump relay energises when water level ≤20% | No relay click, no LED, on that channel only | Traced to missing shared ground on pump relay supply; corrected by tyi[...] 
| WiFi outage at boot | System starts and automation runs without WiFi | System hung indefinitely awaiting WiFi | 30-second non-blocking timeout added; verified local automation runs offline |
| WiFi recovery | Blynk reconnects automatically once WiFi returns | Required manual reboot | `WiFi.setAutoReconnect` + periodic retry added; reconnects without reboot |
| Motion alarm sound | Audible pulsed beep while armed and motion present | Continuous single tone | Buzzer output toggled per loop cycle to produce a beep pattern |

## Fault Diagnosis Narratives

### 1. Active-LOW Relay Inversion

The most disruptive early fault: every relay's ON/OFF behaviour was completely inverted — pressing the bulb button to indicate OFF on both the LCD and the Blynk dashboard caused the physical bul[...]

### 2. Dashboard/Firmware Virtual Pin Mismatch

After the Blynk.Console dashboard was rebuilt with widgets in a different order than the firmware's original virtual pin assignment, six of the seven dashboard widgets displayed incorrect or stati[...]

### 3. Unresponsive Water Pump Relay

A single relay channel — the water pump — showed no response whatsoever: no audible click, no indicator LED activity, while the other three channels operated normally. Because the fault was is[...]

### 4. Blocking WiFi Connection at Boot

The original use of `Blynk.begin()`, which blocks until WiFi connects, meant the entire system — including safety-critical local automation with no genuine dependency on the internet — would n[...]

![Ground fault troubleshooting](fig6.1-ground-fault-troubleshooting.png)

*Interior wiring inspected while diagnosing the water pump relay's missing common-ground fault.*
