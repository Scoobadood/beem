#include "register_view.h"

#include <QLineEdit>
#include <QHBoxLayout>

void configure_label(QLabel *label) {
  QSizePolicy size_policy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
  size_policy.setHorizontalStretch(0);
  size_policy.setVerticalStretch(0);
  size_policy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
  label->setSizePolicy(size_policy);
  label->setMinimumSize(QSize(20, 8));
  label->setLineWidth(0);
  label->setTextFormat(Qt::PlainText);
  label->setScaledContents(false);
}

void configure_line_edit(QLineEdit *line_edit, int32_t le_width, int32_t le_height) {
  QSizePolicy size_policy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
  size_policy.setHorizontalStretch(0);
  size_policy.setVerticalStretch(0);
  size_policy.setHeightForWidth(line_edit->sizePolicy().hasHeightForWidth());
  line_edit->setSizePolicy(size_policy);
  line_edit->setMinimumSize(QSize(le_width, le_height));
  line_edit->setMaximumSize(QSize(le_width, le_height));
  line_edit->setInputMask(QString::fromUtf8(""));
  line_edit->setText(QString::fromUtf8(""));
  line_edit->setMaxLength(4);
  line_edit->setCursorPosition(0);
  line_edit->setAlignment(Qt::AlignCenter);
  line_edit->setReadOnly(true);
}

QWidget *configure_item(QWidget *parent,
                        const char *label_text,
                        QLineEdit *line_edit,
                        int32_t le_width, int32_t le_height) {
  auto widget = new QWidget(parent);

  QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
  widget->setSizePolicy(sizePolicy);
  widget->setMinimumSize(QSize(0, 0));

  auto vertical_layout = new QVBoxLayout(widget);
  vertical_layout->setSpacing(0);
  vertical_layout->setSizeConstraint(QLayout::SetMinimumSize);
  vertical_layout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(vertical_layout);

  auto lbl = new QLabel(label_text, widget);
  configure_label(lbl);
  vertical_layout->addWidget(lbl, 0, Qt::AlignLeft | Qt::AlignTop);

  line_edit->setParent(widget);
  configure_line_edit(line_edit, le_width, le_height);
  vertical_layout->addWidget(line_edit, 0, Qt::AlignLeft | Qt::AlignTop);

  return widget;
}

QWidget *RegisterView::configure_flags(QWidget *parent) {
  auto flags = new QWidget(parent);

  QSizePolicy size_policy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
  size_policy.setHorizontalStretch(0);
  size_policy.setVerticalStretch(0);
  size_policy.setHeightForWidth(flags->sizePolicy().hasHeightForWidth());
  flags->setSizePolicy(size_policy);
  flags->setMinimumSize(QSize(0, 0));

  auto vertical_layout = new QVBoxLayout(flags);
  vertical_layout->setSpacing(0);
  vertical_layout->setSizeConstraint(QLayout::SetMinimumSize);
  vertical_layout->setContentsMargins(0, 0, 0, 0);

  auto lbl = new QLabel("Flags", flags);
  configure_label(lbl);
  vertical_layout->addWidget(lbl, 0, Qt::AlignLeft | Qt::AlignTop);


  auto flag_array = new QWidget(flags);
  auto horizontal_layout = new QHBoxLayout(flag_array);
  horizontal_layout->setSpacing(0);
  horizontal_layout->setContentsMargins(0, 0, 0, 0);
  flag_array->setLayout(horizontal_layout);
  QString flag_names = tr("CZIDBXVN");
  for (auto i = 0; i < 8; i++) {
    auto le =new QLineEdit(flag_array);
    size_policy.setHeightForWidth(le->sizePolicy().hasHeightForWidth());
    le->setSizePolicy(size_policy);
    le->setMinimumSize(QSize(12, 20));
    le->setMaximumSize(QSize(12, 20));
    le->setInputMask(QString::fromUtf8(""));
    le->setText(flag_names.mid(i, i + 1));
    le->setMaxLength(1);
    le->setReadOnly(true);
    horizontal_layout->addWidget(le);

    flag_labels_[i] = le;
  }
  vertical_layout->addWidget(flag_array, 0, Qt::AlignLeft | Qt::AlignTop);
  return flags;
}

