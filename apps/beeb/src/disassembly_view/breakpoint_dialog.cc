#include "breakpoint_dialog.h"

#include <QGridLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

// ─── helpers ─────────────────────────────────────────────────────────────────

QString BreakpointDlg::addr_to_qstring(uint16_t addr) {
  return QString("&%1").arg(addr, 4, 16, QChar('0'));
}

bool BreakpointDlg::qstring_to_addr(const QString& s, uint16_t& addr) {
  if (s.isNull() || s.isEmpty()) return false;
  bool ok;
  addr = static_cast<uint16_t>(s.mid(1).toUInt(&ok, 16));
  return ok;
}

// ─── UI setup ────────────────────────────────────────────────────────────────

static QWidget* make_list_tab(QLineEdit*& te_out, QListWidget*& lst_out,
                              QPushButton*& btn_remove_out,
                              QPushButton*& btn_remove_all_out,
                              QWidget* parent) {
  auto* w = new QWidget(parent);
  auto* grid = new QGridLayout(w);

  te_out = new QLineEdit(w);
  te_out->setInputMask(QString::fromUtf8("hhhh"));
  te_out->setText(QString::fromUtf8(""));
  grid->addWidget(te_out, 0, 0, 1, 2);

  lst_out = new QListWidget(w);
  grid->addWidget(lst_out, 1, 0, 3, 2);

  btn_remove_out = new QPushButton("Remove", w);
  grid->addWidget(btn_remove_out, 4, 0, 1, 1);

  btn_remove_all_out = new QPushButton("Remove All", w);
  grid->addWidget(btn_remove_all_out, 4, 1, 1, 1);

  w->setLayout(grid);
  return w;
}

void BreakpointDlg::setup_ui() {
  resize(250, 340);
  auto* vbox = new QVBoxLayout(this);

  tab_widget_ = new QTabWidget(this);

  // Breakpoints tab
  auto* bp_tab = make_list_tab(te_new_brk_, lst_brk_,
                                btn_remove_, btn_remove_all_,
                                tab_widget_);
  tab_widget_->addTab(bp_tab, "Breakpoints");

  // Watches tab
  auto* watch_tab = make_list_tab(te_new_watch_, lst_watch_,
                                   btn_watch_remove_, btn_watch_remove_all_,
                                   tab_widget_);
  tab_widget_->addTab(watch_tab, "Watches");

  vbox->addWidget(tab_widget_);

  btn_box_ = new QDialogButtonBox(this);
  btn_box_->setOrientation(Qt::Horizontal);
  btn_box_->setStandardButtons(QDialogButtonBox::Ok);
  vbox->addWidget(btn_box_);

  setLayout(vbox);

  // Breakpoint tab connections
  assert(connect(btn_remove_,     &QPushButton::clicked, this, &BreakpointDlg::delete_current_breakpoints));
  assert(connect(btn_remove_all_, &QPushButton::clicked, this, &BreakpointDlg::delete_all_breakpoints));
  assert(connect(te_new_brk_,     &QLineEdit::editingFinished, this, &BreakpointDlg::add_breakpoint));

  // Watch tab connections
  assert(connect(btn_watch_remove_,     &QPushButton::clicked, this, &BreakpointDlg::delete_current_watches));
  assert(connect(btn_watch_remove_all_, &QPushButton::clicked, this, &BreakpointDlg::delete_all_watches));
  assert(connect(te_new_watch_,         &QLineEdit::editingFinished, this, &BreakpointDlg::add_watch));

  assert(connect(btn_box_, &QDialogButtonBox::accepted, this, qOverload<>(&QDialog::accept)));
}

// ─── construction ────────────────────────────────────────────────────────────

BreakpointDlg::BreakpointDlg(BreakpointManager* breakpoint_manager, QWidget* parent)
    : QDialog(parent)
    , breakpoint_manager_{breakpoint_manager}
{
  setup_ui();

  for (auto bp : breakpoint_manager_->breakpoints()) {
    lst_brk_->addItem(addr_to_qstring(bp));
  }
  lst_brk_->sortItems();
  te_new_brk_->clear();

  for (auto w : breakpoint_manager_->watches()) {
    lst_watch_->addItem(addr_to_qstring(w));
  }
  lst_watch_->sortItems();
  te_new_watch_->clear();
}

BreakpointDlg::~BreakpointDlg() = default;

// ─── breakpoint slots ────────────────────────────────────────────────────────

void BreakpointDlg::add_breakpoint() {
  bool ok;
  uint16_t addr = static_cast<uint16_t>(te_new_brk_->text().toInt(&ok, 16));
  if (!ok) {
    te_new_brk_->selectAll();
    return;
  }
  if (breakpoint_manager_->set_breakpoint(addr)) {
    te_new_brk_->clear();
    lst_brk_->addItem(addr_to_qstring(addr));
    lst_brk_->sortItems();
  }
}

void BreakpointDlg::delete_current_breakpoints() {
  auto* item = lst_brk_->currentItem();
  if (!item) return;
  uint16_t addr;
  if (qstring_to_addr(item->text(), addr)) {
    breakpoint_manager_->clear_breakpoint(addr);
    delete lst_brk_->takeItem(lst_brk_->row(item));
  }
}

void BreakpointDlg::delete_all_breakpoints() {
  breakpoint_manager_->clear_all();
  lst_brk_->clear();
}

// ─── watch slots ─────────────────────────────────────────────────────────────

void BreakpointDlg::add_watch() {
  bool ok;
  uint16_t addr = static_cast<uint16_t>(te_new_watch_->text().toInt(&ok, 16));
  if (!ok) {
    te_new_watch_->selectAll();
    return;
  }
  if (breakpoint_manager_->set_watch(addr)) {
    te_new_watch_->clear();
    lst_watch_->addItem(addr_to_qstring(addr));
    lst_watch_->sortItems();
  }
}

void BreakpointDlg::delete_current_watches() {
  auto* item = lst_watch_->currentItem();
  if (!item) return;
  uint16_t addr;
  if (qstring_to_addr(item->text(), addr)) {
    breakpoint_manager_->clear_watch(addr);
    delete lst_watch_->takeItem(lst_watch_->row(item));
  }
}

void BreakpointDlg::delete_all_watches() {
  breakpoint_manager_->clear_all_watches();
  lst_watch_->clear();
}
