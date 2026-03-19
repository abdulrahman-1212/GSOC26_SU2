# PIML Demo — Physics-Informed Machine Learning with SU2

A working demonstration of the GSoC 2026 project
**"Towards Physics-Informed Machine Learning with SU2"** (mentor: Evert Bunschoten).

Trains an MLP to predict turbulent eddy viscosity **ν_t** from flow features **(k, ω, U)**
using a composite loss that combines a data MSE term with the Boussinesq physics residual.
The forward pass runs through **MLPCpp** (`CNeuralNetwork`) and gradients are computed by
**CoDiPack** (`codi::RealReverse`) reverse-mode AD — the same two libraries already present
in the SU2 source tree.

---

## Project layout

```
~/GSoC26/demo/
├── externals/
│   ├── MLPCpp/          # MLP inference library (Bunschoten et al.)
│   └── CoDiPack/        # Algorithmic differentiation library
├── src/
│   ├── main.cpp             # Driver: dataset → generate .mlp → train → save
│   ├── CMLPTrainer.hpp      # AD training loop (core contribution)
│   ├── CAdam.hpp            # Adam optimiser
│   ├── CDataset.hpp         # Synthetic SST turbulence data generator
│   └── generate_mlp_file.hpp# Writes He-initialised .mlp ASCII file
├── tests/
│   └── test_piml.cpp        # Unit + integration tests (no external framework)
├── Makefile
└── README.md
```

---

## Dependencies

| Library   | Version | Role                                    | Location                  |
|-----------|---------|-----------------------------------------|---------------------------|
| MLPCpp    | 2.1.1   | MLP forward pass, weight I/O, .mlp I/O | `externals/MLPCpp/`       |
| CoDiPack  | 3.1.0   | Reverse-mode AD (tape, gradients)       | `externals/CoDiPack/`     |
| g++       | ≥ 9     | C++17                                   | system                    |

No Python, no external ML frameworks, no additional installs beyond what SU2 already ships.

---

## Build and run

All commands are run from `~/GSoC26/demo/`.

```bash
make              # build main demo  (piml_demo)
make test         # build test suite (test_piml)
make run          # build + run demo
make run_test     # build + run all tests
make all          # build both
make clean        # remove binaries and generated .mlp files
```

The critical compiler flag added by the Makefile is:

```
-DMLP_CUSTOM_TYPE="codi::RealReverse"
```

This replaces MLPCpp's internal `mlpdouble` typedef with `codi::RealReverse` everywhere
inside `CNeuralNetwork` — so `GetWeightsBiases()`, `SetWeightsBiases()`, `Predict()`, and
`GetOutput()` all operate on `codi::RealReverse` instead of `double`. No changes to MLPCpp
source are required.

---

## How it works

### Loss function

```
L_total = L_data + λ · L_phys

L_data  = ‖ ν_t_pred − ν_t_ref ‖²           (MSE against SST reference)
L_phys  = ‖ ν_t_pred · ω − k  ‖² / k²       (Boussinesq residual, normalised)
```

`λ = 0.3` by default (set in `CMLPTrainer::Config`). The physics term encodes the SST
model relation `ν_t · ω = k` as a soft constraint, so the trained MLP is physically
consistent by construction even in regions with sparse data.

### AD training loop (`CMLPTrainer::train_batch`)

```
1.  wb_ad   = network.GetWeightsBiases()          // vector<ADType> (mlpdouble)
2.  params  = to_double(wb_ad)                    // extract plain doubles via .getValue()
3.  tape.setActive()
4.  active[i] = params[i]; tape.registerInput(active[i])  // mark weights as tape inputs
5.  network.SetWeightsBiases(active)              // inject active weights into network
6.  network.SetInput(to_ad(x)); network.Predict() // forward pass — all ops recorded on tape
7.  nu_pred = network.GetOutput(0)                // ADType output
8.  loss    = loss_ad(nu_pred, nu_ref, k, omega, λ)  // ADType loss expression
9.  tape.registerOutput(loss); loss.gradient() = 1.0  // seed the reverse sweep
10. tape.evaluate()                               // dL/dw computed for every registered input
11. grads[i] = active[i].gradient()              // read out dL/dw
12. tape.reset()
13. adam.step(params, grads)                      // Adam weight update
14. network.SetWeightsBiases(to_ad(params))       // write updated weights back
```

### Type conversion helpers

Because `mlpdouble == codi::RealReverse` when compiled with `-DMLP_CUSTOM_TYPE`,
all MLPCpp API calls require `vector<ADType>`. Two inline helpers handle the conversion:

```cpp
// vector<ADType> -> vector<double>  (extract primal value from each element)
std::vector<double> to_double(const std::vector<ADType>& v);

// vector<double> -> vector<ADType>  (promote each double to passive ADType)
std::vector<ADType> to_ad(const std::vector<double>& v);
```

### Network architecture

