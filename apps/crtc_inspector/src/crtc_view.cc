#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "crtc_view.h"
#include "ui_crtc_view.h"
#include "spdlog/spdlog.h"
#include <vector>
#include <functional>

const std::vector<std::vector<uint8_t>> DEFAULTS = {
    {0x7f, 0x50, 0x62, 0x08, 0x26, 0x00, 0x20, 0x22, 0x01, 0x07, 0x67, 0x08, 0x06, 0x00, 0x06, 0x00},
    {0x7f, 0x50, 0x62, 0x08, 0x26, 0x00, 0x20, 0x22, 0x01, 0x07, 0x67, 0x08, 0x06, 0x00, 0x06, 0x00},
    {0x7f, 0x50, 0x62, 0x08, 0x26, 0x00, 0x20, 0x22, 0x01, 0x07, 0x67, 0x08, 0x06, 0x00, 0x06, 0x00},
    {0x7f, 0x50, 0x62, 0x08, 0x1e, 0x02, 0x19, 0x1b, 0x01, 0x09, 0x67, 0x09, 0x08, 0x00, 0x08, 0x00},
    {0x3f, 0x28, 0x31, 0x04, 0x26, 0x00, 0x20, 0x22, 0x01, 0x07, 0x67, 0x08, 0x0b, 0x00, 0x0b, 0x00},
    {0x3f, 0x28, 0x31, 0x04, 0x26, 0x00, 0x20, 0x22, 0x01, 0x07, 0x67, 0x08, 0x0b, 0x00, 0x0b, 0x00},
    {0x3f, 0x28, 0x31, 0x04, 0x1e, 0x02, 0x19, 0x1b, 0x01, 0x09, 0x67, 0x09, 0x0c, 0x00, 0x0c, 0x00},
    {0x3f, 0x28, 0x33, 0x04, 0x1e, 0x02, 0x19, 0x1b, 0x03, 0x12, 0x72, 0x13, 0x0f, 0x80, 0x0f, 0x80},
};

const std::vector<std::function<uint8_t(DebuggableCrtc *)>> CALLEES = {
    &DebuggableCrtc::get_reg_horz_total,
    &DebuggableCrtc::get_reg_horz_disp,
    &DebuggableCrtc::get_horz_sync_pos,
    &DebuggableCrtc::get_sync_width,
    &DebuggableCrtc::get_reg_vert_total,
    &DebuggableCrtc::get_reg_vert_adj,
    &DebuggableCrtc::get_reg_vert_disp,
    &DebuggableCrtc::get_vert_sync_pos,
    &DebuggableCrtc::get_reg_interlace,
    &DebuggableCrtc::get_reg_char_rasters,
    &DebuggableCrtc::get_reg_cursor,
    &DebuggableCrtc::get_reg_cursor_end,
    &DebuggableCrtc::get_reg_screen_start_hi,
    &DebuggableCrtc::get_reg_screen_start_lo,
    &DebuggableCrtc::get_reg_cursor_start_hi,
    &DebuggableCrtc::get_reg_cursor_start_lo,
    &DebuggableCrtc::get_reg_light_pen_hi,
    &DebuggableCrtc::get_reg_light_pen_lo
};

//
// Utility Functions
QString two_digit_hex(uint8_t u);
void update_field(QLineEdit *field, uint8_t old_value, uint8_t new_value);
void update_without_signals(QLineEdit *field, const QString &value);
bool value_from_field(QLineEdit *field, uint8_t *value);
QString hex_addr_from(uint8_t h, uint8_t l);

