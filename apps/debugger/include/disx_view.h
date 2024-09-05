//
// Created by Dave Durbin on 9/1/2023.
//

#ifndef CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
#define CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_

#include "disassembler.h"

#include <set>
#include <QPlainTextEdit>

class DisView : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit DisView(QWidget *parent = nullptr);
  ~DisView() override;

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

  std::map<uint16_t, uint16_t> addr_to_row_;
  std::map<uint16_t, uint16_t> row_to_addr_;

  /** The known good address from which we disassmble forwards */
  uint16_t disassemble_from_;

  /** The current PC if set */
  uint16_t current_pc_;

  /** The number of displyed rows */
  uint32_t displayed_rows_;

  /** The notional first displayed row */
  uint32_t first_displayed_row_;

  /** The actual first displayed offset */
  uint32_t first_displayed_byte_offset_;

  /** The start address of the whole memory range */
  uint16_t first_address_;

  uint8_t error_;

  /** The addresses at which a breakpoint has been set */
  std::set<uint16_t> breakpoint_addresses_;

  std::set<uint16_t> breakpoint_lines_;

  /** A map of adress to symbol lookup */
  std::map<uint16_t, QString> symbols_;
};

#endif //CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
