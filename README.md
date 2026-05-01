# ESP32 Touchscreen Control System

Standalone embedded system built on an ESP32-S3 featuring a touchscreen stopwatch interface and an interactive game, demonstrating real-time graphics, touch input processing, and embedded UI optimization.

---

## Overview

This project implements a touchscreen-based study timer with an integrated mini-game using an ESP32-S3 microcontroller, a 4.0” SPI TFT display, and a capacitive touch controller.

The system provides:
- A responsive stopwatch interface (start, stop, reset)
- A touch-controlled game with real-time animation
- Smooth graphical rendering using optimized display techniques

The design highlights advanced embedded systems concepts including SPI display communication, I2C touch input, graphical rendering, and performance optimization.

---

## System Architecture

The system consists of four primary subsystems:

- **Processing:** ESP32-S3 microcontroller
- **Display:** ST7796 SPI TFT (480x320 landscape)
- **Touch Input:** FT6336U capacitive controller (I2C)
- **Power:** USB (ESP32) + external 5V (display)

```
[Touch Input] → [ESP32 Control Logic] → [SPI Graphics Output] → [Display]
```

---

## Control Strategy

### Operating Modes

- **STOPWATCH Mode**
  - Displays elapsed time
  - Touch buttons: START/STOP, RESET, GAME

- **GAME Mode**
  - Touch-controlled obstacle game
  - Includes physics-based motion and scoring

State transitions are handled using a high-level state machine:

- STOPWATCH → GAME (button press)
- GAME → STOPWATCH (back button)
- GAME → GAME OVER → RESET

---

## Touch Input Processing

Raw touch coordinates are received in portrait orientation and mapped to landscape:

```
sx = rawY
sy = 319 - rawX
```

This ensures proper alignment between touch input and displayed UI.

---

## Hardware Design

### Components

- ESP32-S3 (N16R8, 16MB Flash, 8MB PSRAM)
- 4.0” ST7796 SPI TFT display
- FT6336U capacitive touch controller
- External 5V supply (display)
- USB power (ESP32)

### Key Design Decisions

- **SPI display for high-speed graphics**
- **I2C touch for simplified wiring**
- **PSRAM enabled for frame buffering**
- **External power for display stability**

---

## Software Implementation

### Core Features

- Event-driven UI
- millis()-based timing (non-blocking)
- Touch-driven input handling
- Mode-based application logic
- Canvas-based rendering for animation

### Stopwatch Logic

- Uses `millis()` to track elapsed time
- Updates display only when seconds change
- Prevents unnecessary redraws (reduces flicker)

### Game Logic

- Bird motion modeled using velocity + gravity
- Pipes move horizontally across screen
- Collision detection triggers game over
- Score increments on successful obstacle pass

---

## Graphics Optimization

Major focus of this project was reducing display flicker.

### Issues Encountered

- Full-screen redraw caused visible flicker
- SPI bandwidth limitations exposed partial frame updates

### Solutions Implemented

- **Partial redraw (dirty regions)**
- **Separation of static vs dynamic UI**
- **Canvas-based rendering**
- **PSRAM-enabled frame buffer**

These improvements significantly increased visual stability and responsiveness.

---

## System Performance

Final system achieved:

- Smooth stopwatch display with no flicker
- Responsive touch input after coordinate mapping
- Stable game animation (~40 FPS)
- Clean UI transitions between modes
- Reliable collision detection and score tracking

The system successfully balances graphical performance with embedded constraints.

---

## Engineering Challenges & Solutions

### Issue: Screen Flickering

- **Cause:** Full-screen redraw over SPI
- **Solution:** Partial updates + canvas rendering

### Issue: Misaligned Touch Input

- **Cause:** Portrait coordinate system
- **Solution:** Implemented coordinate mapping

### Issue: Game Lag / Slow Rendering

- **Cause:** Large buffer operations without PSRAM
- **Solution:** Enabled PSRAM and optimized frame size

---

## Results & Validation

The final system demonstrates:

- Reliable touch-based UI interaction
- Stable graphical rendering under real-time constraints
- Successful integration of SPI and I2C subsystems
- Functional multi-mode embedded application

The project validates techniques for building responsive graphical interfaces on microcontrollers.

---

## Future Improvements

- Double buffering for further rendering stability
- More advanced UI elements and animations
- Improved game mechanics
- Wireless features (WiFi/Bluetooth)
- Battery-powered standalone version

---

## Skills Demonstrated

- Embedded systems design (ESP32)
- SPI display interfacing
- I2C touch communication
- Real-time graphics rendering
- Performance optimization
- State-machine design
- Debugging hardware/software interaction
