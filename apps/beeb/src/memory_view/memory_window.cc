#include "memory_window.h"

#include <QToolBar>
#include <QButtonGroup>
MemoryWindow::MemoryWindow(QWidget *parent)
    : QMainWindow(parent) //
{
  setWindowTitle("Memory Inspector");
  setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

  memory_view_ = new MemoryView(this);
  setCentralWidget(memory_view_);

  auto toolbar = new QToolBar(this);
  byte_button_ = new QAction("8",toolbar);
  assert(connect(byte_button_, &QAction::triggered, [&](){
    memory_view_->set_layout(MemoryView::BYTES);
    byte_button_->setEnabled(false);
    word_button_->setEnabled(true);
    ascii_button_->setEnabled(true);
  }));
  word_button_ = new QAction("16",toolbar);
  assert(connect(word_button_, &QAction::triggered, [&](){
    memory_view_->set_layout(MemoryView::WORDS);
    byte_button_->setEnabled(true);
    word_button_->setEnabled(false);
    ascii_button_->setEnabled(true);
  }));
  ascii_button_ = new QAction("a",toolbar);
  assert(connect(ascii_button_, &QAction::triggered, [&](){
    memory_view_->set_layout(MemoryView::ASCII);
    byte_button_->setEnabled(true);
    word_button_->setEnabled(true);
    ascii_button_->setEnabled(false);
  }));
  start_addr_ = new QLineEdit(toolbar);

  toolbar->addAction(byte_button_);
  toolbar->addAction(word_button_);
  toolbar->addAction(ascii_button_);
  toolbar->addWidget(start_addr_);
  toolbar->addSeparator();

  byte_button_->setEnabled(false);
  word_button_->setEnabled(true);
  ascii_button_->setEnabled(true);

  addToolBar(toolbar);
}


MemoryWindow::~MemoryWindow() = default;

MemoryView * MemoryWindow::view() {
  return memory_view_;
}