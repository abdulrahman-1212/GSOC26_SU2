// =============================================================================
// tests/test_piml.cpp  -  Unit + integration tests for the PIML demo
// Build: make test  (from ~/GSoC26/demo/)
// Run:   make run_test
// =============================================================================

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <functional>

#include "codi.hpp"
using ADType = codi::RealReverse;
using Tape   = ADType::Tape;

#include "CLookUp_ANN.hpp"
#include "CNeuralNetwork.hpp"
#include "CDataset.hpp"
#include "CAdam.hpp"
#include "CMLPTrainer.hpp"
#include "generate_mlp_file.hpp"

// =============================================================================
// Harness
// =============================================================================
static int  g_passed = 0;
static int  g_failed = 0;
static bool g_section_failed = false;

#define CHECK(cond) \
  do { \
    if (!(cond)) { \
      std::cerr << "  FAIL: " << #cond << "  (" << __FILE__ << ":" << __LINE__ << ")\n"; \
      ++g_failed; g_section_failed = true; \
    } else { ++g_passed; } \
  } while(0)

#define CHECK_NEAR(a, b, tol) \
  do { \
    double _a = static_cast<double>((a).getValue()); \
    double _b = static_cast<double>((b).getValue()); \
    if (std::abs(_a - _b) > (tol)) { \
      std::cerr << "  FAIL: |" << #a << " - " << #b << "| = " \
                << std::abs(_a-_b) << " > " << (tol) \
                << "  (" << __FILE__ << ":" << __LINE__ << ")\n"; \
      ++g_failed; g_section_failed = true; \
    } else { ++g_passed; } \
  } while(0)

// Version for plain doubles (no .getValue())
#define CHECK_NEAR_D(a, b, tol) \
  do { \
    double _a = static_cast<double>(a); \
    double _b = static_cast<double>(b); \
    if (std::abs(_a - _b) > (tol)) { \
      std::cerr << "  FAIL: |" << #a << " - " << #b << "| = " \
                << std::abs(_a-_b) << " > " << (tol) \
                << "  (" << __FILE__ << ":" << __LINE__ << ")\n"; \
      ++g_failed; g_section_failed = true; \
    } else { ++g_passed; } \
  } while(0)

void begin_section(const std::string& name) {
  g_section_failed = false;
  std::cout << "\n[" << name << "]\n";
}
void end_section() {
  std::cout << (g_section_failed ? "  *** SECTION FAILED ***\n" : "  OK\n");
}

// =============================================================================
// TEST 1: CDataset
// =============================================================================
void test_dataset() {
  begin_section("CDataset");

  CDataset ds(500, 0.0, 42);   // no noise: nu_t = 0.09*k/omega exactly
  CHECK(ds.size() == 500);

  for (int i = 0; i < ds.size(); ++i) {
    const auto& s = ds.samples[i];
    CHECK_NEAR_D(s.nu_t, 0.09*s.k/s.omega, 1e-12);
  }

  CHECK(ds.nu_t_min > 0.0);
  CHECK(ds.nu_t_max > ds.nu_t_min);
  CHECK(ds.k_max    > ds.k_min);

  auto inp = ds.inputs(0);
  CHECK(inp.size() == 3);

  // Perfect predictions -> R2 = 1
  std::vector<double> perfect(ds.size());
  for (int i = 0; i < ds.size(); ++i) perfect[i] = ds.target(i);
  CHECK_NEAR_D(ds.r2_score(perfect), 1.0, 1e-10);

  // Constant mean prediction -> R2 = 0
  double mean = 0.0;
  for (int i = 0; i < ds.size(); ++i) mean += ds.target(i);
  mean /= ds.size();
  std::vector<double> const_pred(ds.size(), mean);
  CHECK_NEAR_D(ds.r2_score(const_pred), 0.0, 1e-6);

  // Physics RMS of perfect predictions = 0
  CHECK_NEAR_D(ds.physics_rms(perfect), 0.0, 1e-10);

  end_section();
}

