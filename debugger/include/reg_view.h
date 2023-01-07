#ifndef REG_VIEW_H
#define REG_VIEW_H

#include <QWidget>
#include "m6502.h"

namespace Ui {
class RegisterView;
}

class RegisterView : public QWidget {
 Q_OBJECT

 public:
  explicit RegisterView(QWidget *parent = nullptr);
  ~RegisterView() override;

  void set_cpu(M6502 *cpu);
  void update_flags();

 private:
  Ui::RegisterView *ui;

  M6502 *cpu_;

  uint16_t old_pc_;
  uint8_t old_sp_;
  uint8_t old_flags_;
};

#endif // REG_VIEW_H
