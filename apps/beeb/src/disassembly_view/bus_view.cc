#include "bus_view.h"

#include <QLabel>
#include <QVBoxLayout>

BusView::BusView(QWidget *parent)
    : QWidget(parent) //
    , old_addr_{0} //
    , old_data_{0} //
{
  resize(400, 300);
  auto vertical_layout = new QVBoxLayout(this);
  vertical_layout->setSpacing(0);
  vertical_layout->setSizeConstraint(QLayout::SetDefaultConstraint);
  vertical_layout->setContentsMargins(-1, 12, -1, -1);

  auto frm_addr = new QFrame(this);
  QSizePolicy size_policy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Maximum);
  size_policy.setHorizontalStretch(0);
  size_policy.setVerticalStretch(0);
  size_policy.setHeightForWidth(frm_addr->sizePolicy().hasHeightForWidth());
  frm_addr->setSizePolicy(size_policy);
  frm_addr->setFrameShape(QFrame::StyledPanel);
  frm_addr->setFrameShadow(QFrame::Raised);

  auto horizontal_layout = new QHBoxLayout(frm_addr);
  auto lbl_addr = new QLabel(frm_addr);

  QSizePolicy size_policy_1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
  size_policy_1.setHorizontalStretch(0);
  size_policy_1.setVerticalStretch(0);
  size_policy_1.setHeightForWidth(lbl_addr->sizePolicy().hasHeightForWidth());
  lbl_addr->setSizePolicy(size_policy_1);

  horizontal_layout->addWidget(lbl_addr);

  lbl_addr_bin_lo_ = new QLabel(frm_addr);
  horizontal_layout->addWidget(lbl_addr_bin_lo_);

  lbl_addr_bin_hi_= new QLabel(frm_addr);
  horizontal_layout->addWidget(lbl_addr_bin_hi_);

  lbl_addr_hex_ = new QLabel(frm_addr);
  size_policy_1.setHeightForWidth(lbl_addr_hex_->sizePolicy().hasHeightForWidth());
  lbl_addr_hex_->setSizePolicy(size_policy_1);
  horizontal_layout->addWidget(lbl_addr_hex_);

  vertical_layout->addWidget(frm_addr);

  auto frm_data = new QFrame(this);
  size_policy.setHeightForWidth(frm_data->sizePolicy().hasHeightForWidth());
  frm_data->setSizePolicy(size_policy);
  frm_data->setFrameShape(QFrame::StyledPanel);
  frm_data->setFrameShadow(QFrame::Raised);

  auto horizontal_layout_2 = new QHBoxLayout(frm_data);
  horizontal_layout_2->setObjectName("horizontalLayout_2");
  auto lbl_data = new QLabel(frm_data);
  horizontal_layout_2->addWidget(lbl_data);
  lbl_data_bin_ = new QLabel(frm_data);
  horizontal_layout_2->addWidget(lbl_data_bin_);
  lbl_data_hex_ = new QLabel(frm_data);
  horizontal_layout_2->addWidget(lbl_data_hex_);
  vertical_layout->addWidget(frm_data);

  auto frm_ctl = new QFrame(this);
  size_policy.setHeightForWidth(frm_ctl->sizePolicy().hasHeightForWidth());
  frm_ctl->setSizePolicy(size_policy);
  frm_ctl->setFrameShape(QFrame::StyledPanel);
  frm_ctl->setFrameShadow(QFrame::Raised);

  auto horizontal_layout_3 = new QHBoxLayout(frm_ctl);
  auto rb_reset = new QRadioButton(frm_ctl);
  horizontal_layout_3->addWidget(rb_reset);
  auto rb_sync = new QRadioButton(frm_ctl);
  horizontal_layout_3->addWidget(rb_sync);
  auto rb_rw = new QRadioButton(frm_ctl);
  horizontal_layout_3->addWidget(rb_rw);
  vertical_layout->addWidget(frm_ctl, 0, Qt::AlignTop);

  setLayout(vertical_layout);
}

void set_binary_value(QLabel *label, uint8_t value) {
  label->setText(QString("%1").arg(value, 8, 2, QChar('0')));
}

void BusView::set_bus(const std::shared_ptr<Bus>& bus) {
  auto new_addr = bus->get_address();
  if ((new_addr & 0xff00) != (old_addr_ & 0xff00)) {
    set_binary_value(lbl_addr_bin_hi_, new_addr >> 8);
    lbl_addr_bin_hi_->setStyleSheet("QLabel {color:red; }");
  } else {
    lbl_addr_bin_hi_->setStyleSheet("QLabel {color:black; }");
  }

  if ((new_addr & 0xff) != (old_addr_ & 0xff)) {
    set_binary_value(lbl_addr_bin_lo_, new_addr & 0xff);
    lbl_addr_bin_lo_->setStyleSheet("QLabel {color:red; }");
  } else {
    lbl_addr_bin_lo_->setStyleSheet("QLabel {color:black; }");
  }
  old_addr_ = new_addr;
  lbl_addr_hex_->setText(QString("%1").arg(new_addr, 4, 16, QChar('0')));

  auto new_data = bus->get_data();
  if (old_data_ != new_data) {
    set_binary_value(lbl_data_bin_, new_data);
    lbl_data_bin_->setStyleSheet("QLabel {color:red; }");
  } else {
    lbl_data_bin_->setStyleSheet("QLabel {color:black; }");
  }
  old_data_ = new_data;
  lbl_data_hex_->setText(QString("%1").arg(new_data, 2, 16, QChar('0')));

  rb_sync_->setChecked(bus->tst_SYNC());
  rb_reset_->setChecked(bus->tst_RST());
  rb_rw_->setChecked(bus->tst_RW());
}

BusView::~BusView() = default;