// =============================================================================
// TEST 2: CAdam
// =============================================================================
void test_adam() {
  begin_section("CAdam");

  // Minimise (x - 3)^2
  CAdamConfig cfg; cfg.alpha = 1e-2;
  CAdam adam(1, cfg);
  std::vector<double> p = {0.0};
  for (int i = 0; i < 2000; ++i) {
    std::vector<double> g = {2.0*(p[0]-3.0)};
    adam.step(p, g);
  }
  CHECK_NEAR_D(p[0], 3.0, 1e-3);
  CHECK(adam.steps() == 2000);

  adam.reset();
  CHECK(adam.steps() == 0);

  // Size mismatch throws
  bool threw = false;
  try {
    std::vector<double> bad_p = {1.0, 2.0}, bad_g = {1.0};
    adam.step(bad_p, bad_g);
  } catch (const std::runtime_error&) { threw = true; }
  CHECK(threw);

  // Multi-parameter: minimise sum (x_i - i)^2
  const int N = 10;
  CAdam adam2(N, cfg);
  std::vector<double> p2(N, 0.0);
  for (int step = 0; step < 3000; ++step) {
    std::vector<double> g2(N);
    for (int i = 0; i < N; ++i) g2[i] = 2.0*(p2[i] - i);
    adam2.step(p2, g2);
  }
  for (int i = 0; i < N; ++i)
    CHECK_NEAR_D(p2[i], static_cast<double>(i), 1e-2);

  end_section();
}

// =============================================================================
// TEST 3: loss_d (plain-double loss helper in CMLPTrainer)
// =============================================================================
void test_loss_double() {
  begin_section("loss_d (plain double loss)");

  double k = 1.0, omega = 2.0;
  double nu_ref = 0.09 * k / omega;   // = 0.045

  // Perfect prediction
  auto Lp = loss_d(nu_ref, nu_ref, k, omega, 0.3);
  CHECK_NEAR_D(Lp.data,  0.0, 1e-15);
  CHECK_NEAR_D(Lp.phys,  0.0, 1e-10);
  CHECK_NEAR_D(Lp.total, 0.0, 1e-10);

  // Zero prediction: L_data = nu_ref^2, L_phys = k^2/k^2 = 1
  auto Lz = loss_d(0.0, nu_ref, k, omega, 0.3);
  CHECK_NEAR_D(Lz.data, nu_ref*nu_ref, 1e-12);
  CHECK_NEAR_D(Lz.phys, 1.0,           1e-12);

  // lambda scales physics term
  auto L0 = loss_d(0.0, nu_ref, k, omega, 0.0);
  auto L1 = loss_d(0.0, nu_ref, k, omega, 1.0);
  CHECK(L1.total > L0.total);

  end_section();
}

// =============================================================================
// TEST 4: CoDi gradient on scalar function
// =============================================================================
void test_codi_gradient() {
  begin_section("CoDi gradient (finite difference check)");

  // f(w) = (2w + 1 - 5)^2, df/dw = 4*(2w - 4), at w=3: df/dw = 8
  auto f_d = [](double w){ double e = w*2.0+1.0-5.0; return e*e; };
  double w0 = 3.0, h = 1e-5;
  double fd  = (f_d(w0+h) - f_d(w0-h)) / (2.0*h);

  Tape& tape = ADType::getTape();
  tape.setActive();
  ADType w = w0;
  tape.registerInput(w);
  ADType loss = (w*2.0 + 1.0 - 5.0) * (w*2.0 + 1.0 - 5.0);
  tape.registerOutput(loss);
  loss.gradient() = 1.0;
  tape.evaluate();
  double codi_g = w.gradient();
  tape.reset();

  CHECK_NEAR_D(codi_g, fd,  1e-6);
  CHECK_NEAR_D(codi_g, 8.0, 1e-10);

  end_section();
}

