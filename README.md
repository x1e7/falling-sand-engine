# Falling Sand Engine (SIMULATION)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://isocpp.org/)

A real-time 2D particle sandbox simulation with physics-based interactions. Features sand, water, and walls with realistic movement behaviors.

## Screenshots

![Screenshot](sandbox.png)

## Features

- **Sand** - Falls downward, rolls down slopes
- **Water** - Flows downward and sideways, can be displaced by sand
- **Walls** - Static obstacles that block particle movement
- Real-time world editing (LMB/RMB)
- 60 FPS rendering

## TODO

- [ ] Background color (dark parchment theme)
- [ ] Particle velocity system (inertia, momentum)
- [ ] Dirty rectangles optimization (render only changed pixels)
- [x] Variable brush size
- [ ] Pause simulation (Spacebar)
- [ ] Particle counter (total particles on screen)
- [ ] FPS counter overlay

### Core Physics
- [ ] Liquid pressure system (water spreads faster under pressure)
- [ ] Surface tension for water
- [ ] Temperature system (heat transfer between particles)
- [ ] State changes: Water → Steam, Sand → Glass
- [x] Density-based layering (oil floats on water)
- [ ] Gas pressure system (smoke rises, expands)

### New Content
- [ ] Lava (melts walls, ignites wood)
- [ ] Wood (burns, creates ash)
- [ ] Oil (floats on water, ignites)
- [ ] Acid (dissolves most materials)
- [ ] Plant/Seed (grows on soil)
- [x] New particles

### Advanced Physics
- [ ] Explosion physics (shockwave, debris)
- [ ] Wind simulation (directional airflow)
- [ ] Fluid simulation (SPH for liquids)
- [ ] Soft bodies (gelatin, slime)
- [ ] Temperature system (hot/cold particles)

### Visual & UI
- [ ] Zoom (scroll wheel)
- [ ] Pan (middle mouse drag)
- [ ] Particle picker UI (click to select material)
- [ ] Particle info tooltip (hover for details)
- [ ] Grid overlay toggle
- [x] Save/load worlds

### Gameplay
- [ ] Campaign/Level system (firefighting, escape)
- [ ] Sandbox tools (fill, clear, copy/paste)
- [ ] Undo/Redo system
- [ ] Recording/Replay of simulations

### Performance & Polish
- [ ] Multithreaded physics (parallel particle updates)
- [ ] GPU acceleration (OpenCL/CUDA for physics)
- [ ] WebAssembly port (Emscripten)
- [ ] Save to IndexedDB (web persistence)

### Long-term
- [ ] Multiplayer (real-time sharing)
- [ ] Modding API (custom particles, behaviors)
- [ ] Mobile support (touch controls)

## Controls

| Key | Action |
|-----|--------|
| `1` | Select Sand |
| `2` | Select Water |
| `3` | Select Fire |
| `4` | Select Wall |
| `CTRL + S` | Save World |
| `CTRL + L` | Load World |
| `LMB` | Place particle |
| `RMB` | Delete particle |
| `ESC` | Exit |