CrtcView::CrtcView(QWidget *parent)
    : QWidget(parent), ui_(new Ui::CrtcView) {

  bus_ = std::make_shared<Bus>();

  ui_->setupUi(this);

  register_fields_.push_back(ui_->txt_r0);
  register_fields_.push_back(ui_->txt_r1);
  register_fields_.push_back(ui_->txt_r2);
  register_fields_.push_back(ui_->txt_r3);
  register_fields_.push_back(ui_->txt_r4);
  register_fields_.push_back(ui_->txt_r5);
  register_fields_.push_back(ui_->txt_r6);
  register_fields_.push_back(ui_->txt_r7);
  register_fields_.push_back(ui_->txt_r8);
  register_fields_.push_back(ui_->txt_r9);
  register_fields_.push_back(ui_->txt_r10);
  register_fields_.push_back(ui_->txt_r11);
  register_fields_.push_back(ui_->txt_r12);
  register_fields_.push_back(ui_->txt_r13);
  register_fields_.push_back(ui_->txt_r14);
  register_fields_.push_back(ui_->txt_r15);
  register_fields_.push_back(ui_->txt_r16);
  register_fields_.push_back(ui_->txt_r17);

  // Wire up complex register data
  QObject::connect(ui_->txt_r3, &QLineEdit::textChanged, this, &CrtcView::breakout_r3);
  QObject::connect(ui_->txt_r8, &QLineEdit::textChanged, this, &CrtcView::breakout_r8);
  QObject::connect(ui_->txt_r10, &QLineEdit::textChanged, this, &CrtcView::breakout_r10);
  QObject::connect(ui_->txt_r12, &QLineEdit::textChanged, this, &CrtcView::update_scr_addr);
  QObject::connect(ui_->txt_r13, &QLineEdit::textChanged, this, &CrtcView::update_scr_addr);
  QObject::connect(ui_->txt_r14, &QLineEdit::textChanged, this, &CrtcView::update_curs_addr);
  QObject::connect(ui_->txt_r15, &QLineEdit::textChanged, this, &CrtcView::update_curs_addr);
  QObject::connect(ui_->txt_r16, &QLineEdit::textChanged, this, &CrtcView::update_lp_addr);
  QObject::connect(ui_->txt_r17, &QLineEdit::textChanged, this, &CrtcView::update_lp_addr);

  // Mode buttons
  QObject::connect(ui_->rbg_mode, &QButtonGroup::buttonClicked, [this](QAbstractButton *btn) {
    auto mode = btn->text().right(1).toInt();
    set_mode(mode);
  });

  QObject::connect(ui_->btn_next, &QPushButton::pressed, [this]() {
    crtc_->generate_next_address(bus_);
    update_fields();
  });
  QObject::connect(ui_->btn_end_of_scan, &QPushButton::pressed, [this]() {
    while (!crtc_->is_end_of_scanline()) {
      crtc_->generate_next_address(bus_);
    }
    update_fields();
  });
  QObject::connect(ui_->btn_end_of_row, &QPushButton::pressed, [this]() {
    while (!crtc_->is_end_of_row()) {
      crtc_->generate_next_address(bus_);
    }
    update_fields();
  });

  QObject::connect(ui_->btn_end_of_frame, &QPushButton::pressed, [this]() {
    while (crtc_->get_output_addr() != 0x7ff8) {
      crtc_->generate_next_address(bus_);
    }
    update_fields();
  });

  ui_->rb_mode_0->setChecked(true);
}

CrtcView::~CrtcView() {
  delete ui_;
}

void
CrtcView::set_crtc(DebuggableCrtc *crtc) {
  crtc_ = crtc;
  update_fields();
}

void
CrtcView::update_fields() {
  // Copy current to last
  for (auto i = 0; i < NUM_REGISTERS; ++i) {
    auto old_value = current_registers_[i];
    auto new_value = CALLEES[i](crtc_);
    update_field(register_fields_[i], old_value, new_value);
    last_registers_[i] = old_value;
    current_registers_[i] = new_value;
  }

  auto addr = crtc_->get_output_addr();
  ui_->txt_output_addr->setText("&" + QString::number(addr, 16));

  auto lc = crtc_->get_linear_cnt();
  ui_->txt_linear_addr_cnt->setText("&" + QString::number(lc, 16));

  auto sl = crtc_->get_character_row();
  ui_->txt_scan_line->setText(QString::number(sl));

  auto ric = crtc_->get_raster_in_char();
  ui_->txt_raster_in_char->setText(QString::number(ric));

  auto cc = crtc_->get_char_cnt();
  ui_->txt_char_cnt->setText(QString::number(cc));
}

void
CrtcView::set_mode(uint8_t mode) {
  if (mode >= 8) {
    spdlog::error("Invalid mode {}", mode);
    return;
  }
  crtc_->reset();
  // Ignore lightpen when setting mode.
  for (auto i = 0; i < NUM_REGISTERS - 2; ++i) {
    crtc_->set_register(i, DEFAULTS[mode][i]);
  }
  switch (mode) {
    case 0:
    case 1:
    case 2:
      crtc_->hw_scroll_addr()->set_data(4, false);
      crtc_->hw_scroll_addr()->set_data(5, true);
      break;
    case 3:
      crtc_->hw_scroll_addr()->set_data(4, false);
      crtc_->hw_scroll_addr()->set_data(5, false);
      break;
    case 4:
    case 5:
      crtc_->hw_scroll_addr()->set_data(4, true);
      crtc_->hw_scroll_addr()->set_data(5, true);
      break;
    case 6:
      crtc_->hw_scroll_addr()->set_data(4, true);
      crtc_->hw_scroll_addr()->set_data(5, false);
      break;
    default:
      break;
  }
  update_fields();
}

void CrtcView::breakout_r3() {
  uint8_t v;
  if (!value_from_field(ui_->txt_r3, &v)) return;

  update_without_signals(ui_->txt_hsync_width, QString::number(v & 0x0f));
  update_without_signals(ui_->txt_vsync_width, QString::number(v >> 4));
}

