#ifndef DISASSEMBLY_VIEW_H
#define DISASSEMBLY_VIEW_H

#include "disassembler.h"

#include <set>

#include <QWidget>
#include <QTextCursor>

namespace Ui {
  class DisassemblyView;
}

class DisassemblyView : public QWidget {
Q_OBJECT

public:
  explicit DisassemblyView(QWidget *parent = nullptr);

  ~DisassemblyView();

  void update_breakpoints(const std::set<uint16_t> &breakpoints);

  void set_data(const std::vector<uint8_t> &data);

  void set_symbols(const std::map<uint16_t, std::string>& symbols);

  void resizeEvent(QResizeEvent *event) override;

  bool eventFilter(QObject *obj, QEvent *event) override;

  void mousePressEvent(QMouseEvent *e) override;

  void resize(const QSize & size);

  void scroll_to(uint16_t address);

  void set_pc(uint16_t pc);

signals:

  void needs_data(QWidget *src, uint16_t start_addr, uint32_t num_bytes);

  void breakpoint_set(uint16_t);

  void breakpoint_cleared(uint16_t);


private:
  void clear_brkpt_formatting(QTextCursor cursor);

  void set_brkpt_formatting(QTextCursor cursor);

  struct FormattedOperation;

  FormattedOperation format_for_display(const Operation &op);

  void disassemble_data(const std::vector<uint8_t> &data);

  void layout_disassembly();

  bool address_on_screen(uint16_t addr, uint32_t *row = nullptr, float *proportion = nullptr);

  bool has_breakpoint(uint16_t addr) const;

  void redraw(QTextCursor cursor, bool is_pc);

  Ui::DisassemblyView *ui;

  /** Performs the disassembly */
  Disassembler disassembler_;

  /** Convert mouse clicks to addresses for breakpoint setting etc. */
  std::map<uint16_t, uint16_t> addr_to_row_;
  std::map<uint16_t, uint16_t> row_to_addr_;

  /** The current PC if set */
  uint16_t current_pc_;

  /** Height of a row of text */
  int32_t row_height_;

  /** The number of displayed rows */
  uint32_t displayed_rows_;

  /** The actual first displayed offset */
  uint32_t first_displayed_byte_offset_;

  /** The addresses at which a breakpoint has been set */
  std::set<uint16_t> breakpoint_addresses_;

  /** A map of adress to symbol lookup */
  std::map<uint16_t, QString> symbols_;

  const QColor pc_colour_;

  std::vector<Operation> disassembly_;
};

#endif // DISASSEMBLY_VIEW_H
