#ifndef BEEB_MEMORY_WINDOW_H
#define BEEB_MEMORY_WINDOW_H

#include <QMainWindow>
#include <QLineEdit>

#include "memory_view.h"

class MemoryWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit MemoryWindow(QWidget *parent = nullptr);
  ~MemoryWindow();
  MemoryView *view();

 private:
  MemoryView *memory_view_;
  QAction * byte_button_;
  QAction * word_button_;
  QAction * ascii_button_;
  QLineEdit * start_addr_;
};

#endif // BEEB_MEMORY_WINDOW_H