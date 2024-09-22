#ifndef ACIA_TEST_WINDOW_H
#define ACIA_TEST_WINDOW_H

#include <QMainWindow>
#include "debuggable_acia.h"

namespace Ui {
class AciaTestWindow;
}

class AciaTestWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit AciaTestWindow(const std::shared_ptr<DebuggableAcia>& acia, QWidget *parent = nullptr);
  ~AciaTestWindow();

 public slots:
  void tick();

 private:
  void reload_ui();
  void send_ctl();
  void send_data();
  void read_status();
  void set_cts();
  void clr_cts();

  Ui::AciaTestWindow *ui;
  std::shared_ptr<DebuggableAcia> acia_;
  std::shared_ptr<Bus> bus_;

};

#endif // ACIA_TEST_WINDOW_H
