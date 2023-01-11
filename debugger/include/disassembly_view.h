//
// Created by Dave Durbin on 9/1/2023.
//

#ifndef CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
#define CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_

#include "disassembler.h"

#include <set>
#include <QPlainTextEdit>

class DisassemblyView : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit DisassemblyView(QWidget *parent = nullptr);
  ~DisassemblyView() override;

  void set_data(std::shared_ptr<std::vector<uint8_t>> memory);

  void mousePressEvent(QMouseEvent *e) override;

 public slots:
  void set_current_address(uint16_t pc);

 signals:
  void breakpoint_set(uint16_t);
  void breakpoint_cleared(uint16_t);

 private:
/**
 * Disassemble sufficient data to populate one screen.
 */
  void update_disassembly();
  void set_bp_formatting(Operation &op, QTextCursor &cursor);
  void clear_bp_formatting(Operation &op, QTextCursor &cursor);

  Disassembler disassembler_;

  std::shared_ptr<std::vector<uint8_t>> data_;

  std::map<uint16_t, uint16_t> addr_to_row_;
  std::map<uint16_t, uint16_t> row_to_addr_;
  std::vector<Operation> disassembly_;

  uint16_t disassemble_from_;

  uint8_t error_;

  std::set<uint16_t> breakpoint_lines_;
};

#endif //CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
