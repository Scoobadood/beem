#include "breakpoint_dialog.h"

#include <QTextList>
#include <QGridLayout>
#include <QPushButton>

void
BreakpointDlg::setup_ui() {
  resize(217, 311);
  auto gridLayout = new QGridLayout(this);
  te_new_brk_ = new QLineEdit(this);
  te_new_brk_->setInputMask(QString::fromUtf8("hhhh"));
  te_new_brk_->setText(QString::fromUtf8(""));
  gridLayout->addWidget(te_new_brk_, 0, 0, 1, 2);

  lst_brk_ = new QListWidget(this);
  gridLayout->addWidget(lst_brk_, 1, 0, 3, 2);

  btn_remove_ = new QPushButton("Remove", this);
  gridLayout->addWidget(btn_remove_, 4, 0, 1, 1);
  btn_remove_all_ = new QPushButton("Remove All", this);
  gridLayout->addWidget(btn_remove_all_, 4, 1, 1, 1);

  btn_box_ = new QDialogButtonBox(this);
  btn_box_->setOrientation(Qt::Horizontal);
  btn_box_->setStandardButtons(QDialogButtonBox::Ok);
  gridLayout->addWidget(btn_box_, 5, 0, 1, 2);


  assert(connect(btn_remove_, &QPushButton::clicked, this, &BreakpointDlg::delete_current_breakpoints));
  assert(connect(btn_remove_all_, &QPushButton::clicked, this, &BreakpointDlg::delete_all_breakpoints));
  assert(connect(btn_box_, &QDialogButtonBox::accepted, this, qOverload<>(&QDialog::accept)));

  setLayout(gridLayout);
}

BreakpointDlg::BreakpointDlg(BreakpointManager *breakpoint_manager, QWidget *parent)
    : QDialog(parent) //
    , breakpoint_manager_{breakpoint_manager} //
{
  setup_ui();

  for (auto bp : breakpoint_manager_->breakpoints()) {
    auto txt = bp_to_qstring(bp);
    lst_brk_->addItem(txt);
  }
  lst_brk_->sortItems();
  te_new_brk_->clear();
  assert(connect(te_new_brk_, &QLineEdit::editingFinished, this, &BreakpointDlg::add_breakpoint));
}

BreakpointDlg::~BreakpointDlg() = default;

void BreakpointDlg::delete_current_breakpoints()  {
  auto selected_item = lst_brk_->currentItem();
  if (!selected_item) return;

  uint16_t bp;
  bool ok = qstring_to_bp(selected_item->text(), bp);
  if (ok) {
    breakpoint_manager_->clear_breakpoint(bp);
    delete lst_brk_->takeItem(lst_brk_->row(selected_item));
  }
}

void BreakpointDlg::delete_all_breakpoints()  {
    breakpoint_manager_->clear_all();
    lst_brk_->clear();
}

void BreakpointDlg::add_breakpoint() {
  bool ok;
  uint16_t brk = te_new_brk_->text().toInt(&ok, 16);
  if (!ok) {
    te_new_brk_->selectAll();
    return;
  }

  if (breakpoint_manager_->set_breakpoint(brk)) {
    te_new_brk_->clear();
    auto txt = bp_to_qstring(brk);
    lst_brk_->addItem(txt);
    lst_brk_->sortItems();
  }
}

QString BreakpointDlg::bp_to_qstring(uint16_t bp){
  return QString("&%1").arg(bp, 4, 16, QChar('0'));
}

bool BreakpointDlg::qstring_to_bp(QString bps, uint16_t & bp){
  if( bps.isNull() || bps.isEmpty()) return false;
  bool ok;
  bp = bps.mid(1).toUInt(&ok, 16);
  return ok;
}
