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

  void set_data(const std::vector<uint8_t> & data, uint16_t base_address);

  void set_symbols(const std::map<uint16_t, std::string> &symbols);

  void mousePressEvent(QMouseEvent *e) override;

  void update_disassembly();

 public slots:
  void set_current_address(uint16_t pc);

 signals:
  void breakpoint_set(uint16_t);
  void breakpoint_cleared(uint16_t);

 private:
/**
 * Disassemble sufficient data to populate one screen.
 */
  void set_bp_formatting(Operation &op, QTextCursor &cursor);
  void clear_bp_formatting(Operation &op, QTextCursor &cursor);
  std::vector<QString> format_for_display(const Operation &op);

  Disassembler disassembler_;

  std::vector<uint8_t> data_;

  std::map<uint16_t, uint16_t> addr_to_row_;
  std::map<uint16_t, uint16_t> row_to_addr_;
  std::vector<Operation> disassembly_;

  uint16_t disassemble_from_;

  uint8_t error_;

  std::set<uint16_t> breakpoint_lines_;

  std::map<uint16_t, QString> symbols_;
};

#endif //CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