void RegisterView::make_ui(QWidget *form) {
  form->resize(236, 32);
  form->setStyleSheet(QString::fromUtf8("QLabel { font-family: 'Arial'; font-size: 8pt; }\n"
                                        "QLineEdit{ font-family: 'Monaco'; font-size: 12pt; }"));
  auto horizontalLayout = new QHBoxLayout(form);
  horizontalLayout->setSpacing(0);
  horizontalLayout->setContentsMargins(0, 0, 0, 0);

  reg_pc_ = new QLineEdit();
  horizontalLayout->addWidget(configure_item(this, "PC", reg_pc_, 40, 20));

  reg_sp_ = new QLineEdit();
  horizontalLayout->addWidget(configure_item(this, "SP", reg_sp_, 40, 20));

  reg_a_ = new QLineEdit();
  horizontalLayout->addWidget(configure_item(this, "A", reg_a_, 20, 20));

  reg_x_ = new QLineEdit();
  horizontalLayout->addWidget(configure_item(this, "X", reg_x_, 20, 20));

  reg_y_ = new QLineEdit();
  horizontalLayout->addWidget(configure_item(this, "Y", reg_y_, 20, 20));

  horizontalLayout->addWidget(configure_flags(this));
}

RegisterView::RegisterView(QWidget *parent) :
    QWidget(parent) //
    , old_pc_{0} //
    , old_sp_{0} //
    , old_a_{0} //
    , old_x_{0} //
    , old_y_{0} //
    , old_flags_{0} //
{
  setStyleSheet("QLabel { font-family: 'Arial'; font-size: 8pt; }\n"
                "QLineEdit{ font-family: 'Monaco'; font-size: 12pt; }");

  make_ui(this);
}

RegisterView::~RegisterView() = default;

void RegisterView::set_flags(uint8_t new_flags) {
  for (auto flag_idx = 0; flag_idx < 8; flag_idx++) {
    uint8_t f = (0x01 << flag_idx);

    if ((old_flags_ & f) == (new_flags & f)) {
      flag_labels_[flag_idx]->setStyleSheet("color: black");
    } else {
      flag_labels_[flag_idx]->setStyleSheet("color:red; font-weight: bold;");
    }
    flag_labels_[flag_idx]->setText((new_flags & f)
                                    ? flag_labels_[flag_idx]->text().toUpper()
                                    : flag_labels_[flag_idx]->text().toLower());
  }
  old_flags_ = new_flags;
  update();
}

void update_register_8(QLineEdit *text_field, uint8_t &old_value, uint8_t new_value) {
  if (old_value == new_value) {
    text_field->setStyleSheet("color:black");
  } else {
    text_field->setStyleSheet("color:red; font-weight: bold");
    old_value = new_value;
  }
  text_field->setText(QStringLiteral("%1").arg(new_value, 2, 16, QChar('0')));
}

void RegisterView::update_pc(uint16_t new_pc) {
  if (old_pc_ == new_pc) {
    reg_pc_->setStyleSheet("color:black");
  } else {
    reg_pc_->setStyleSheet("color:red; font-weight: bold");
    reg_pc_->setText(QStringLiteral("%1").arg(new_pc, 4, 16, QChar('0')));
    old_pc_ = new_pc;
  }
}

void RegisterView::update_sp(uint8_t new_sp) {
  if ((0x100 | old_sp_) == (0x100 | new_sp)) {
    reg_sp_->setStyleSheet("color:black");
  } else {
    reg_sp_->setStyleSheet("color:red; font-weight: bold");
    reg_sp_->setText(QStringLiteral("%1").arg(new_sp | 0x100, 3, 16, QChar('0')));
    old_sp_ = new_sp;
  }
}

void RegisterView::set_registers(uint8_t new_a, uint8_t new_x, uint8_t new_y, uint16_t new_pc, uint16_t new_sp) {
  update_pc(new_pc);
  update_sp(new_sp);
  update_register_8(reg_a_, old_a_, new_a);
  update_register_8(reg_x_, old_x_, new_x);
  update_register_8(reg_y_, old_y_, new_y);
}