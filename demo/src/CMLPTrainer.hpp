#pragma once
#include <vector>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <random>

#include "codi.hpp"
using ADType = codi::RealReverse;
using Tape   = ADType::Tape;

#include "CLookUp_ANN.hpp"
#include "CNeuralNetwork.hpp"
#include "CDataset.hpp"
#include "CAdam.hpp"

// Convert vector<ADType> -> vector<double> via getValue()
inline std::vector<double> to_double(const std::vector<ADType>& v) {
  std::vector<double> r(v.size());
  for (std::size_t i = 0; i < v.size(); ++i) r[i] = v[i].getValue();
  return r;
}

// Convert vector<double> -> vector<ADType>
inline std::vector<ADType> to_ad(const std::vector<double>& v) {
  std::vector<ADType> r(v.size());
  for (std::size_t i = 0; i < v.size(); ++i) r[i] = v[i];
  return r;
}

// ADType loss — recorded on tape
inline ADType loss_ad(const ADType& nu_pred, double nu_ref,
                      double k, double omega, double lam) {
  ADType ed = nu_pred - nu_ref;
  ADType ld = ed * ed;
  double k2 = k*k + 1e-30;
  ADType rp = nu_pred * omega - k;
  ADType lp = rp * rp / k2;
  return ld + lam * lp;
}

// plain double loss — for reporting
struct LossVal { double total, data, phys; };
inline LossVal loss_d(double nu_pred, double nu_ref,
                      double k, double omega, double lam) {
  double ed = nu_pred - nu_ref;
  double k2 = k*k + 1e-30;
  double rp = nu_pred * omega - k;
  return { ed*ed + lam*rp*rp/k2, ed*ed, rp*rp/k2 };
}

// Configs defined outside their classes to avoid GCC nested-default-init bug
struct CAdamTrainerConfig {
  int    n_epochs    = 300;
  int    batch_size  = 64;
  double lambda      = 0.3;
  int    print_every = 50;
  CAdamConfig adam   = CAdamConfig();
};

class CMLPTrainer {
public:
  using Config = CAdamTrainerConfig;

  CMLPTrainer(MLPToolbox::CNeuralNetwork* net, Config cfg = Config())
    : net_(net), cfg_(cfg)
  {
    // GetWeightsBiases() returns vector<mlpdouble> = vector<ADType>
    std::size_t np = net_->GetWeightsBiases().size();
    adam_ = CAdam(np, cfg_.adam);
  }

  void train(const CDataset& ds) {
    std::cout << "\n=== CMLPTrainer ===  epochs=" << cfg_.n_epochs
              << "  batch=" << cfg_.batch_size
              << "  lambda=" << cfg_.lambda
              << "  lr=" << cfg_.adam.alpha << "\n\n";
    std::mt19937 rng(7);
    std::cout << std::left << std::setw(8) << "Epoch"
              << std::setw(13) << "L_total"
              << std::setw(10) << "R2" << "Phys-RMS\n"
              << std::string(42,'-') << "\n";

    for (int ep = 1; ep <= cfg_.n_epochs; ++ep) {
      double el = run_epoch(ds, rng);
      if (ep == 1 || ep % cfg_.print_every == 0) {
        auto   preds = predict_all(ds);
        double r2    = ds.r2_score(preds);
        double prms  = ds.physics_rms(preds);
        std::cout << std::fixed << std::setprecision(5) << std::left
                  << std::setw(8) << ep << std::setw(13) << el
                  << std::setw(10) << r2
                  << std::scientific << std::setprecision(3) << prms << "\n";
      }
    }
    std::cout << std::string(42,'-') << "\n";
    auto preds = predict_all(ds);
    std::cout << "\n[Final]  R2=" << std::fixed << std::setprecision(4)
              << ds.r2_score(preds) << "   Physics RMS="
              << std::scientific << std::setprecision(3)
              << ds.physics_rms(preds) << "\n\nSample predictions (first 6):\n"
              << std::left << std::setw(14) << "  nu_t_ref"
              << std::setw(14) << "nu_t_pred" << "error%\n";
    for (int i = 0; i < std::min(6, ds.size()); ++i) {
      double ref=ds.target(i), pred=preds[i];
      std::cout << std::scientific << std::setprecision(4)
                << "  " << std::setw(14) << ref << std::setw(14) << pred
                << std::fixed << std::setprecision(1)
                << std::abs(pred-ref)/(ref+1e-30)*100.0 << "%\n";
    }
  }

  // Inference only — use SetInput + Predict() to avoid type mismatch
  double predict_one(const std::vector<double>& x) const {
    Tape& tape = ADType::getTape();
    tape.setPassive();
    // SetInput accepts vector<mlpdouble>; to_ad converts doubles
    net_->SetInput(to_ad(x));
    net_->Predict();
    double out = net_->GetOutput(0).getValue();
    tape.setActive();
    return out;
  }

  std::vector<double> predict_all(const CDataset& ds) const {
    std::vector<double> p(ds.size());
    for (int i = 0; i < ds.size(); ++i) p[i] = predict_one(ds.inputs(i));
    return p;
  }

private:
  MLPToolbox::CNeuralNetwork* net_;
  Config cfg_;
  CAdam  adam_;

  double run_epoch(const CDataset& ds, std::mt19937& rng) {
    std::vector<int> idx(ds.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);
    double acc = 0.0; int nb = 0;
    for (int s = 0; s < ds.size(); s += cfg_.batch_size) {
      int e = std::min(s + cfg_.batch_size, ds.size());
      acc += train_batch(ds, idx, s, e, e-s);
      ++nb;
    }
    return acc / nb;
  }

  double train_batch(const CDataset& ds, const std::vector<int>& idx,
                     int start, int end, int bs) {
    Tape& tape = ADType::getTape();

    // 1. Read weights as ADType, extract plain doubles
    std::vector<ADType> wb_ad  = net_->GetWeightsBiases();
    std::vector<double> params = to_double(wb_ad);
    std::size_t         np     = params.size();

    // 2. Build active ADType weights and register on tape
    std::vector<ADType> active(np);
    tape.setActive();
    for (std::size_t i = 0; i < np; ++i) {
      active[i] = params[i];
      tape.registerInput(active[i]);
    }

    // 3. Push active weights into the network
    net_->SetWeightsBiases(active);

    // 4. Accumulate batch loss
    ADType batch_loss = 0.0;
    for (int bi = start; bi < end; ++bi) {
      int i = idx[bi];
      const auto& s = ds.samples[i];
      // SetInput + Predict() — avoids passing vector<double> to Predict(vector<mlpdouble>)
      net_->SetInput(to_ad(ds.inputs(i)));
      net_->Predict();
      ADType nu_pred = net_->GetOutput(0);
      batch_loss = batch_loss
                 + loss_ad(nu_pred, s.nu_t, s.k, s.omega, cfg_.lambda)
                 / static_cast<double>(bs);
    }

    // 5. Reverse sweep
    tape.registerOutput(batch_loss);
    batch_loss.gradient() = 1.0;
    tape.evaluate();

    // 6. Extract gradients
    std::vector<double> grads(np);
    for (std::size_t i = 0; i < np; ++i) grads[i] = active[i].gradient();

    double loss_val = batch_loss.getValue();

    // 7. Reset tape, Adam update, write back
    tape.reset();
    adam_.step(params, grads);
    net_->SetWeightsBiases(to_ad(params));

    return loss_val;
  }
};
