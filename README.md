# Single-Phase Cycloconverter | ATmega328P

An embedded power-electronics project combining SCR switching, zero-crossing synchronisation, register-level C firmware, and a custom four-layer PCB designed in KiCad.

The intended input is **230 V AC RMS at 50 Hz**. The firmware runs an automatic sequence of frequency and voltage targets, then ramps down and disables gate firing.

## Operating sequence

The uploaded firmware uses a nominal **5 V/Hz** lookup table and the following sequence:

| Step | Frequency target | Nominal voltage target | Duration |
| --- | --- | --- | --- |
| Ramp up | 0 → 10 Hz | 0 → 50 V | 5 s |
| Hold | 10 Hz | 50 V | 2 s |
| Ramp up | 10 → 20 Hz | 50 → 100 V | 5 s |
| Hold | 20 Hz | 100 V | 2 s |
| Ramp up | 20 → 40 Hz | 100 → 200 V | 5 s |
| Hold | 40 Hz | 200 V | 2 s |
| Ramp down | 40 → 0 Hz | 200 → 0 V | 5 s |

After ramp-down, the controller remains in its stopped state until reset. The firmware reaches zero after 5 seconds of ramp-down and enters its final state at the next 500 ms update.

The hold-time comments in the source say 10 seconds, but the implemented thresholds are 2000 ms, giving **2-second holds**.

Voltage values are intended lookup-table targets, not measurements or closed-loop regulation. The switched output is non-sinusoidal; its RMS voltage and frequency spectrum require waveform verification, particularly at the 40 Hz setting.

## Firmware

The controller is an **ATmega328P running at 16 MHz**, programmed in embedded C using direct register access.

- **Zero-crossing interrupt:** PB0 / PCINT0 supplies the input timing reference.
- **Timer1:** Schedules the firing delay and gate-pulse duration with a prescaler of 64.
- **Gate pulses:** Approximately 300 µs, based on `PULSE_TICKS = 74`.
- **Timer2:** Generates a 1 ms timebase for the ramp and hold sequence.
- **Frequency control:** A phase accumulator selects the commanded output polarity at input half-cycle boundaries.
- **Voltage control:** `fire_table[]` selects a firing delay for each frequency setpoint.
- **Shutdown:** A zero-frequency setpoint prevents further gate firing.

The control sequence runs in interrupt service routines; the main loop remains idle.

## Pin mapping

| Signal | ATmega328P pin |
| --- | --- |
| Zero-crossing input | PB0 |
| T1 | PB1 |
| T2 | PB2 |
| T3 | PB3 |
| T4 | PB4 |
| N1 | PD4 |
| N2 | PD5 |
| N3 | PD6 |
| N4 | PD7 |

This firmware defines **eight gate outputs** in two groups. Confirm that the schematic and PCB revision match this mapping before using the firmware; the earlier four-SCR PCB arrangement must not be assumed to match it directly.

## Four-layer PCB design

The project also includes a **custom four-layer PCB designed in KiCad**, with attention to the separation of mains circuitry and low-voltage control electronics.

PCB design features include:

- Separate high-voltage power and low-voltage control regions.
- Isolation keepout regions around the optocoupler barrier.
- An internal ground pour confined to the low-voltage region.
- Wider copper tracks for the main SCR and load-current paths.
- Dedicated routing classes for power, mains sensing, and control signals.
- RC snubbers placed close to the associated switching devices.
- Input fuse and MOV provision.
- External power headers and terminal-block connections.

The design work covers schematic capture, footprint assignment, multilayer routing, and fabrication-output preparation. PCB layout completion does not by itself establish hardware validation or electrical-safety compliance.

## Development tools

| Tool | Purpose |
| --- | --- |
| Microchip Studio | AVR firmware development |
| Proteus | Circuit simulation and waveform inspection |
| KiCad | Schematic capture and four-layer PCB design |

## Current limitations and future improvements

- Output voltage is controlled through a fixed lookup table; the supplied firmware has no voltage-feedback loop.
- Input polarity is inferred by alternating a software flag at each accepted zero-crossing event.
- The supplied firmware has no current-zero detection or current-based interlock between converter groups.
- Planned improvements include current sensing, commutation interlocking, and measured calibration of the voltage targets.

## Safety

This project involves hazardous 230 V AC. Gate signals turning off do not force an SCR to stop conducting. Hardware testing requires appropriate protection, mains-rated measurement equipment, and verification of commutation behaviour, especially with inductive loads.

## Author

Jason Nkuna

Embedded systems, power electronics, and PCB design.
