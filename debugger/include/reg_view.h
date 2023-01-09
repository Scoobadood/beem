/**
 * Shows the CPU registers and flags.
 * Whenever a value changes, it's highlighted in RED
 * If the value remains the same over two cycles then it returns to BLACK
 *
 */
#ifndef REG_VIEW_H
#define REG_VIEW_H

#include <QWidget>
#include <QLabel>
#include "m6502.h"

namespace Ui {
class RegisterView;
}

class RegisterView : public QWidget {
 Q_OBJECT

 public:
  explicit RegisterView(QWidget *parent = nullptr);
  ~RegisterView() override;

 public slots:
  void set_flags(uint8_t new_flags);
  void set_registers(uint8_t new_a, uint8_t new_x, uint8_t new_y, uint16_t new_pc, uint16_t new_sp);

 private:
  void update_pc(uint16_t new_pc);
  void update_sp(uint8_t new_sp);
  Ui::RegisterView *ui;

  uint16_t old_pc_;
  uint8_t old_sp_;
  uint8_t old_a_;
  uint8_t old_x_;
  uint8_t old_y_;
  uint8_t old_flags_;
  QLabel *flag_labels_[8];
};

#endif // REG_VIEW_H
