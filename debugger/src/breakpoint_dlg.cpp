#include "breakpoint_dlg.h"
#include "ui_breakpoint_dlg.h"
#include <QTextList>

BreakpointDlg::BreakpointDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BreakpointDlg) {
  ui->setupUi(this);
  ui->te_new_brk->clear();
  connect(ui->te_new_brk, &QLineEdit::editingFinished, this, &BreakpointDlg::add_brk);
}

BreakpointDlg::~BreakpointDlg() {
  delete ui;
}

void BreakpointDlg::set_breakpoints(const std::set<uint16_t> &breakpoints) {
  breakpoints_ = breakpoints;
  ui->lst_brk->clear();
  for (uint16_t b: breakpoints) {
    auto txt = QString("&%1").arg(b, 4, 16, QChar('0'));
    ui->lst_brk->addItem(txt);
  }
  ui->lst_brk->sortItems();
}

void BreakpointDlg::add_brk() {
  bool ok;
  uint16_t brk = ui->te_new_brk->text().toInt(&ok, 16);
  if (!ok) {
    ui->te_new_brk->selectAll();
    return;
  }

  if (breakpoints_.count(brk) > 0) return;
  breakpoints_.emplace(brk);
  auto txt = QString("&%1").arg(brk, 4, 16, QChar('0'));
  ui->lst_brk->addItem(txt);
  ui->lst_brk->sortItems();
}