```
Input (3)  →  Hidden (8, ReLU)  →  Hidden (8, ReLU)  →  Output (1, linear)
  k                                                          ν_t
  ω          113 trainable parameters
  U          MinMax normalisation handled internally by MLPCpp ScalerFunction
```

### Data

`CDataset` generates synthetic samples satisfying `ν_t = 0.09 · k / ω` (SST Boussinesq)
with 2 % Gaussian noise — mimicking what SU2's SST solver would write to a VTU restart
file. Min/max bounds from the dataset are baked into the `.mlp` file header so MLPCpp's
built-in `MinMaxScaler` handles normalisation automatically during `Predict()`.

### Inference (no AD overhead)

During evaluation, the tape is paused with `tape.setPassive()` so `Predict()` runs as
plain arithmetic without recording:

```cpp
tape.setPassive();
network.SetInput(to_ad(x));   // inputs are passive — only weights are ever active
network.Predict();
double out = network.GetOutput(0).getValue();
tape.setActive();
```

---

## Output

After training the demo prints a convergence table and saves two files:

| File                         | Contents                                       |
|------------------------------|------------------------------------------------|
| `piml_network.mlp`           | Initial He-initialised weights — MLPCpp ASCII  |
| `piml_network_trained.mlp`   | Trained weights — loadable directly by SU2     |

The trained `.mlp` file can be loaded immediately by SU2's `CDataDrivenFluid` class via
`CLookUp_ANN`.

---

## Test suite

`tests/test_piml.cpp` contains 9 tests with no external framework. Build and run with
`make run_test`. All type conversions between `double` and `ADType` are handled correctly
so the tests compile with the same `-DMLP_CUSTOM_TYPE` flag as the main demo.

| Test | What it verifies |
|------|------------------|
| `test_dataset` | Boussinesq relation holds (no noise), bounds consistent, perfect R²=1, perfect physics RMS=0 |
| `test_adam` | Converges to known minimum, `reset()` clears state, size mismatch throws |
| `test_loss_double` | `loss_d()` gives zero for perfect prediction, λ scales physics term correctly |
| `test_codi_gradient` | CoDi reverse sweep matches finite difference on a scalar function |
| `test_mlp_file_io` | Architecture/names/activations load correctly, `ADType` weight round-trip, `WriteNeuralNetwork` preserves weights |
| `test_codi_through_network` | CoDi gradients through `CNeuralNetwork::Predict` match finite differences for 15 sampled weights |
| `test_trainer_converges` | R² improves and exceeds 0.5 after 100 epochs |
| `test_lookup_ann` | `CLookUp_ANN::Predict` via `CIOMap`, `mlpdouble` weight round-trip |
| `test_adam_analytic` | Adam reaches known minimum of 2D quadratic to tight tolerance |

---

## Connection to SU2

| Demo component         | SU2 counterpart                                    |
|------------------------|----------------------------------------------------|
| `CMLPTrainer`          | New class in `SU2_CFD/include/fluid/`              |
| `CAdam` / `CAdamConfig`| New classes in `Common/include/`                   |
| `CNeuralNetwork`       | Already in `externals/MLPCpp/` (unchanged)         |
| `codi::RealReverse`    | Already in `externals/CoDiPack/` (unchanged)       |
| `CDataset`             | Replaced by SU2 VTU / restart file reader          |
| `generate_mlp_file`    | Replaced by existing `.mlp` files from offline training |

The global CoDi tape used here (`ADType::getTape()`) is the same tape used by SU2's
adjoint solver. No new AD infrastructure is introduced.

---

## Configuration

All hyperparameters are set in `main.cpp` via `CMLPTrainer::Config`:

```cpp
CMLPTrainer::Config cfg;
cfg.n_epochs    = 400;    // training epochs
cfg.batch_size  = 64;     // mini-batch size
cfg.lambda      = 0.3;    // physics loss weight (0 = data only)
cfg.print_every = 50;     // reporting interval
cfg.adam.alpha  = 3e-3;   // Adam learning rate
cfg.adam.beta1  = 0.9;    // Adam 1st-moment decay
cfg.adam.beta2  = 0.999;  // Adam 2nd-moment decay
```

---

## Known compiler notes

- GCC rejects `= {}` as a default argument for structs that use in-class member
  initialisers when the struct is nested inside the class that uses it as a default
  argument. This is worked around by defining `CAdamConfig` and `CAdamTrainerConfig`
  at file scope before the classes that use them.
- The `-Wcomment` warning on multi-line `\` in comments is suppressed by avoiding
  backslash-continued comment lines in all source files.

---

## References

- Raissi, Perdikaris & Karniadakis (2019). *Physics-informed neural networks.* JCP 378.
- Kingma & Ba (2015). *Adam: A method for stochastic optimization.* ICLR 2015.
- Bunschoten et al. MLPCpp v2.1.1. https://github.com/EvertBunschoten/MLPCpp
- Sagebaum, Albring & Gauger. CoDiPack 3.1. https://scicomp.rptu.de/software/codi
