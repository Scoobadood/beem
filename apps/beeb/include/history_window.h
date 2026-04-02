#pragma once

#include <QDialog>
#include <QTextEdit>
#include <vector>

#include "execution_engine.h"
#include <Disassembler/disassembler.h>

class HistoryWindow : public QDialog {
  Q_OBJECT

 public:
  HistoryWindow(std::vector<ExecutionEngine::InsnRecord> history,
                const Disassembler& disasm,
                QWidget* parent = nullptr);
  ~HistoryWindow() override = default;

 private:
  QTextEdit* text_;
};
