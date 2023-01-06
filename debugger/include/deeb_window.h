#ifndef DEEBWINDOW_H
#define DEEBWINDOW_H

#include <QMainWindow>
#include "memory.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DeebWindow; }
QT_END_NAMESPACE

class DeebWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit DeebWindow(QWidget *parent = nullptr);
  ~DeebWindow() override;

 private:
  Ui::DeebWindow *ui;
  void load_file();
  void load_rom();

  Memory *memory_;
};
#endif // DEEBWINDOW_H
