/**
 * Shows the CPU registers and flags.
 * Whenever a value changes, it's highlighted in RED
 * If the value remains the same over two cycles then it returns to BLACK
 *
 */
#ifndef BEEB_REG_VIEW_H
#define BEEB_REG_VIEW_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>

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
  void make_ui(QWidget *form);
  QWidget * configure_flags( QWidget * parent);

  QLineEdit * reg_a_;
  QLineEdit * reg_x_;
  QLineEdit * reg_y_;

  QLineEdit * reg_pc_;
  QLineEdit * reg_sp_;

  uint16_t old_pc_;
  uint8_t old_sp_;
  uint8_t old_a_;
  uint8_t old_x_;
  uint8_t old_y_;
  uint8_t old_flags_;
  QLineEdit *flag_labels_[8];
};

#endif // BEEB_REG_VIEW_H
