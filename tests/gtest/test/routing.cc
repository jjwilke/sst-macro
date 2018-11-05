#include <gtest/gtest.h>
#include <sstmac/main/sstmac.h>

TEST(my_class, my_test)
{
  sprockit::sim_parameters params;
  //sstmac::try_main(&params, argc, argv, true);
  params.print_params();
  int check = params.get_optional_int_param("param", 42);
  ASSERT_EQ(check, 42);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

