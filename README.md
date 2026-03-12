# Cheap_Multi_Effect_Guitar_pedal

This project is a low-cost multi-effect based on the **ATMega328P** microcontroller and the **FV-1** audio DSP processor. The main goal is to create a functional device that is cheap to build and does not rely on external solutions.

## Design

* **Bare Metal:** The code is written in **pure C** (for the ATMega) and **Spin Assembly** (for the FV-1) to avoid reliance on external libraries.
* **Integration with rack system/audio interface:** MIDI integration by using the MIDI DIN 5 standard to control effects from a computer or other control systems to automate music performance.
* **Zero-waste:** The project doesn't rely only on the FV-1 within the audio domain. A lot of stuff is handled by the ATMega, like LFO modulations (for FV-1 parameters), auto-pan, etc., to maximize the music experience within a limited budget.
* **Self-Sufficient:** The chassis for this design is custom-made and 3D printed.
* **Production standards:** The project is developed and tested on prototype boards; however, in the end, it will be a printed PCB.
* **Educational Focus:** This repository is intended to be a step-by-step guide on how to design, program, and assemble the whole thing, including explanations of my design decisions.

## Hardware Choices

### ATMega328P
* **Cheap and Accessible:** One of the most affordable microcontrollers available.
* **No Programmer Required:** It doesn't require a dedicated programmer, as it can be flashed using cheap Arduino Uno clones.

### FV-1 DSP
* **High Performance:** Contains integrated 24-bit ADC and DAC.
* **Affordable:** Provides professional signal processing within a **$10–$15** price range.

### Analog modules
* **Buffers (input stage):** Provide high-quality, noise-free audio.
* **Distortion effect:** Analog distortion controlled digitally for MIDI automation purposes. Old-school sound with digital comfort.
* **EQ:** Controlled using PWM+LPF and LED+photoresistor by the ATMega.
* **Volume (output stage):** Digitally controlled analog stage, same as distortion.

---

## Documentation & Files

This repository will provide:
* Full electronic schematics.
* PCB design.
* 3D printed parts models.
* Complete source code (C and Assembly).
* Detailed price breakdowns and documentation.

*Note: Documentation, pricing, and schematics are being updated – they may already be present in the repository.*

---