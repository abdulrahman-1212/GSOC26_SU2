# =============================================================================
# Makefile  -  PIML demo (real MLPCpp + CoDiPack)
#
# Usage (from ~/GSoC26/demo/):
#   make              # build main demo
#   make test         # build test suite
#   make run          # build + run demo
#   make run_test     # build + run tests
#   make all          # build both
#   make clean
# =============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra

# codi::RealReverse used as mlpdouble inside MLPCpp
CXXFLAGS += -DMLP_CUSTOM_TYPE="codi::RealReverse"

INCS := \
    -I externals/MLPCpp/include \
    -I externals/CoDiPack/include \
    -I src

TARGET      := piml_demo
TEST_TARGET := test_piml

.PHONY: all run run_test test clean

all: $(TARGET) $(TEST_TARGET)

# ── Main demo ─────────────────────────────────────────────────────────────────
$(TARGET): src/main.cpp \
           src/CDataset.hpp \
           src/CAdam.hpp \
           src/CMLPTrainer.hpp \
           src/generate_mlp_file.hpp
	$(CXX) $(CXXFLAGS) $(INCS) src/main.cpp -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

# ── Test suite ────────────────────────────────────────────────────────────────
$(TEST_TARGET): tests/test_piml.cpp \
                src/CDataset.hpp \
                src/CAdam.hpp \
                src/CMLPTrainer.hpp \
                src/generate_mlp_file.hpp
	$(CXX) $(CXXFLAGS) $(INCS) -I tests tests/test_piml.cpp -o $(TEST_TARGET)

test: $(TEST_TARGET)

run_test: $(TEST_TARGET)
	./$(TEST_TARGET)

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -f $(TARGET) $(TEST_TARGET) \
	      piml_network.mlp piml_network_trained.mlp \
	      test_network.mlp test_roundtrip.mlp \
	      test_grad_net.mlp test_train_net.mlp test_lookup.mlp
