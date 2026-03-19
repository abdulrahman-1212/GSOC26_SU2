#pragma once
// =============================================================================
// generate_mlp_file.hpp
// Writes a He-initialised .mlp file in MLPCpp ASCII format.
// Architecture: 3 -> 8 -> 8 -> 1
// =============================================================================
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include "CDataset.hpp"

inline std::string generate_mlp_file(const CDataset& ds,
                                     const std::string& filename = "piml_network.mlp",
                                     unsigned seed = 42)
{
  const std::vector<int> arch = {3, 8, 8, 1};
  const int n_layers = static_cast<int>(arch.size());

  std::mt19937 rng(seed);

  std::ofstream f(filename);
  if (!f.is_open())
    throw std::runtime_error("generate_mlp_file: cannot open " + filename);

  f << std::scientific << std::setprecision(16) << std::showpos;

  f << "<header>\n\n";
  f << "[number of layers]\n" << n_layers << "\n";

  f << "\n[neurons per layer]\n";
  for (int l : arch) f << l << "\n";

  f << "\n[activation function]\n";
  f << "linear\n";
  for (int l = 1; l < n_layers - 1; ++l) f << "relu\n";
  f << "linear\n";

  f << "\n[input names]\nk\nomega\nU\n";
  f << "\n[input regularization method]\nminmax\n";
  f << "\n[input normalization]\n";
  f << ds.k_min     << "\t" << ds.k_max     << "\n";
  f << ds.omega_min << "\t" << ds.omega_max << "\n";
  f << ds.U_min     << "\t" << ds.U_max     << "\n";

  f << "\n[output names]\nnu_t\n";
  f << "\n[output regularization method]\nminmax\n";
  f << "\n[output normalization]\n";
  f << ds.nu_t_min  << "\t" << ds.nu_t_max  << "\n";

  f << "\n</header>\n";

  f << "\n[weights per layer]\n";
  for (int l = 0; l < n_layers - 1; ++l) {
    int fan_in  = arch[l];
    int fan_out = arch[l + 1];
    bool hidden = (l < n_layers - 2);
    double std  = hidden ? std::sqrt(2.0 / fan_in) : std::sqrt(1.0 / fan_in);
    std::normal_distribution<double> dist(0.0, std);
    f << "<layer>\n";
    for (int i = 0; i < fan_in; ++i) {
      for (int j = 0; j < fan_out; ++j) f << dist(rng) << " ";
      f << "\n";
    }
    f << "</layer>\n";
  }

  f << "\n[biases per layer]\n";
  for (int l = 0; l < n_layers; ++l) {
    for (int n = 0; n < arch[l]; ++n) f << 0.0 << " ";
    f << "\n";
  }

  f.close();
  return filename;
}
