#ifndef CRTC_VIEW_H
#define CRTC_VIEW_H

#include <QWidget>
#include <QLineEdit>
#include "debuggable_crtc.h"

namespace Ui {
class CrtcView;
}

const uint8_t NUM_REGISTERS = 18;

class CrtcView : public QWidget {
 Q_OBJECT

 public:
  explicit CrtcView(QWidget *parent = nullptr);
  ~CrtcView();

  void set_crtc(DebuggableCrtc *crtc);
  void set_mode(uint8_t mode);
 private:
  void update_fields();

  void breakout_r3();
  void breakout_r8();
  void breakout_r10();
  void update_scr_addr();
  void update_curs_addr();
  void update_lp_addr();

  Ui::CrtcView *ui_;
  DebuggableCrtc *crtc_;
  std::shared_ptr<Bus> bus_;
  uint8_t last_registers_[NUM_REGISTERS];
  uint8_t current_registers_[NUM_REGISTERS];
  std::vector<QLineEdit *> register_fields_;
};

#endif // CRTC_VIEW_H
