#pragma once
#include <vector>
#include <cmath>
#include <stdexcept>

struct CAdamConfig {
  double alpha = 3e-3;
  double beta1 = 0.9;
  double beta2 = 0.999;
  double eps   = 1e-8;
};

class CAdam {
public:
  using Config = CAdamConfig;

  CAdam() = default;
  CAdam(std::size_t n, Config c = Config()) : cfg_(c), m_(n,0.0), v_(n,0.0) {}

  void step(std::vector<double>& p, const std::vector<double>& g) {
    if (p.size() != g.size() || p.size() != m_.size())
      throw std::runtime_error("CAdam::step - size mismatch");
    ++t_;
    double lr_t = cfg_.alpha
                  * std::sqrt(1.0 - std::pow(cfg_.beta2, t_))
                  / (1.0 - std::pow(cfg_.beta1, t_));
    for (std::size_t i = 0; i < p.size(); ++i) {
      m_[i] = cfg_.beta1*m_[i] + (1.0-cfg_.beta1)*g[i];
      v_[i] = cfg_.beta2*v_[i] + (1.0-cfg_.beta2)*g[i]*g[i];
      p[i] -= lr_t * m_[i] / (std::sqrt(v_[i]) + cfg_.eps);
    }
  }

  void   reset()  { t_=0; std::fill(m_.begin(),m_.end(),0.0);
                           std::fill(v_.begin(),v_.end(),0.0); }
  int    steps()  const { return t_; }
  Config config() const { return cfg_; }

private:
  Config cfg_;
  int    t_ = 0;
  std::vector<double> m_, v_;
};