// 76 54 32 10
// 00 10 00 10
void CrtcView::breakout_r8() {
  uint8_t v;
  if (!value_from_field(ui_->txt_r8, &v)) return;
  if ((v & 0x01) == 0) {
    update_without_signals(ui_->txt_r8_mode, "N");
  } else {
    if ((v & 0x02) == 0) {
      update_without_signals(ui_->txt_r8_mode, "I");
    } else {
      update_without_signals(ui_->txt_r8_mode, "V");
    }
  }
  auto disp_blnk = (v >> 4) & 0x3;
  switch (disp_blnk) {
    case 0:
      update_without_signals(ui_->txt_r8_disp_delay, "0");
      break;
    case 1:
      update_without_signals(ui_->txt_r8_disp_delay, "1");
      break;
    case 2:
      update_without_signals(ui_->txt_r8_disp_delay, "2");
      break;
    case 3:
      update_without_signals(ui_->txt_r8_disp_delay, "X");
      break;
    default:
      break;
  }
  auto curs_blnk = (v >> 6) & 0x3;
  switch (curs_blnk) {
    case 0:
      update_without_signals(ui_->txt_r8_curs_delay, "0");
      break;
    case 1:
      update_without_signals(ui_->txt_r8_curs_delay, "1");
      break;
    case 2:
      update_without_signals(ui_->txt_r8_curs_delay, "2");
      break;
    case 3:
      update_without_signals(ui_->txt_r8_curs_delay, "X");
      break;
    default:
      break;
  }
}
void CrtcView::breakout_r10() {
  //Bit 6 enables or disables the blink feature.
  // Bit 5 is the blink timing control bit.
  // When bit 5=0, blink frequency = 16 times the field rate.
  // When bit 5=1, blink frequency = 32 times the field rate.
  // When bit 6=0 and bit 5=1, the cursor is disabled.
  // The cursor start line is set by the lower five bits.
  // 0 0   : blinbk off
  // 0 1   : curs off
  // 1 0   : blink on x16
  // 1 1   : blink on x32
  uint8_t v;
  if (!value_from_field(ui_->txt_r10, &v)) return;
  auto s = (v >> 5) & 0x03;
  switch (s) {
    case 0:
      update_without_signals(ui_->txt_r10_curs, "Y");
      update_without_signals(ui_->txt_r10_blink, "X");
      break;
    case 1:
      update_without_signals(ui_->txt_r10_curs, "N");
      update_without_signals(ui_->txt_r10_blink, "");
      break;
    case 2:
      update_without_signals(ui_->txt_r10_curs, "Y");
      update_without_signals(ui_->txt_r10_blink, "S");
      break;
    case 3:
      update_without_signals(ui_->txt_r10_curs, "Y");
      update_without_signals(ui_->txt_r10_blink, "F");
      break;
    default:
      break;
  }
  update_without_signals(ui_->txt_r10_start, QString::number(v & 0x1f));
}

void CrtcView::update_scr_addr() {
  uint8_t hi;
  uint8_t lo;
  if (!value_from_field(ui_->txt_r12, &hi)) return;
  if (!value_from_field(ui_->txt_r13, &lo)) return;
  update_without_signals(ui_->txt_scr_addr, hex_addr_from(hi, lo));
}
void CrtcView::update_curs_addr() {
  uint8_t hi;
  uint8_t lo;
  if (!value_from_field(ui_->txt_r14, &hi)) return;
  if (!value_from_field(ui_->txt_r15, &lo)) return;
  update_without_signals(ui_->txt_curs_start_addr, hex_addr_from(hi, lo));

}
void CrtcView::update_lp_addr() {
  uint8_t hi;
  uint8_t lo;
  if (!value_from_field(ui_->txt_r16, &hi)) return;
  if (!value_from_field(ui_->txt_r17, &lo)) return;
  update_without_signals(ui_->txt_lp_addr, hex_addr_from(hi, lo));
}

QString two_digit_hex(uint8_t u) {
  return "&" + QString::number(u, 16).toUpper().rightJustified(2, '0');
}

void
update_field(QLineEdit *field, uint8_t old_value, uint8_t new_value) {
  if (old_value == new_value) {
    field->setStyleSheet("QLineEdit { color: #000000; }");
    return;
  }

  field->setStyleSheet("QLineEdit { color: #ff0000; }");
  field->setText(two_digit_hex(new_value));
}

void update_without_signals(QLineEdit *field, const QString &value) {
  auto b = field->blockSignals(true);
  field->setText(value);
  field->blockSignals(b);
}

bool value_from_field(QLineEdit *field, uint8_t *value) {
  auto txt = field->text().trimmed();
  if (txt.isEmpty() or txt.isNull()) return false;
  if (txt.at(0) == '&') {
    if (txt.length() > 1) {
      txt = txt.right(txt.length() - 1);
    } else {
      return false;
    }
  }
  bool ok = false;
  *value = txt.toInt(&ok, 16);
  return ok;
}

QString hex_addr_from(uint8_t h, uint8_t l) {
  uint16_t address = (static_cast<uint16_t>(h) << 8) | l;
  QString hexString = QString::number(address, 16).toUpper().rightJustified(4, '0');
  return "&" + hexString;
}

