#ifndef BEEB_SRC_BEEB_WINDOW_H_
#define BEEB_SRC_BEEB_WINDOW_H_

#include <QMainWindow>

#include "beeb.h"
#include "vdu_view.h"
#include "debugger_window.h"
#include "memory_window.h"
#include "led_label_widget.h"

class CrtWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit CrtWindow(Beeb& beeb, QWidget *parent = nullptr);

  ~CrtWindow() override;

  void closeEvent(QCloseEvent *event) override;
  void data_requested(QWidget * source, uint16_t address, uint32_t num_bytes);

 public slots:

 private:
  Beeb& beeb_;
  LedLabelWidget * cassette_drive_led_;

  VduView *vdu_;
};

#endif // BEEB_SRC_BEEB_WINDOW_H_
