# 🌆 Interactive 3D City Simulation

An interactive 3D city environment developed using **C++, OpenGL, and FreeGLUT** as a Computer Graphics project. The simulation represents a modern urban environment with detailed buildings, roads, moving vehicles, traffic signals, pedestrians, and environmental elements.

The project also features dynamic **Day/Night transitions**, weather effects, and interactive camera controls to create an engaging 3D experience.

---

## ✨ Features

### 🏙️ 3D City Environment
- Detailed 3D buildings and urban structures.
- Shopping mall, hospital, school, and mosque.
- Roads, sidewalks, pedestrian crossings, and street furniture.
- Trees, flowers, grass, and other environmental elements.
- Footbridge and additional city landmarks.

### 🚗 Traffic Simulation
- Animated vehicles moving along city roads.
- Traffic signal system with coordinated light phases.
- Interactive traffic light controls.
- Vehicle movement and speed adjustment.

### 🌞 Dynamic Day & Night Cycle
- Smooth transition between daytime and nighttime.
- Dynamic sky color and lighting changes.
- Sun and moon visual effects.
- Different lighting atmospheres for day and night.

### 🌧️ Weather Effects
- Rain particle simulation.
- Snow particle simulation.
- Clear weather mode.
- Dynamic weather changes during the simulation.

### 🎮 Interactive Camera System
- Multiple camera presets.
- Camera rotation, movement, and zoom.
- Mouse-based camera interaction.
- Automatic camera orbit.
- Reset camera functionality.

### 🌳 Environmental Animation
- Animated pedestrians.
- Flying birds.
- Moving clouds.
- Additional environmental details that make the city feel alive.

---

## 🛠️ Technologies Used

| Technology | Purpose |
|---|---|
| C++ | Core programming language |
| OpenGL | 3D graphics rendering |
| FreeGLUT | Window management and user interaction |
| GLU | Graphics utility functions |
| Code::Blocks | Development environment |
| GCC / MinGW | C++ compiler |

---

## 🎮 Controls

| Key | Action |
|---|---|
| `D` | Switch to Day mode |
| `N` | Switch to Night mode |
| `B` | Enable Rain |
| `P` | Enable Snow |
| `C` | Clear Weather |
| `R` | Reset Camera |
| `Space` | Pause / Resume simulation |
| `H` | Show / Hide HUD |
| `T` | Toggle Automatic Camera Orbit |
| `W` | Increase vehicle speed |
| `S` | Decrease vehicle speed |
| `+` / `-` | Zoom In / Zoom Out |
| `W` / `A` / `S` / `D` | Camera movement where applicable |
| `Arrow Keys` | Rotate camera |
| `Page Up` / `Page Down` | Zoom camera |
| `1–8` | Switch camera presets |
| `Esc` | Exit simulation |

**Note:** Some controls may have context-dependent behavior. For example, `W`, `A`, `S`, and `D` are also used for camera movement in the original camera system.

---

## ⚙️ Installation and Setup

### Requirements

Before running the project, make sure you have:

- Windows operating system.
- Code::Blocks IDE.
- GCC / MinGW compiler.
- OpenGL libraries.
- FreeGLUT library.

### Step 1: Clone the Repository

Open your terminal and run:

```bash
git clone YOUR_REPOSITORY_URL
```

Replace `YOUR_REPOSITORY_URL` with your GitHub repository URL.

### Step 2: Open the Project

1. Open Code::Blocks.
2. Select **File → Open**.
3. Open the project file (`.cbp`) if available.
4. Otherwise, create a C++ project and add `main.cpp`.

### Step 3: Configure Libraries

Make sure the following libraries are linked in your project:

```text
freeglut
opengl32
glu32
```

Also configure the FreeGLUT include and library directories if they are not already configured.

### Step 4: Build and Run

Build the project and run it using Code::Blocks.

The city simulation window should appear once compilation is successful.

---

## 📸 Project Overview

The simulation combines 3D modeling, lighting, animation, and user interaction to represent a dynamic urban environment.

**Main components:**
- City infrastructure and buildings
- Road and traffic management
- Animated vehicles and pedestrians
- Day/Night lighting system
- Rain and snow particle effects
- Interactive camera system

---

## 🎯 Project Objectives

- Apply fundamental computer graphics concepts in a practical project.
- Develop a three-dimensional urban environment.
- Implement object modeling, transformations, and lighting.
- Simulate vehicle movement and traffic signals.
- Demonstrate dynamic environmental effects.
- Build an interactive graphics application using keyboard and mouse controls.

---

## 🚀 Future Improvements

- More realistic vehicle physics and traffic behavior.
- Improved building interiors and architectural details.
- Additional weather conditions.
- More advanced lighting and shadow effects.
- Environmental sound effects.
- Expanded city infrastructure and interactive elements.

---

## 👨‍💻 Project Information

**Project Title:** Interactive 3D City Simulation

**Course:** Computer Graphics Lab

**Language:** C++

**Graphics Library:** OpenGL & FreeGLUT

**Development Environment:** Code::Blocks

---

⭐ If you find this project interesting, consider giving the repository a star!

