#include <sprockit/sim_parameters.h>
#include <gtest/gtest.h>

namespace  {
sprockit::sim_parameters* params_;

}

class ParametersEnvironment : public testing::Environment {
 public:
  explicit ParametersEnvironment(int argc, char **argv, sprockit::sim_parameters* params) {
    params_ = params;
  }
};

TEST(BASICTESTS, torus)
{
  params_->print_params();
  EXPECT_EQ("str", "str");
}
