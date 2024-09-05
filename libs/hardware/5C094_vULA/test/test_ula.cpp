#include "test_ula.h"
#include "spdlog/spdlog.h"

#include <fstream>

/** Common utilities */
void TestUla::SetUp() {
  v_ula = new VideoUla(0xfe20);

  col_ = 0;
  scan_line_ = 0;
  char_line_ = 0;

  pixel_buffer_ = nullptr;

  bus = std::make_shared<Bus>();
  dram_bus = std::make_shared<Bus>();
}

void TestUla::TearDown() {
  delete v_ula;
  delete[] pixel_buffer_;
}

void TestUla::expect_eq(const std::string &file) {
  std::ifstream f{file, std::ios::binary};
  EXPECT_TRUE(f.is_open());
  std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  f.close();
  EXPECT_EQ(d.size(), pixel_buffer_idx_);
  for (auto i = 0; i < pixel_buffer_idx_; ++i) {
    EXPECT_EQ(d[i], pixel_buffer_[i]);
  }
}

void TestUla::init_pixel_buffer(uint32_t sz) {
  pixel_buffer_size_ = sz;
  pixel_buffer_ = new uint8_t[pixel_buffer_size_];
  pixel_buffer_idx_ = 0;
}

void TestUla::set_mode(uint8_t mode) {
  // Set VULA
  bus->set_address(0xfe20);
  bus->set_data(vula_config_[mode]);
  bus->clr_RW();

  v_ula->tick(bus, dram_bus);

  uint8_t *plt;
  switch (mode) {
    case 0:plt = palette_0346;
      pixels_per_byte_ = 8;
      shift_clk_freq_ = 16;
      scan_width_ = 640;
      data_ = welcome_mode_3;
      data_size_ = sizeof(welcome_mode_3);
      break;
    case 1:plt = palette_15;
      pixels_per_byte_ = 4;
      shift_clk_freq_ = 8;
      data_ = welcome_mode_1;
      data_size_ = sizeof(welcome_mode_1);
      scan_width_ = 320;
      break;
    case 2:plt = palette_2;
      pixels_per_byte_ = 2;
      shift_clk_freq_ = 4;
      pixels_per_byte_ = 2;
      data_ = welcome_mode_2;
      data_size_ = sizeof(welcome_mode_2);
      scan_width_ = 160;
      break;
    case 3:plt = palette_0346;
      shift_clk_freq_ = 16;
      pixels_per_byte_ = 8;
      data_ = welcome_mode_3;
      data_size_ = sizeof(welcome_mode_3);
      scan_width_ = 640;
      break;
    case 4:plt = palette_0346;
      shift_clk_freq_ = 8;
      pixels_per_byte_ = 8;
      scan_width_ = 320;
      data_ = welcome_mode_4;
      data_size_ = sizeof(welcome_mode_4);
      break;
    case 5:plt = palette_15;
      shift_clk_freq_ = 4;
      pixels_per_byte_ = 4;
      data_ = welcome_mode_5;
      data_size_ = sizeof(welcome_mode_5);
      scan_width_ = 160;
      break;
    case 6:plt = palette_0346;
      shift_clk_freq_ = 8;
      pixels_per_byte_ = 8;
      data_ = welcome_mode_6;
      data_size_ = sizeof(welcome_mode_6);
      scan_width_ = 160;
      break;

    default:FAIL();
  }

  // Set palette
  for (int i = 0; i < 16; ++i) {
    bus->set_address(0xfe21);
    bus->set_data(plt[i]);
    v_ula->tick(bus, dram_bus);
  }
}

uint16_t TestUla::generate_sdi() const {
  return (char_line_ * scan_width_) + (col_ * 8) + scan_line_;
}

void TestUla::update_row_column() {
  col_ = (col_ + 1) % (16 * (8 / pixels_per_byte_));// Number of chars of data
  if (col_ == 0) {
    scan_line_ = (scan_line_ + 1) % 8;
    if (scan_line_ == 0) {
      char_line_++;
    }
  }
}
void TestUla::load_next_byte() {
  auto sdi = generate_sdi();
  dram_bus->set_address(0x0000 + sdi);
  dram_bus->set_data(data_[sdi]);
}

void TestUla::render_output() {
  if (pixel_buffer_idx_ + 3 > pixel_buffer_size_) {
    spdlog::critical("PBI out of range. Value = {}, max = {}", pixel_buffer_idx_ + 3, pixel_buffer_size_);
    throw std::runtime_error("PBI out of range.");
  }
  auto rgb = v_ula->rgb();
  pixel_buffer_[pixel_buffer_idx_++] = (rgb >> 16) & 0xff;
  pixel_buffer_[pixel_buffer_idx_++] = (rgb >> 8) & 0xff;
  pixel_buffer_[pixel_buffer_idx_++] = (rgb >> 0) & 0xff;
}

/*************************************************************
 *                                                           *
 *              T E S T S   S T A R T   H E R E              *
 *                                                           *
 *************************************************************/
void TestUla::run_test(uint8_t mode, const std::string &file_name) {
  set_mode(mode);
  auto sz = (3         /* colour planes */
      * 16                     /* characters in welcome message */
      * 64                     /* pixels per character */
      * (16 / shift_clk_freq_) /* bits per pixel */
  );
  init_pixel_buffer(sz);

  pixel_buffer_idx_ = 0;

  uint data_idx = 0;

  auto ticks_required = pixels_per_byte_ * 16 / shift_clk_freq_;
  while (data_idx < data_size_) {
    load_next_byte();

    for (uint32_t j = 0; j < ticks_required; ++j) {
      v_ula->tick(bus, dram_bus);
      render_output();
    }
    update_row_column();
    data_idx++;
  }
  // Compare to expected output data
  expect_eq(file_name);
}

TEST_F(TestUla, mode_0_renders_welcome_ok) {
  run_test(0, "test_data/mode_0_welcome.rgb");
}
TEST_F(TestUla, mode_1_renders_welcome_ok) {
  run_test(1, "test_data/mode_1_welcome.rgb");
}
TEST_F(TestUla, mode_2_renders_welcome_ok) {
  run_test(2, "test_data/mode_2_welcome.rgb");
}
TEST_F(TestUla, mode_3_renders_welcome_ok) {
  run_test(3, "test_data/mode_3_welcome.rgb");
}
TEST_F(TestUla, mode_4_renders_welcome_ok) {
  run_test(4, "test_data/mode_4_welcome.rgb");
}
TEST_F(TestUla, mode_5_renders_welcome_ok) {
  run_test(5, "test_data/mode_5_welcome.rgb");
}
TEST_F(TestUla, mode_6_renders_welcome_ok) {
  run_test(6, "test_data/mode_6_welcome.rgb");
}

