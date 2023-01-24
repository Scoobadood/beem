#ifndef DEEBWINDOW_H
#define DEEBWINDOW_H

#include "beeb.h"

#include <QMainWindow>

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

  void load_symbols();

signals:

  void flags_changed(uint8_t flags);

  void registers_changed(uint8_t a, uint8_t x, uint8_t y, uint16_t pc, uint16_t sp);

  void pc_changed(uint16_t pc);

  void bus_changed(const std::shared_ptr<Bus> &bus);

private slots:

  void on_act_edit_breakpoints_triggered();

  void beeb_data_needed(QWidget *source, uint16_t start_address, uint32_t num_bytes);

  void set_debug_buttons_paused();

  void set_debug_buttons_running();

private:
  /**
   * Toggle RST line and wait for CPU to reset.
   */
  void reset_cpu();

  void step();

  void run();

  void brk();

  Ui::DeebWindow *ui;

  bool brk_requested_;

  std::set<uint16_t> breakpoints_;

  Beeb *beeb_;
};

#endif // DEEBWINDOW_H
