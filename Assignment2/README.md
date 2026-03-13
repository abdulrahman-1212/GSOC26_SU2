# Axisymmetric Turbulent Jet Simulation using SU2

## 1. Introduction and Motivation

Axisymmetric turbulent jets are fundamental flows in fluid mechanics and appear in many engineering applications such as propulsion systems, fuel injection, combustion devices, ventilation systems, and environmental flows. Understanding the mixing behavior of turbulent jets is important for predicting momentum diffusion, scalar transport, and jet spreading characteristics.

The purpose of this simulation is to study the behavior of a **subsonic turbulent jet** using the open-source CFD solver **SU2** and to visualize the flow field using **ParaView**. The simulation models an **axisymmetric turbulent jet** using a Reynolds-Averaged Navier–Stokes (RANS) formulation with the **SST turbulence model**, which is commonly used for flows with shear layers and separation.

The main objectives of this study are:

* Simulate the development of an axisymmetric turbulent jet.
* Analyze the convergence behavior of the solver.
* Generate flow-field visualization data.
* Compare qualitative jet characteristics with experimental observations reported in literature.

---

## 2. Numerical Method

The simulation is performed using the **Reynolds-Averaged Navier–Stokes (RANS)** equations. Turbulence is modeled using the **Shear Stress Transport (SST)** model, which combines the advantages of the (k\text{-}\omega) and (k\text{-}\varepsilon) models and provides improved accuracy in predicting shear-layer flows such as jets.

The flow solver uses:

* Finite-volume discretization
* Implicit time stepping
* Upwind convection schemes
* Adaptive CFL control for stability

The simulation assumes **axisymmetric flow**, allowing the 3D jet to be modeled using a reduced computational domain.

---

## 3. Configuration Overview

### 3.1 Core Simulation Parameters

| Parameter        | Value | Description                     |
| ---------------- | ----- | ------------------------------- |
| Solver           | RANS  | Reynolds-averaged Navier–Stokes |
| Turbulence model | SST   | Shear Stress Transport model    |
| Axisymmetric     | Yes   | Axisymmetric jet assumption     |
| Mesh format      | SU2   | Native mesh format              |

The mesh used is:

```
jet_mesh.su2
```

---

### 3.2 Physical Conditions

| Parameter       | Value                        |
| --------------- | ---------------------------- |
| Mach number     | 0.15                         |
| Reynolds number | 50,000                       |
| Reynolds length | 0.01 m                       |
| Reference area  | (7.854 \times 10^{-5} , m^2) |

The jet is therefore **subsonic and moderately turbulent**, representing a typical laboratory jet configuration.

---

### 3.3 Boundary Conditions

The computational domain contains the following boundaries:

| Boundary | Type              | Description                                  |
| -------- | ----------------- | -------------------------------------------- |
| inlet    | Velocity inlet    | Specifies velocity, pressure and temperature |
| outlet   | Pressure outlet   | Allows flow to exit                          |
| symmetry | Symmetry plane    | Axisymmetric condition                       |
| wall     | Isothermal wall   | Constant temperature wall                    |
| farfield | Farfield boundary | External domain boundary                     |

Turbulence quantities at the inlet are defined by:

* Turbulence intensity = **1%**
* Turbulent viscosity ratio = **3**

These values represent a **moderately turbulent inflow condition**.

---

### 3.4 Numerical Schemes

The following discretization methods are used:

| Method                       | Scheme                 |
| ---------------------------- | ---------------------- |
| Gradient reconstruction      | Weighted Least Squares |
| Convective flux (flow)       | AUSM                   |
| Convective flux (turbulence) | Scalar Upwind          |

For numerical stability, the simulation starts with:

* First-order spatial accuracy
* No slope limiter
* Entropy fix coefficient = 0.2

---

### 3.5 Time Integration

The solver uses an **implicit Euler time discretization** with adaptive CFL control:

| Parameter    | Value     |
| ------------ | --------- |
| CFL number   | 0.3       |
| CFL adaptive | Enabled   |
| CFL growth   | 0.5 → 1.5 |

The low starting CFL improves solver robustness during early iterations.

---

### 3.6 Material Model

