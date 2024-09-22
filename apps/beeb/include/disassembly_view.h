#ifndef BEEB_DISASSEMBLY_VIEW_H
#define BEEB_DISASSEMBLY_VIEW_H

#include <Disassembler/disassembler.h>

#include <set>

#include <QWidget>
#include <QTextEdit>
#include <QTextCursor>
#include "data_display_widget.h"
#include "breakpoint_manager.h"
struct ColourScheme;

class DisassemblyView : public DataDisplayWidget {
 Q_OBJECT

 public:
  explicit DisassemblyView(BreakpointManager *breakpoint_manager, QWidget *parent = nullptr);

  ~DisassemblyView() override;

  void set_data(const std::vector<uint8_t> &data) override;

  void set_symbols(const std::map<uint16_t, std::string> &symbols);

  void resizeEvent(QResizeEvent *event) override;

  bool eventFilter(QObject *obj, QEvent *event) override;

  void mousePressEvent(QMouseEvent *e) override;

  void resize(const QSize &size);

  void scroll_to(uint16_t address);

  void set_pc(uint16_t pc);

 public slots:
  void breakpoints_changed();

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

  bool is_labelled(uint16_t address, QString &label);

  QString address_or_label(uint16_t addr, bool zp = false);

  bool address_on_screen(uint16_t addr, uint32_t *row = nullptr, float *proportion = nullptr);

  void redraw(QTextCursor cursor, bool is_pc);

  QTextEdit *te_disassembly_;
  QScrollBar *sb_disassembly_;

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
  BreakpointManager *breakpoint_manager_;

  /** A map of adress to symbol lookup */
  std::map<uint16_t, QString> symbols_;

  /** The colour scheme for this window */
  std::unique_ptr<ColourScheme> colour_scheme_;

  std::vector<Operation> disassembly_;
};

#endif // BEEB_DISASSEMBLY_VIEW_H
