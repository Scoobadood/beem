#include "history_window.h"

#include <Disassembler/operation_formatter.h>
#include <QFont>
#include <QVBoxLayout>
#include "spdlog/fmt/fmt.h"

HistoryWindow::HistoryWindow(std::vector<ExecutionEngine::InsnRecord> history,
                             const Disassembler& disasm,
                             QWidget* parent)
    : QDialog(parent)
{
  setWindowTitle("Instruction History");
  resize(780, 520);

  text_ = new QTextEdit(this);
  text_->setReadOnly(true);
  text_->setFont(QFont("Monaco", 11));
  text_->setLineWrapMode(QTextEdit::NoWrap);

  // Build a local disassembler so we can freely call set_base_address().
  Disassembler local_disasm;
  local_disasm.set_symbols(disasm.symbols());

  for (const auto& rec : history) {
    local_disasm.set_base_address(rec.pc);
    uint16_t offset = 0;
    uint8_t  err    = 0;
    auto op = local_disasm.disassemble_one(
        reinterpret_cast<const uint8_t*>(&rec.mem4), 4, offset, err);

    auto line = format_single_line(op, local_disasm.symbols());
    auto full = fmt::format("{:50s} A:{:02x} X:{:02x} Y:{:02x} SP:{:02x} F:{}",
                            line, rec.a, rec.x, rec.y, rec.sp,
                            format_flags(rec.flags));
    text_->append(QString::fromStdString(full));
  }
  text_->moveCursor(QTextCursor::End);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(text_);
  setLayout(layout);
}
