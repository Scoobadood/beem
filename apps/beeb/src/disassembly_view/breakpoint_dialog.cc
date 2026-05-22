#include "breakpoint_dialog.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <optional>

// ─── helpers ─────────────────────────────────────────────────────────────────

QString BreakpointDlg::addr_to_qstring(uint16_t addr) {
  return QString("&%1").arg(addr, 4, 16, QChar('0'));
}

// Format: "&XXXX" or "&XXXX = YY"
QString BreakpointDlg::watch_to_qstring(uint16_t addr, std::optional<uint8_t> tv) {
  auto s = addr_to_qstring(addr);
  if (tv) s += QString(" = %1").arg(*tv, 2, 16, QChar('0')).toUpper();
  return s;
}

// Format: "&XXXX" or "&XXXX -> &YYYY"
QString BreakpointDlg::logpoint_to_qstring(uint16_t pc_addr, std::optional<uint16_t> mem_addr) {
  auto s = addr_to_qstring(pc_addr);
  if (mem_addr) s += QString(" -> &%1").arg(*mem_addr, 4, 16, QChar('0')).toUpper();
  return s;
}

bool BreakpointDlg::qstring_to_addr(const QString& s, uint16_t& addr) {
  if (s.isNull() || s.isEmpty()) return false;
  bool ok;
  // Strip any " = YY" suffix before parsing the address
  auto addr_part = s.mid(1).section(' ', 0, 0);
  addr = static_cast<uint16_t>(addr_part.toUInt(&ok, 16));
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
  btn_remove_out->setFocusPolicy(Qt::NoFocus);
  grid->addWidget(btn_remove_out, 4, 0, 1, 1);

  btn_remove_all_out = new QPushButton("Remove All", w);
  btn_remove_all_out->setFocusPolicy(Qt::NoFocus);
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

  // Watches tab — custom layout with optional trigger value input
  auto* watch_tab = new QWidget(tab_widget_);
  auto* watch_grid = new QGridLayout(watch_tab);

  // Row 0: address input and "= value" input side by side
  auto* addr_row = new QWidget(watch_tab);
  auto* addr_hbox = new QHBoxLayout(addr_row);
  addr_hbox->setContentsMargins(0, 0, 0, 0);
  te_new_watch_ = new QLineEdit(addr_row);
  te_new_watch_->setInputMask(QString::fromUtf8("hhhh"));
  te_new_watch_->setPlaceholderText("addr");
  addr_hbox->addWidget(te_new_watch_);
  addr_hbox->addWidget(new QLabel("=", addr_row));
  te_new_watch_value_ = new QLineEdit(addr_row);
  te_new_watch_value_->setInputMask(QString::fromUtf8("hh"));
  te_new_watch_value_->setMaximumWidth(40);
  te_new_watch_value_->setPlaceholderText("any");
  addr_hbox->addWidget(te_new_watch_value_);
  addr_row->setLayout(addr_hbox);
  watch_grid->addWidget(addr_row, 0, 0, 1, 2);

  lst_watch_ = new QListWidget(watch_tab);
  watch_grid->addWidget(lst_watch_, 1, 0, 3, 2);

  btn_watch_remove_     = new QPushButton("Remove",     watch_tab);
  btn_watch_remove_->setFocusPolicy(Qt::NoFocus);
  btn_watch_remove_all_ = new QPushButton("Remove All", watch_tab);
  btn_watch_remove_all_->setFocusPolicy(Qt::NoFocus);
  watch_grid->addWidget(btn_watch_remove_,     4, 0, 1, 1);
  watch_grid->addWidget(btn_watch_remove_all_, 4, 1, 1, 1);
  watch_tab->setLayout(watch_grid);

  tab_widget_->addTab(watch_tab, "Watches");

  // Logpoints tab — PC address + optional memory address to log
  auto* logpt_tab = new QWidget(tab_widget_);
  auto* logpt_grid = new QGridLayout(logpt_tab);

  auto* logpt_row = new QWidget(logpt_tab);
  auto* logpt_hbox = new QHBoxLayout(logpt_row);
  logpt_hbox->setContentsMargins(0, 0, 0, 0);
  te_new_logpt_pc_ = new QLineEdit(logpt_row);
  te_new_logpt_pc_->setInputMask(QString::fromUtf8("hhhh"));
  te_new_logpt_pc_->setPlaceholderText("pc");
  logpt_hbox->addWidget(te_new_logpt_pc_);
  logpt_hbox->addWidget(new QLabel("->", logpt_row));
  te_new_logpt_mem_ = new QLineEdit(logpt_row);
  te_new_logpt_mem_->setInputMask(QString::fromUtf8("hhhh"));
  te_new_logpt_mem_->setMaximumWidth(60);
  te_new_logpt_mem_->setPlaceholderText("addr");
  logpt_hbox->addWidget(te_new_logpt_mem_);
  logpt_row->setLayout(logpt_hbox);
  logpt_grid->addWidget(logpt_row, 0, 0, 1, 2);

  lst_logpt_ = new QListWidget(logpt_tab);
  logpt_grid->addWidget(lst_logpt_, 1, 0, 3, 2);

  btn_logpt_remove_     = new QPushButton("Remove",     logpt_tab);
  btn_logpt_remove_->setFocusPolicy(Qt::NoFocus);
  btn_logpt_remove_all_ = new QPushButton("Remove All", logpt_tab);
  btn_logpt_remove_all_->setFocusPolicy(Qt::NoFocus);
  logpt_grid->addWidget(btn_logpt_remove_,     4, 0, 1, 1);
  logpt_grid->addWidget(btn_logpt_remove_all_, 4, 1, 1, 1);
  logpt_tab->setLayout(logpt_grid);

  tab_widget_->addTab(logpt_tab, "Logpoints");

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

  // Logpoint tab connections
  assert(connect(btn_logpt_remove_,     &QPushButton::clicked, this, &BreakpointDlg::delete_current_logpoints));
  assert(connect(btn_logpt_remove_all_, &QPushButton::clicked, this, &BreakpointDlg::delete_all_logpoints));
  assert(connect(te_new_logpt_pc_,      &QLineEdit::editingFinished, this, &BreakpointDlg::add_logpoint));

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

  for (auto& [addr, tv] : breakpoint_manager_->watches()) {
    lst_watch_->addItem(watch_to_qstring(addr, tv));
  }
  lst_watch_->sortItems();
  te_new_watch_->clear();
  te_new_watch_value_->clear();

  for (auto& [pc_addr, mem_addr] : breakpoint_manager_->logpoints()) {
    lst_logpt_->addItem(logpoint_to_qstring(pc_addr, mem_addr));
  }
  lst_logpt_->sortItems();
  te_new_logpt_pc_->clear();
  te_new_logpt_mem_->clear();
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
  uint16_t addr = static_cast<uint16_t>(te_new_watch_->text().toUInt(&ok, 16));
  if (!ok) {
    te_new_watch_->selectAll();
    return;
  }
  std::optional<uint8_t> tv;
  const auto val_text = te_new_watch_value_->text().trimmed();
  if (!val_text.isEmpty()) {
    bool val_ok;
    uint8_t v = static_cast<uint8_t>(val_text.toUInt(&val_ok, 16));
    if (val_ok) tv = v;
  }
  if (breakpoint_manager_->set_watch(addr, tv)) {
    te_new_watch_->clear();
    te_new_watch_value_->clear();
    lst_watch_->addItem(watch_to_qstring(addr, tv));
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

// ─── logpoint slots ───────────────────────────────────────────────────────────

void BreakpointDlg::add_logpoint() {
  bool ok;
  uint16_t pc_addr = static_cast<uint16_t>(te_new_logpt_pc_->text().toUInt(&ok, 16));
  if (!ok) {
    te_new_logpt_pc_->selectAll();
    return;
  }
  std::optional<uint16_t> mem_addr;
  const auto mem_text = te_new_logpt_mem_->text().trimmed();
  if (!mem_text.isEmpty()) {
    bool mem_ok;
    uint16_t v = static_cast<uint16_t>(mem_text.toUInt(&mem_ok, 16));
    if (mem_ok) mem_addr = v;
  }
  if (breakpoint_manager_->set_logpoint(pc_addr, mem_addr)) {
    te_new_logpt_pc_->clear();
    te_new_logpt_mem_->clear();
    lst_logpt_->addItem(logpoint_to_qstring(pc_addr, mem_addr));
    lst_logpt_->sortItems();
  }
}

void BreakpointDlg::delete_current_logpoints() {
  auto* item = lst_logpt_->currentItem();
  if (!item) return;
  uint16_t pc_addr;
  if (qstring_to_addr(item->text(), pc_addr)) {
    breakpoint_manager_->clear_logpoint(pc_addr);
    delete lst_logpt_->takeItem(lst_logpt_->row(item));
  }
}

void BreakpointDlg::delete_all_logpoints() {
  breakpoint_manager_->clear_all_logpoints();
  lst_logpt_->clear();
}
