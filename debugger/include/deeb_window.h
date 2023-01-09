#ifndef DEEBWINDOW_H
#define DEEBWINDOW_H

#include <QMainWindow>
#include "memory.h"
#include "m6502.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DeebWindow; }
QT_END_NAMESPACE

class DeebWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit DeebWindow(QWidget *parent = nullptr);
  ~DeebWindow() override;

  signals:
  void flags_changed(uint8_t flags);
  void registers_changed(uint8_t a, uint8_t x, uint8_t y, uint16_t pc, uint16_t sp);

 private:
  Ui::DeebWindow *ui;
  void load_file();
  void load_rom();
  void step();

  Memory *memory_;
  M6502 * cpu_;
  uint64_t pins_;
};
#endif // DEEBWINDOW_H
