#include "reg_view.h"

#include "ui_reg_view.h"

RegisterView::RegisterView(QWidget *parent) :
    QWidget(parent) //
    , ui(new Ui::RegisterView) //
    , old_pc_{0} //
    , old_sp_{0} //
    , old_a_{0} //
    , old_x_{0} //
    , old_y_{0} //
    , old_flags_{0} //
{
  ui->setupUi(this);

  flag_labels_[7] = ui->lblN;
  flag_labels_[6] = ui->lblV;
  flag_labels_[5] = ui->lblX;
  flag_labels_[4] = ui->lblB;
  flag_labels_[3] = ui->lblD;
  flag_labels_[2] = ui->lblI;
  flag_labels_[1] = ui->lblZ;
  flag_labels_[0] = ui->lblC;
}

RegisterView::~RegisterView() {
  delete ui;
}

void RegisterView::set_flags(uint8_t new_flags) {
  for (auto flag_idx = 0; flag_idx < 8; flag_idx++) {
    uint8_t f = (0x01 << flag_idx);

    if ((old_flags_ & f) == (new_flags & f)) {
      flag_labels_[flag_idx]->setStyleSheet("QLabel {color:black; }");
    } else {
      flag_labels_[flag_idx]->setStyleSheet("QLabel {color:red; font-weight: bold; }");
      flag_labels_[flag_idx]->setText((new_flags & f)
                                      ? flag_labels_[flag_idx]->text().toUpper()
                                      : flag_labels_[flag_idx]->text().toLower());
    }
  }
  old_flags_ = new_flags;
  update();
}

void update_register_8(QLineEdit *text_field, uint8_t &old_value, uint8_t new_value) {
  if (old_value == new_value) {
    text_field->setStyleSheet("color:black");
  } else {
    text_field->setStyleSheet("color:red; font-weight: bold");
    text_field->setText(QStringLiteral("%1").arg(new_value, 2, 16, QChar('0')));
    old_value = new_value;
  }
}

void RegisterView::update_pc(uint16_t new_pc) {
  if (old_pc_ == new_pc) {
    ui->txtPC->setStyleSheet("color:black");
  } else {
    ui->txtPC->setStyleSheet("color:red; font-weight: bold");
    ui->txtPC->setText(QStringLiteral("%1").arg(new_pc, 4, 16, QChar('0')));
    old_pc_ = new_pc;
  }
}

void RegisterView::update_sp(uint8_t new_sp) {
  if ((0x100 | old_sp_) == (0x100 | new_sp)) {
    ui->txtSP->setStyleSheet("color:black");
  } else {
    ui->txtSP->setStyleSheet("color:red; font-weight: bold");
    ui->txtSP->setText(QStringLiteral("%1").arg(new_sp | 0x100, 3, 16, QChar('0')));
    old_sp_ = new_sp;
  }
}

void RegisterView::set_registers(uint8_t new_a, uint8_t new_x, uint8_t new_y, uint16_t new_pc, uint16_t new_sp) {
  update_pc(new_pc);
  update_sp(new_sp);
  update_register_8(ui->txtA, old_a_, new_a);
  update_register_8(ui->txtX, old_x_, new_x);
  update_register_8(ui->txtY, old_y_, new_y);
}