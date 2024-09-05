#include "test_beeb_memory.h"
#include <spdlog/spdlog-inl.h>


void TestBeebMemory::SetUp() {
  beeb = std::make_shared<Beeb>();
}

void TestBeebMemory::TearDown() {
}

TEST_F(TestBeebMemory, zero_length_should_return_empty) {
  auto actual = beeb->get_memory_contents(0x3000, 0);
  EXPECT_TRUE(actual.empty());
}

TEST_F(TestBeebMemory, dram_bottom) {
  auto actual = beeb->get_memory_contents(0x0000, 10);
  EXPECT_EQ(10, actual.size());
}

TEST_F(TestBeebMemory, dram_mid) {
  auto actual = beeb->get_memory_contents(0x3000, 20);
  EXPECT_EQ(20, actual.size());
}

TEST_F(TestBeebMemory, dram_top) {
  auto actual = beeb->get_memory_contents(0x7fff, 1);
  EXPECT_EQ(1, actual.size());
}

TEST_F(TestBeebMemory, dram_basic_rom_boundary) {
  auto actual = beeb->get_memory_contents(0x7fff, 2);
  EXPECT_EQ(2, actual.size());
}

TEST_F(TestBeebMemory, dram_all) {
  auto actual = beeb->get_memory_contents(0x0000, 0x8000);
  EXPECT_EQ(0x8000, actual.size());
}


TEST_F(TestBeebMemory, basic_rom_bottom) {
  auto actual = beeb->get_memory_contents(0x8000, 10);
  EXPECT_EQ(10, actual.size());
}

TEST_F(TestBeebMemory, basic_rom_mid) {
  auto actual = beeb->get_memory_contents(0x9A00, 16);
  EXPECT_EQ(16, actual.size());
}

TEST_F(TestBeebMemory, basic_rom_top) {
  auto actual = beeb->get_memory_contents(0xbfff, 1);
  EXPECT_EQ(1, actual.size());
}

TEST_F(TestBeebMemory, basic_rom_all) {
  auto actual = beeb->get_memory_contents(0x8000, 0x4000);
  EXPECT_EQ(0x4000, actual.size());
}

TEST_F(TestBeebMemory, dram_basic_rom_all) {
  auto actual = beeb->get_memory_contents(0x0000, 0xc000);
  EXPECT_EQ(0xc000, actual.size());
}


TEST_F(TestBeebMemory, mos_start) {
  auto actual = beeb->get_memory_contents(0xc000, 19);
  EXPECT_EQ(19, actual.size());
}

TEST_F(TestBeebMemory, mos_mid) {
  auto actual = beeb->get_memory_contents(0xDE15, 71);
  EXPECT_EQ(71, actual.size());
}

TEST_F(TestBeebMemory, mos_top) {
  auto actual = beeb->get_memory_contents(0xffff, 1);
  EXPECT_EQ(1, actual.size());
}

TEST_F(TestBeebMemory, basic_rom_and_mos) {
  auto actual = beeb->get_memory_contents(0xbfff, 2);
  EXPECT_EQ(2, actual.size());
}

TEST_F(TestBeebMemory, all_basic_rom_and_mos) {
  auto actual = beeb->get_memory_contents(0x8000, 0x4000);
  EXPECT_EQ(0x4000, actual.size());
}

TEST_F(TestBeebMemory, all_memory) {
  auto actual = beeb->get_memory_contents(0, 0x10000);
  EXPECT_EQ(0x10000, actual.size());
}
