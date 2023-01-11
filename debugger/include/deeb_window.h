#ifndef DEEBWINDOW_H
#define DEEBWINDOW_H

#include <QMainWindow>
#include "memory.h"
#include "m6502.h"
#include "bus.h"

#include <set>

QT_BEGIN_NAMESPACE
namespace Ui { class DeebWindow; }
QT_END_NAMESPACE

class DeebWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit DeebWindow(QWidget *parent = nullptr);
  ~DeebWindow() override;

 public slots:
  void breakpoint_set(uint16_t brk_addr);
  void breakpoint_cleared(uint16_t brk_addr);

 signals:
  void flags_changed(uint8_t flags);
  void registers_changed(uint8_t a, uint8_t x, uint8_t y, uint16_t pc, uint16_t sp);
  void pc_changed(uint16_t pc);
  void bus_changed(Bus & bus);

 private:
 /**
  * Toggle RST line and wait for CPU to reset.
  */
  void reset_cpu();
  
  Ui::DeebWindow *ui;
  void load_file();
  void load_rom();
  void step();
  void run();

  std::set<uint16_t> breakpoints_;

  Memory *memory_;
  M6502 *cpu_;
  Bus bus_;
};
#endif // DEEBWINDOW_H
