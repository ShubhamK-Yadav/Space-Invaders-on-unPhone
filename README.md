# Space Invaders on unPhone
## Overview
This project implements the classic Space Invaders arcade game in C++ for the unPhone handheld device. It was developed using PlatformIO inside Visual Studio Code and demonstrates game logic, rendering, and input handling on motion controls via the built-in accelerometer.

### Features
#### Classic gameplay mechanics:
- Player shooting and enemy formations.
- Collision detection between bullets and enemies.

#### Accelerometer-based controls:
- Tilt the unPhone left or right to move the spaceship in the corresponding direction.
- Optimised for unPhone hardware:

#### Efficient LCD rendering.
- Smooth input response from the accelerometer.

#### Embedded development workflow:
- Built and deployed via PlatformIO.

### Requirements
- PlatformIO extension installed in Visual Studio Code.
- unPhone device with USB connection.
- C++17-compatible toolchain (provided by PlatformIO environment).

### Installation & Usage
1. Clone the repository:
```
git clone https://github.com/yourusername/space-invaders-unphone.git
cd space-invaders-unphone
```
2. Open in Visual Studio Code.
3. Install PlatformIO (if not already installed).
4. Connect your unPhone via USB.
5. Build & Upload:
```
pio run --target upload
```

### Controls
- Tilt Left / Right: Move the spaceship horizontally using the accelerometer.
- Fire button: Shoot bullets at enemies.
