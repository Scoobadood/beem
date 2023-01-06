#ifndef DISASM_VIEW_H
#define DISASM_VIEW_H

#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include "disassembler.h"

namespace Ui {
class DisasmView;
}

class DisasmView : public QWidget {
 Q_OBJECT

 public:
  explicit DisasmView(QWidget *parent = nullptr);
  ~DisasmView() override;

  void set_data(std::shared_ptr<std::vector<uint8_t>> memory);

 private:
  void start_disassembly();
  void disassembly_complete();
  void dis_start_addr_changed();
  void enable_view();
  void disable_view();

  Ui::DisasmView *ui;
  Disassembler disassembler_;
  std::shared_ptr<std::vector<uint8_t>> memory_;
  uint16_t disassemble_from_;
  std::map<uint16_t,uint16_t> pc_to_row_;
  std::map<uint16_t,uint16_t> row_to_pc_;
  QFuture<std::vector<Operation>> disassembly_future_;
  uint8_t error_;
};

#endif // DISASM_VIEW_H
