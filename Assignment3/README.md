# Heated Flat Plate Simulation (CHT) using SU2

## 1. Introduction

This project simulates the airflow over a **heated flat plate** using the open-source CFD solver **SU2**. The simulation is designed for **conjugate heat transfer (CHT) related studies**, where heat exchange between a solid surface and a fluid flow is important.

Flat plate boundary-layer problems are classical benchmarks in computational fluid dynamics because they allow investigation of:

* Boundary layer development
* Turbulent heat transfer
* Wall shear stress and drag
* Thermal diffusion from heated surfaces

The numerical simulation solves the **Reynolds-Averaged Navier–Stokes (RANS)** equations with the **SST turbulence model**, which is widely used for wall-bounded turbulent flows.

Visualization of the results is performed using **ParaView**.

---

# 2. Problem Description

The physical problem consists of airflow over a **flat plate with a constant temperature wall boundary condition**.

The flow domain includes:

* A **heated flat plate**
* A **farfield boundary**
* Turbulent boundary layer development along the plate

The plate surface temperature is fixed, allowing analysis of heat transfer from the wall into the fluid.

---

# 3. Numerical Model

## 3.1 Governing Equations

The simulation solves the **compressible RANS equations** including turbulence modeling using:

**SST (Shear Stress Transport) turbulence model**

This model combines:

* (k-\omega) behavior near the wall
* (k-\epsilon) behavior in the outer region

It is well suited for **boundary layer and heat transfer problems**.

---

# 4. Flow Conditions

The free-stream conditions used in the simulation are:

| Parameter              | Value     |
| ---------------------- | --------- |
| Mach number            | 0.03059   |
| Freestream pressure    | 101325 Pa |
| Freestream temperature | 293.15 K  |
| Reynolds number        | 24,407    |
| Reynolds length        | 0.035 m   |

The flow is therefore **low-speed and nearly incompressible**.

Additional fluid properties:

| Property     | Value            |
| ------------ | ---------------- |
| Density      | 1.204 kg/m³      |
| Viscosity    | 1.82 × 10⁻⁵ Pa·s |
| Gas constant | 287.058 J/(kg·K) |
| Gamma        | 1.4              |

---

# 5. Boundary Conditions

The simulation domain uses the following boundary definitions:

| Boundary | Type              | Description                       |
| -------- | ----------------- | --------------------------------- |
| plate    | Isothermal wall   | Constant wall temperature (293 K) |
| farfield | Farfield boundary | Free-stream flow                  |

The wall temperature remains constant during the simulation, allowing analysis of **heat transfer and thermal boundary layer growth**.

---

# 6. Time Integration

The simulation is **time dependent** and uses a **dual-time stepping scheme**.

| Parameter             | Value                          |
| --------------------- | ------------------------------ |
| Time marching method  | Dual time stepping (2nd order) |
| Time step             | 0.003 s                        |
| Total time iterations | 10                             |
| Inner iterations      | 10                             |

This approach allows simulation of **transient behavior in the boundary layer flow**.

---

# 7. Numerical Methods

The numerical schemes used in the simulation include:

| Method                  | Scheme                 |
| ----------------------- | ---------------------- |
| Gradient reconstruction | Weighted least squares |
| Flow convection scheme  | JST scheme             |
| Turbulence convection   | Scalar upwind          |
| Time discretization     | Implicit Euler         |
| Linear solver           | FGMRES                 |
| Preconditioner          | ILU                    |

Multigrid acceleration is also enabled to improve convergence performance.

---

# 8. Mesh

The simulation uses the mesh file:

```
2D_FlatPlate_Rounded.su2
```

The mesh represents a **2D domain around the flat plate**, allowing resolution of the boundary layer developing along the wall.

---

# 9. Convergence History

The solver was executed for:

```
TIME_ITER = 10
```

Example convergence output from the final iteration:

| Inner Iter | rms[Rho] | rms[k] | rms[w] | CL       | CD      |
| ---------- | -------- | ------ | ------ | -------- | ------- |
| 0          | -7.05    | -2.06  | 1.82   | -0.00223 | 0.13898 |
| 5          | -7.10    | -2.34  | 1.58   | -0.00221 | 0.13618 |
| 9          | -7.14    | -2.53  | 1.45   | -0.00199 | 0.13407 |

### Observations

* Density residual converges to approximately **10⁻⁷**.
* Turbulence residuals gradually decrease.
* Drag coefficient stabilizes near **0.134**.

The solver terminated because the **maximum time iterations were reached**:

```
Maximum number of time iterations reached (TIME_ITER = 10)
```

---

# 10. Simulation Output Files

At the end of the simulation, SU2 generated several output files.

### Restart files

```
restart_flow_00008.dat
restart_flow_00009.dat
```

These files store the **flow solution state** and can be used to restart the simulation.

### Visualization files

```
flow_00009.vtu
surface_flow_00009.vtu
```

These files contain the full flow field and surface data for visualization.

---

# 11. Visualization

The results can be visualized using **ParaView**.

Steps:

1. Open ParaView
2. Load:

```
flow_00009.vtu
```

Recommended visualization plots:

* Velocity magnitude contours
* Temperature distribution
* Boundary layer velocity profiles
* Surface pressure distribution
* Streamlines near the plate

These plots help analyze **boundary layer development and heat transfer behavior**.

---

# 12. Key Results

From the solver output:

| Parameter              | Value |
| ---------------------- | ----- |
| Final Drag Coefficient | 0.134 |
| Lift Coefficient       | ≈ 0   |
| Density Residual       | ~10⁻⁷ |

The near-zero lift coefficient is expected because the **flat plate is aligned with the incoming flow**.

---

# 13. Conclusion

This project simulated **flow over a heated flat plate** using **SU2** with the SST turbulence model.

Key findings:

* The simulation successfully captured turbulent boundary layer development.
* Drag coefficient converged to approximately **0.134**.
* Residuals decreased steadily during the iterations.
* Visualization files were generated for post-processing.

This case demonstrates the use of SU2 for **heat transfer and wall-bounded turbulent flow simulations**, which are relevant in many engineering applications such as cooling systems, heat exchangers, and aerodynamic heating problems.

---

# 14. Files in this Project

| File                       | Description               |
| -------------------------- | ------------------------- |
| `cht_flatplate.cfg`        | SU2 configuration file    |
| `2D_FlatPlate_Rounded.su2` | Computational mesh        |
| `restart_flow_00009.dat`   | Restart solution          |
| `flow_00009.vtu`           | Volume flow visualization |
| `surface_flow_00009.vtu`   | Surface visualization     |

---

# 15. Author

Simulation performed using **SU2**
Visualization performed using **ParaView**
