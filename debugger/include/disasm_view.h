#ifndef DISASM_VIEW_H
#define DISASM_VIEW_H

#include <QWidget>

namespace Ui {
class DisasmView;
}

class DisasmView : public QWidget {
 Q_OBJECT

 public:
  explicit DisasmView(QWidget *parent = nullptr);
  ~DisasmView();

  void set_data(std::shared_ptr<std::vector<uint8_t>> memory);

 private:
  Ui::DisasmView *ui;
  std::shared_ptr<std::vector<uint8_t>> memory_;
  uint16_t disassemble_from_;
};

#endif // DISASM_VIEW_H