The fluid is modeled as an **ideal gas**:

| Parameter    | Value                     |
| ------------ | ------------------------- |
| Gas constant | 287.058 J/(kg·K)          |
| Gamma        | 1.4                       |
| Viscosity    | (1.8 \times 10^{-5}) Pa·s |

---

### 3.7 Linear Solver

The linear system arising from the implicit formulation is solved using:

| Parameter       | Value     |
| --------------- | --------- |
| Linear solver   | BCGSTAB   |
| Preconditioner  | Jacobi    |
| Max iterations  | 100       |
| Error tolerance | (10^{-3}) |

---

## 4. Convergence History

The simulation was run for:

```
ITER = 30000
```

The monitored convergence variable was:

```
RMS density residual
```

Final solver output:

| Residual | Value    |
| -------- | -------- |
| rms[Rho] | -1.22425 |
| Target   | < -4     |

Therefore:

```
Convergence criterion NOT satisfied.
```

The solver stopped because the **maximum iteration limit was reached**.

### Observed Behavior

From the terminal output:

* Residual oscillates between **−0.9 and −2.8**
* Residual does **not decrease monotonically**
* Flow likely reached a **quasi-steady state but not strict residual convergence**

Possible causes:

* First-order numerical scheme
* Low CFL number
* Turbulence model stiffness
* Insufficient mesh resolution

Further convergence improvement could be achieved by:

* Increasing CFL gradually
* Switching to **second-order MUSCL reconstruction**
* Increasing mesh resolution
* Extending iteration count

---

## 5. Output Data

At the end of the simulation, the following files were generated:

| File              | Description                   |
| ----------------- | ----------------------------- |
| `restart_jet.dat` | Binary restart solution       |
| `flow.vtu`        | Flow field visualization file |

The `.vtu` file can be visualized using **ParaView** to observe:

* Velocity field
* Pressure distribution
* Turbulence structures
* Jet spreading

---

## 6. Comparison with Experimental Data

The simulated jet behavior can be compared with experimental measurements reported in:

**Investigation of the Mixing Process in an Axisymmetric Turbulent Jet Using PIV and LIF**
https://www.researchgate.net/publication/254224677_Investigation_of_the_Mixing_Process_in_an_Axisymmetric_Turbulent_Jet_Using_PIV_and_LIF

This study used **Particle Image Velocimetry (PIV)** and **Laser-Induced Fluorescence (LIF)** to analyze the mixing process in turbulent jets.

### Key experimental observations

Typical axisymmetric turbulent jets exhibit:

1. **Potential core region** near the nozzle
2. **Shear layer growth**
3. **Self-similar velocity decay**
4. **Radial spreading of the jet**

### Expected numerical results

The CFD simulation should reproduce:

* Jet centerline velocity decay
* Radial velocity profile spreading
* Development of turbulent mixing layer

Due to the use of **RANS modeling**, small-scale turbulent structures are not directly resolved, but the **mean flow behavior** should match experimental trends.

---

## 7. Visualization

The results can be visualized in **ParaView** by loading:

```
flow.vtu
```

Recommended visualization techniques:

* Velocity magnitude contours
* Streamlines of the jet
* Turbulent viscosity fields
* Axial velocity profiles

These plots allow observation of:

* Jet core structure
* Turbulent mixing region
* Flow spreading angle

---

## 8. Conclusion

This project simulated an **axisymmetric turbulent jet** using **SU2** with the SST turbulence model.

Key observations:

* The solver ran successfully for 30,000 iterations.
* Convergence criterion was not fully satisfied.
* Flow solution files were generated for visualization.
* The predicted jet structure is expected to qualitatively match experimental observations from PIV/LIF studies.

Future improvements include:

* Using second-order schemes
* Refining the mesh
* Increasing iteration count
* Performing quantitative comparison with experimental velocity profiles.

---

## 9. Reference

Investigation of the Mixing Process in an Axisymmetric Turbulent Jet Using PIV and LIF
https://www.researchgate.net/publication/254224677_Investigation_of_the_Mixing_Process_in_an_Axisymmetric_Turbulent_Jet_Using_PIV_and_LIF
