# Chess Engine & GUI (C++)

## Overview

This project is a **Chess GUI and Engine written in C++** using **OpenGL and ImGui**.
The application renders a chessboard, handles piece movement, legal move generation, and integrates with a chess engine for gameplay.

This project is part of my C++ and game engine learning, focusing on:

* C++
* OpenGL rendering
* Chess engine logic
* Bitboards and move generation
* GUI using ImGui
* CMake build system

---

## Features

* Chessboard GUI
* Piece movement
* Legal move highlighting
* Bitboard-based move generation
* OpenGL rendering
* ImGui interface
* Windows build using CMake + MinGW / MSVC
* Linux build support

---

## Tech Stack

* **Language:** C++17
* **Graphics:** OpenGL 4.6
* **GUI:** ImGui
* **Windowing:** GLFW
* **OpenGL Loader:** GLAD
* **Math Library:** GLM
* **Build System:** CMake

---

## Build Instructions

### Requirements

* C++17 compiler
* CMake
* OpenGL 4.6
* MinGW / MSVC (Windows) or GCC/Clang (Linux)

### Clone Repository

```bash
git clone https://github.com/Notshreyasrbhat/Chess.git
cd Chess
```

### Build using CMake

```bash
cmake -B build
cmake --build build
```

### Build Release (Visual Studio)

```bash
cmake --build build --config Release
```

---

## Project Structure

```
Chess/
│
├── src/
├── assets/
├── resources/
├── build/
├── CMakeLists.txt
└── README.md
```

---

## Learning Goals

This project helped me learn:

* Large C++ project structure
* Rendering with OpenGL
* ImGui integration
* Chess programming concepts
* Bitboards
* CMake build systems
* Working with large codebases

---

## Credits

OpenGL and rendering structure inspired by **The Cherno Game Engine Series**
Chess programming concepts from **Chess Programming Wiki**

Libraries used:

* GLFW
* GLAD
* stb_image
* GLM
* ImGui

