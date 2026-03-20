# Assignment 4 — Spatially Varying Wall Temperature via Python Wrapper

**GSoC 2026 | SU2 Physics-Informed Machine Learning**
**Solver:** SU2 v8.1.0 | **Turbulence Model:** Spalart-Allmaras (RANS)

---

## Overview

This assignment demonstrates the use of the SU2 Python wrapper (`pysu2`) to impose a **spatially varying isothermal wall boundary condition** on a steady-state compressible turbulent flat plate. Rather than a uniform wall temperature, the plate temperature increases linearly from the leading edge to the trailing edge, coupling the thermal and turbulent boundary layers in a physically richer configuration.

---

## Problem Setup

| Parameter | Value |
|---|---|
| Solver | RANS + Spalart-Allmaras |
| Mach Number | 0.2 |
| Angle of Attack | 0° |
| Freestream Pressure | 101,325 Pa |
| Freestream Temperature | 293.15 K |
| Reynolds Number | 5 × 10⁶ |
| Reynolds Length | 1.0 m |
| Wall Temperature (leading edge) | 300 K |
| Wall Temperature (trailing edge) | 500 K |
| Mesh | `mesh_flatplate.su2` |
| Max Iterations | 5000 |
| Convergence Target | RMS residual < 10⁻⁸ |

---

## Repository Structure

```
Assignment4/
├── turb_SA_flatplate.cfg       # SU2 configuration file
├── run_simulation.py           # Python wrapper (pysu2) with spatially varying T_wall
├── mesh_flatplate.su2          # Flat plate mesh
├── outputs/
│   └── flow.vtu                # Volume output for ParaView
└── images/
    ├── velocity.png             # Velocity field visualization
    └── temperature.png          # Temperature field visualization
```

---

## Spatially Varying Wall Temperature

The Python wrapper (`run_simulation.py`) uses `pysu2.CSinglezoneDriver` to intercept the solver before the run and apply a custom temperature profile to each vertex on the `plate` marker.

The temperature distribution follows a linear ramp:

$$T_{wall}(x) = 300 + 200 \cdot \frac{x}{L_{plate}}$$

where $L_{plate} = 0.01687$ m is the plate length. This gives:

- **Leading edge** ($x = 0$): $T_{wall} = 300$ K
- **Trailing edge** ($x = L$): $T_{wall} = 500$ K

The key wrapper logic:

```python
for i_vert in range(n_vertices):
    coords = driver.MarkerCoordinates(marker_id)
    x_coord = coords(i_vert, 0)
    normalized_x = np.clip(x_coord / plate_length, 0.0, 1.0)
    temperature = 300.0 + 200.0 * normalized_x
    driver.SetMarkerCustomTemperature(marker_id, i_vert, temperature)
```

---

## How to Run

### Prerequisites

- SU2 v8.1.0 with Python bindings (`pysu2`)
- `mpi4py`
- `numpy`

### Single process

```bash
python3 run_simulation.py
```

### MPI (parallel)

```bash
mpirun -n 4 python3 run_simulation.py
```

### Output

The solver writes volume output to `outputs/flow.vtu` every 500 iterations. Open in ParaView:

- **Windows (WSL users):** navigate to `\\wsl$\Ubuntu\root\GSoC26\Assignment4\outputs\` in ParaView's file dialog.
- **Linux:** `paraview outputs/flow.vtu`

---

## Results

### Velocity Field

![Velocity Field](images/velocity.png)

The boundary layer grows along the plate in the streamwise direction. The no-slip condition at the wall is clearly captured, with a smooth velocity gradient from zero at the wall to the freestream value. The Spalart-Allmaras model accurately resolves the turbulent boundary layer thickness evolution downstream.

### Temperature Field

![Temperature Field](images/temperature.png)

The thermal boundary layer closely follows the velocity boundary layer structure. The spatially varying wall temperature produces a progressively hotter near-wall region toward the trailing edge, consistent with the imposed linear ramp from 300 K to 500 K. Heat diffuses outward from the wall into the freestream, with the thermal layer thickening downstream as expected for a heated flat plate under forced convection.

---

## Key Configuration Notes

- `MARKER_PYTHON_CUSTOM= ( plate )` — exposes the marker to the Python wrapper for custom BC injection.
- `MARKER_ISOTHERMAL= ( plate, 300.0 )` — sets a fallback uniform temperature; this is **overridden at runtime** by `SetMarkerCustomTemperature()` in the wrapper.
- `FLUID_MODEL= STANDARD_AIR` with Sutherland viscosity law accounts for temperature-dependent viscosity, which is physically important given the large wall temperature variation.
- `PRANDTL_TURB= 0.90` governs turbulent heat flux in the energy equation.

---

## Numerical Scheme Summary

| Setting | Value |
|---|---|
| Convective Flux | Roe scheme |
| Reconstruction | MUSCL with Venkatakrishnan limiter |
| Time Integration | Implicit Euler |
| Gradient Method | Weighted Least Squares |
| CFL Number | 5.0 |
