#pragma once

#include <gtest/gtest.h>
#include "execution_engine.h"

class TestExecutionEngine : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override {}

  std::unique_ptr<ExecutionEngine> engine;
};
