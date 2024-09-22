#ifndef BEEB_INCLUDE_BREAKPOINT_DIALOG_H_
#define BEEB_INCLUDE_BREAKPOINT_DIALOG_H_

#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QListWidget>

#include "breakpoint_manager.h"

class BreakpointDlg : public QDialog {
  Q_OBJECT

 public:
  explicit BreakpointDlg(BreakpointManager * breakpoint_manager,
                         QWidget *parent = nullptr);
  ~BreakpointDlg() override;

 private:
  void setup_ui();
  void add_breakpoint();
  void delete_current_breakpoints();
  void delete_all_breakpoints();
  static QString bp_to_qstring(uint16_t bp);
  bool qstring_to_bp(QString bps, uint16_t & bp);

  BreakpointManager * breakpoint_manager_;
  QLineEdit *te_new_brk_;
  QDialogButtonBox *btn_box_;
  QPushButton *btn_remove_;
  QPushButton *btn_remove_all_;
  QListWidget *lst_brk_;
};

#endif // BEEB_INCLUDE_BREAKPOINT_DIALOG_H_
