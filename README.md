# Falling Sand Engine (SIMULATION)
<p align="center">
    <a href="https://isocpp.org/"><img alt="C++" src="https://img.shields.io/badge/C%2B%2B-17-blue"/></a>
    <img alt="size" src="https://img.shields.io/github/repo-size/x1e7/falling-sand-engine"/>
</p>

A real-time 2D particle sandbox simulation with physics-based interactions. Features sand, water, and walls with realistic movement behaviors.

## Screenshots

![Screenshot](screenshots/16_07_26.jpg)
*Preview (16/7/26)*

![Screenshot](screenshots/02_09_26.png)
*Preview (2/9/26)*

![Screenshot](screenshots/17_09_26.png)
*Preview (17/9/26)*

## Build

```bash
cmake -B build -G Ninja
cmake --build build
```

## TODO

### v0.2 — Performance (in progress)
- [ ] Multithreaded simulation
- [ ] Two-phase tick
- [ ] RNG optimization
- [ ] Direct cell access in renderer

### v0.3 — Physics
- [ ] Temperature field
- [ ] Reaction table
- [ ] Liquid pressure
- [ ] Phase transitions

### v0.4 — Tools
- [ ] Line/rect/fill tools
- [ ] Undo/redo
- [ ] Particle editor

### v0.5 — Scripting
- [ ] Lua bindings
- [ ] Demo scenes

### v1.0 — Release
- [ ] WASM port
- [ ] Documentation

## Controls

| Key | Action |
|-----|--------|
| `1` | Select Sand |
| `2` | Select Water |
| `3` | Select Fire |
| `4` | Select Wall |
| `5` | Select Oil |
| `LMB` | Place selected particle |
| `RMB` | Delete particle |
| `Mouse Wheel` | Adjust brush size |
| `Space` | Pause/Resume simulation |
| `ESC` | Exit |
