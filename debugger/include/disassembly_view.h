//
// Created by Dave Durbin on 9/1/2023.
//

#ifndef CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
#define CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_

#include <QPlainTextEdit>
#include "disassembler.h"

class DisassemblyView : public QPlainTextEdit {
 public:
  explicit DisassemblyView(QWidget *parent = nullptr);
  ~DisassemblyView() override;

  void set_data(std::shared_ptr<std::vector<uint8_t>> memory);

 public slots:
  void set_current_address(uint16_t pc);

 private:
/**
 * Disassemble sufficient data to populate one screen.
 */
  void update_disassembly();

  Disassembler disassembler_;

  std::shared_ptr<std::vector<uint8_t>> data_;

  std::map<uint16_t, uint16_t> addr_to_row_;
  std::map<uint16_t, uint16_t> row_to_addr_;
  std::vector<Operation> disassembly_;

  uint32_t top_row_;
  uint32_t last_row_;
  uint16_t disassemble_from_;

  uint8_t error_;
};

#endif //CHIPS_M6502_DEBUGGER_INCLUDE_DISASSEMBLYVIEW_H_