// =============================================================================
// TEST 5: generate_mlp_file + CNeuralNetwork load
// =============================================================================
void test_mlp_file_io() {
  begin_section("generate_mlp_file + CNeuralNetwork load");

  CDataset ds(200, 0.0, 1);
  const std::string fname = "test_network.mlp";
  generate_mlp_file(ds, fname, 99);

  MLPToolbox::CNeuralNetwork net(fname);

  CHECK(net.GetnInputs()  == 3);
  CHECK(net.GetnOutputs() == 1);
  CHECK(net.GetnLayers()  == 4);

  CHECK(net.GetInputName(0)  == "k");
  CHECK(net.GetInputName(1)  == "omega");
  CHECK(net.GetInputName(2)  == "U");
  CHECK(net.GetOutputName(0) == "nu_t");

  CHECK(net.GetActivationFunction(0) == "linear");
  CHECK(net.GetActivationFunction(1) == "relu");
  CHECK(net.GetActivationFunction(2) == "relu");
  CHECK(net.GetActivationFunction(3) == "linear");

  // GetWeightsBiases returns vector<mlpdouble> = vector<ADType>
  auto wb = net.GetWeightsBiases();
  CHECK(wb.size() > 0);

  // Round-trip: perturb, set, get back — compare via getValue()
  std::vector<ADType> wb2 = wb;
  for (auto& v : wb2) v = v.getValue() * 1.5;
  net.SetWeightsBiases(wb2);
  auto wb3 = net.GetWeightsBiases();
  CHECK(wb3.size() == wb2.size());
  for (std::size_t i = 0; i < wb2.size(); ++i)
    CHECK_NEAR_D(wb3[i].getValue(), wb2[i].getValue(), 1e-12);

  // Restore
  net.SetWeightsBiases(wb);

  // Predict via SetInput + Predict()
  Tape& tape = ADType::getTape();
  tape.setPassive();
  net.SetInput(to_ad(ds.inputs(0)));
  net.Predict();
  double out = net.GetOutput(0).getValue();
  tape.setActive();
  CHECK(std::isfinite(out));
  CHECK(!std::isnan(out));

  // WriteNeuralNetwork + reload preserves weights
  net.WriteNeuralNetwork("test_roundtrip.mlp");
  MLPToolbox::CNeuralNetwork net2("test_roundtrip.mlp");
  auto wb_orig = net.GetWeightsBiases();
  auto wb_load = net2.GetWeightsBiases();
  CHECK(wb_orig.size() == wb_load.size());
  for (std::size_t i = 0; i < wb_orig.size(); ++i)
    CHECK_NEAR_D(wb_orig[i].getValue(), wb_load[i].getValue(), 1e-10);

  std::remove(fname.c_str());
  std::remove("test_roundtrip.mlp");

  end_section();
}

// =============================================================================
// TEST 6: CoDi gradient through CNeuralNetwork
// =============================================================================
void test_codi_through_network() {
  begin_section("CoDi gradient through CNeuralNetwork::Predict");

  CDataset ds(50, 0.0, 7);
  const std::string fname = "test_grad_net.mlp";
  generate_mlp_file(ds, fname, 10);
  MLPToolbox::CNeuralNetwork net(fname);

  const auto& s = ds.samples[0];

  // CoDi gradient
  Tape& tape = ADType::getTape();
  std::vector<ADType> wb_ad = net.GetWeightsBiases();
  std::vector<double> params = to_double(wb_ad);
  std::size_t np = params.size();

  std::vector<ADType> active(np);
  tape.setActive();
  for (std::size_t i = 0; i < np; ++i) {
    active[i] = params[i];
    tape.registerInput(active[i]);
  }
  net.SetWeightsBiases(active);
  net.SetInput(to_ad(ds.inputs(0)));
  net.Predict();
  ADType nu_pred = net.GetOutput(0);
  ADType err  = nu_pred - s.nu_t;
  ADType loss = err * err;
  tape.registerOutput(loss);
  loss.gradient() = 1.0;
  tape.evaluate();

  std::vector<double> codi_grads(np);
  for (std::size_t i = 0; i < np; ++i) codi_grads[i] = active[i].gradient();
  tape.reset();
  net.SetWeightsBiases(to_ad(params));   // restore

  // Finite difference on a subset of weights
  const double h = 1e-5;
  auto loss_at = [&](std::size_t idx, double delta) -> double {
    std::vector<double> p = params;
    p[idx] += delta;
    Tape& t2 = ADType::getTape(); t2.setPassive();
    net.SetWeightsBiases(to_ad(p));
    net.SetInput(to_ad(ds.inputs(0)));
    net.Predict();
    double o = net.GetOutput(0).getValue();
    t2.setActive();
    net.SetWeightsBiases(to_ad(params));
    double e = o - s.nu_t; return e*e;
  };

  std::vector<std::size_t> check_idx;
  for (std::size_t i = 0; i < std::min(np, std::size_t(5)); ++i)
    check_idx.push_back(i);
  if (np > 10)
    for (std::size_t i = np/2; i < np/2+5 && i < np; ++i)
      check_idx.push_back(i);
  for (std::size_t i = (np>5?np-5:0); i < np; ++i)
    check_idx.push_back(i);

  for (std::size_t idx : check_idx) {
    double fd = (loss_at(idx,h) - loss_at(idx,-h)) / (2.0*h);
    CHECK_NEAR_D(codi_grads[idx], fd, 1e-4);
  }

  std::remove(fname.c_str());
  end_section();
}

