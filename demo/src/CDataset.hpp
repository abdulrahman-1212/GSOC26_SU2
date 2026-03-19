#pragma once
// =============================================================================
// CDataset.hpp  -  Synthetic SST turbulence dataset
//
// Generates (k, omega, U) -> nu_t samples satisfying:
//   nu_t = C_mu * k / omega   (Boussinesq / SST, C_mu = 0.09)
//
// Also computes the min/max normalisation bounds that get written into the
// .mlp file header, so MLPCpp's built-in MinMaxScaler handles scaling
// automatically during Predict().
// =============================================================================
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <string>

struct Sample {
  double k;       // turbulent kinetic energy     [physical]
  double omega;   // specific dissipation rate    [physical]
  double U;       // velocity magnitude           [physical]
  double nu_t;    // reference eddy viscosity     [physical]
};

class CDataset {
public:
  std::vector<Sample> samples;

  // Min/max of each variable over the dataset
  // Written into the .mlp file so MLPCpp normalises automatically
  double k_min, k_max;
  double omega_min, omega_max;
  double U_min, U_max;
  double nu_t_min, nu_t_max;

  CDataset(int N, double noise_level = 0.02, unsigned seed = 123) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dk(0.05, 3.0);
    std::uniform_real_distribution<double> dw(0.5,  8.0);
    std::uniform_real_distribution<double> dU(0.5,  20.0);
    std::normal_distribution<double>       noise(0.0, noise_level);

    samples.reserve(N);
    for (int i = 0; i < N; ++i) {
      Sample s;
      s.k     = dk(rng);
      s.omega = dw(rng);
      s.U     = dU(rng);
      s.nu_t  = std::max(1e-10, 0.09 * s.k / s.omega * (1.0 + noise(rng)));
      samples.push_back(s);
    }
    compute_bounds();
  }

  int size() const { return static_cast<int>(samples.size()); }

  // Raw inputs as vector (physical, MLPCpp normalises internally via .mlp bounds)
  std::vector<double> inputs(int i) const {
    return { samples[i].k, samples[i].omega, samples[i].U };
  }

  double target(int i) const { return samples[i].nu_t; }

  // R2 on physical predictions
  double r2_score(const std::vector<double>& preds) const {
    if ((int)preds.size() != size()) throw std::invalid_argument("r2_score: size mismatch");
    double mean = 0.0;
    for (auto& s : samples) mean += s.nu_t;
    mean /= size();
    double ss_res = 0.0, ss_tot = 0.0;
    for (int i = 0; i < size(); ++i) {
      ss_res += (target(i) - preds[i]) * (target(i) - preds[i]);
      ss_tot += (target(i) - mean)     * (target(i) - mean);
    }
    return 1.0 - ss_res / (ss_tot + 1e-30);
  }

  double physics_rms(const std::vector<double>& preds) const {
    double acc = 0.0;
    for (int i = 0; i < size(); ++i) {
      double res = preds[i] * samples[i].omega - samples[i].k;
      acc += res * res;
    }
    return std::sqrt(acc / size());
  }

private:
  void compute_bounds() {
    k_min = omega_min = U_min = nu_t_min =  1e30;
    k_max = omega_max = U_max = nu_t_max = -1e30;
    for (auto& s : samples) {
      k_min     = std::min(k_min,     s.k);      k_max     = std::max(k_max,     s.k);
      omega_min = std::min(omega_min, s.omega);   omega_max = std::max(omega_max, s.omega);
      U_min     = std::min(U_min,     s.U);       U_max     = std::max(U_max,     s.U);
      nu_t_min  = std::min(nu_t_min,  s.nu_t);    nu_t_max  = std::max(nu_t_max,  s.nu_t);
    }
  }
};
