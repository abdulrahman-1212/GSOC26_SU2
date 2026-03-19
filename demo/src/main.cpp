#include <iostream>
#include <iomanip>
#include <string>

#include "codi.hpp"
#include "CLookUp_ANN.hpp"
#include "CNeuralNetwork.hpp"
#include "CDataset.hpp"
#include "generate_mlp_file.hpp"
#include "CAdam.hpp"
#include "CMLPTrainer.hpp"

int main() {
  std::cout << "============================================================\n"
            << "  PIML Demo  -  MLPCpp + CoDiPack + Adam\n"
            << "============================================================\n\n";

  constexpr int N = 2000;
  std::cout << "Generating " << N << " synthetic SST samples...\n";
  CDataset ds(N, 0.02, 123);

  double nu_mean = 0.0;
  for (auto& s : ds.samples) nu_mean += s.nu_t;
  nu_mean /= ds.size();
  std::cout << std::scientific << std::setprecision(3)
            << "  nu_t range: [" << ds.nu_t_min << ", " << ds.nu_t_max
            << "]  mean=" << nu_mean << "\n\n";

  const std::string mlp_file = "piml_network.mlp";
  std::cout << "Writing initial network to: " << mlp_file << "\n";
  generate_mlp_file(ds, mlp_file, 42);

  std::cout << "Loading network via CNeuralNetwork...\n";
  MLPToolbox::CNeuralNetwork network(mlp_file);
  network.DisplayNetwork();
  std::cout << "  Trainable parameters: "
            << network.GetWeightsBiases().size() << "\n\n";

  CMLPTrainer::Config cfg;
  cfg.n_epochs    = 400;
  cfg.batch_size  = 64;
  cfg.lambda      = 0.3;
  cfg.print_every = 50;
  cfg.adam.alpha  = 3e-3;

  CMLPTrainer trainer(&network, cfg);
  trainer.train(ds);

  const std::string out_file = "piml_network_trained.mlp";
  std::cout << "\nSaving trained network to: " << out_file << "\n";
  network.WriteNeuralNetwork(out_file);
  std::cout << "Done.\n";
  return 0;
}
