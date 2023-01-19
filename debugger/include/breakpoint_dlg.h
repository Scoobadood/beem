#ifndef BREAKPOINT_DIALOG_H
#define BREAKPOINT_DIALOG_H

#include <QDialog>
#include <set>

namespace Ui {
class BreakpointDlg;
}

class BreakpointDlg : public QDialog {
 Q_OBJECT

 public:
  explicit BreakpointDlg(QWidget *parent = nullptr);
  ~BreakpointDlg() override;

  void set_breakpoints(const std::set<uint16_t> &breakpoints);
  [[nodiscard]] inline const std::set<uint16_t> breakpoints() const {return breakpoints_;}

 private:
  Ui::BreakpointDlg *ui;
  void add_brk();
  std::set<uint16_t> breakpoints_;

};

#endif // BREAKPOINT_DIALOG_H