// =============================================================================
// TEST 7: CMLPTrainer convergence
// =============================================================================
void test_trainer_converges() {
  begin_section("CMLPTrainer convergence");

  CDataset ds(500, 0.02, 42);
  const std::string fname = "test_train_net.mlp";
  generate_mlp_file(ds, fname, 1);
  MLPToolbox::CNeuralNetwork net(fname);

  CMLPTrainer::Config cfg;
  cfg.n_epochs    = 100;
  cfg.batch_size  = 64;
  cfg.lambda      = 0.3;
  cfg.print_every = 9999;
  cfg.adam.alpha  = 3e-3;

  CMLPTrainer trainer(&net, cfg);
  auto before = trainer.predict_all(ds);
  double r2_before = ds.r2_score(before);

  // Suppress output
  std::streambuf* ob = std::cout.rdbuf();
  std::ostringstream dev; std::cout.rdbuf(dev.rdbuf());
  trainer.train(ds);
  std::cout.rdbuf(ob);

  auto after = trainer.predict_all(ds);
  double r2_after = ds.r2_score(after);

  CHECK(r2_after > r2_before);
  CHECK(r2_after > 0.5);
  CHECK(std::isfinite(ds.physics_rms(after)));

  std::remove(fname.c_str());
  end_section();
}

// =============================================================================
// TEST 8: CLookUp_ANN Predict API
// =============================================================================
void test_lookup_ann() {
  begin_section("CLookUp_ANN::Predict via CIOMap");

  CDataset ds(100, 0.0, 5);
  const std::string fname = "test_lookup.mlp";
  generate_mlp_file(ds, fname, 2);

  MLPToolbox::CLookUp_ANN lookup({fname});
  CHECK(lookup.GetNANNs() == 1);

  std::vector<std::string> inp = {"k","omega","U"}, out = {"nu_t"};
  MLPToolbox::CIOMap query(inp, out);
  lookup.PairVariableswithMLPs(query);

  const auto& s = ds.samples[0];
  // vals_input and refs_output are vector<mlpdouble> = vector<ADType>
  std::vector<mlpdouble> vals_in = to_ad({s.k, s.omega, s.U});
  mlpdouble nu_pred_val = 0.0;
  std::vector<mlpdouble*> refs_out = { &nu_pred_val };

  bool ok = lookup.Predict(query, vals_in, refs_out);
  CHECK(ok);
  CHECK(std::isfinite(nu_pred_val.getValue()));

  // Weight round-trip via CLookUp_ANN
  auto wb_orig = lookup.GetWeightsBiases(0);
  CHECK(wb_orig.size() > 0);
  lookup.SetWeightsBiases(wb_orig, 0);
  auto wb_back = lookup.GetWeightsBiases(0);
  CHECK(wb_orig.size() == wb_back.size());
  for (std::size_t i = 0; i < wb_orig.size(); ++i)
    CHECK_NEAR_D(wb_orig[i].getValue(), wb_back[i].getValue(), 1e-12);

  std::remove(fname.c_str());
  end_section();
}

// =============================================================================
// TEST 9: Adam analytic regression
// =============================================================================
void test_adam_analytic() {
  begin_section("CAdam analytic regression");

  // Minimise (w1-1)^2 + (w2-2)^2
  CAdamConfig cfg; cfg.alpha = 5e-3;
  CAdam adam(2, cfg);
  std::vector<double> p = {0.0, 0.0};
  for (int i = 0; i < 5000; ++i) {
    std::vector<double> g = {2.0*(p[0]-1.0), 2.0*(p[1]-2.0)};
    adam.step(p, g);
  }
  CHECK_NEAR_D(p[0], 1.0, 1e-3);
  CHECK_NEAR_D(p[1], 2.0, 1e-3);

  end_section();
}

// =============================================================================
int main() {
  std::cout << "============================================================\n"
            << "  PIML Demo - Test Suite\n"
            << "============================================================\n";

  test_dataset();
  test_adam();
  test_loss_double();
  test_codi_gradient();
  test_mlp_file_io();
  test_codi_through_network();
  test_trainer_converges();
  test_lookup_ann();
  test_adam_analytic();

  std::cout << "\n============================================================\n"
            << "  Results: " << g_passed << " passed, "
            << g_failed << " failed"
            << "  (total " << g_passed+g_failed << ")\n"
            << "============================================================\n";
  return g_failed == 0 ? 0 : 1;
}
