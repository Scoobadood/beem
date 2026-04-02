#ifndef BEEB_INCLUDE_BREAKPOINT_DIALOG_H_
#define BEEB_INCLUDE_BREAKPOINT_DIALOG_H_

#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTabWidget>

#include "breakpoint_manager.h"

class BreakpointDlg : public QDialog {
  Q_OBJECT

 public:
  explicit BreakpointDlg(BreakpointManager * breakpoint_manager,
                         QWidget *parent = nullptr);
  ~BreakpointDlg() override;

 private:
  void setup_ui();

  // Breakpoint tab slots
  void add_breakpoint();
  void delete_current_breakpoints();
  void delete_all_breakpoints();

  // Watch tab slots
  void add_watch();
  void delete_current_watches();
  void delete_all_watches();

  static QString addr_to_qstring(uint16_t addr);
  static bool qstring_to_addr(const QString& s, uint16_t& addr);

  BreakpointManager * breakpoint_manager_;

  // Breakpoints tab widgets
  QLineEdit    *te_new_brk_;
  QListWidget  *lst_brk_;
  QPushButton  *btn_remove_;
  QPushButton  *btn_remove_all_;

  // Watches tab widgets
  QLineEdit    *te_new_watch_;
  QListWidget  *lst_watch_;
  QPushButton  *btn_watch_remove_;
  QPushButton  *btn_watch_remove_all_;

  QTabWidget        *tab_widget_;
  QDialogButtonBox  *btn_box_;
};

#endif // BEEB_INCLUDE_BREAKPOINT_DIALOG_H_
