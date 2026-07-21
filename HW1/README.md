# HW1 — Mass-Spring Particle System (Jelly Simulation)

From-scratch C++/OpenGL soft-body simulator: a jelly cube built from a 3D grid of
particles connected by structural, shear, and bending springs, integrated over time
and collided against an inclined-plane terrain. Rendered with custom GLSL shaders
(particle/render/shadow/skybox) and controlled through an ImGui panel.

## Key files

- `code/main.cpp` — render loop + ImGui UI (Camera / Simulation / Recording / Debug panels)
- `code/src/simulation/jelly.*` — particle/spring topology and internal force computation
- `code/src/simulation/integrator.*` — Explicit Euler, semi-implicit (symplectic) Euler,
  plus bonus 4th-order Runge-Kutta and Position-Based Dynamics with XSPH viscosity
- `code/src/simulation/terrain.*` — plane collision with restitution + friction
- `Particle System.pdf` — assignment brief; `report_113950011.pdf` — written report

## Build & run

```powershell
cd code
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make
.\main.exe
```

Alternatively, open `code/SoftSim/SoftSim.vcxproj` in Visual Studio. Third-party
libraries (Eigen, GLFW, glad, Dear ImGui, stb) are vendored under `code/vendor/`.

## Results

See `report_113950011.pdf` for the write-up and simulation results.
