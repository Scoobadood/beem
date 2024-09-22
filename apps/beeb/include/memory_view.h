#ifndef BEEB_MEMORY_VIEW_H
#define BEEB_MEMORY_VIEW_H

#include <QWidget>
#include <cstdint>
#include <QTextEdit>

#include "data_display_widget.h"

class MemoryView : public DataDisplayWidget {
 Q_OBJECT

 public:
  explicit MemoryView(QWidget *parent = nullptr);
  ~MemoryView() override;

  void set_data(const std::vector<uint8_t> &data) override;
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  enum Layout {
    BYTES = 0,
    WORDS = 1,
    ASCII = 2
  };
  void set_layout(Layout layout);

 signals:
  void needs_data(QWidget *src, uint16_t start_addr, uint32_t num_bytes);

 private:
  void layout_content();
  void layout_content_bytes();
  void layout_content_words();
  uint32_t compute_data_required();
  void scroll_to(int32_t address);

  /* Displayed address of first byte in memory */
  uint16_t first_address_;

  /* Row number of the first displayed row 0x0000 - 0xffff */
  uint32_t first_row_;

  /* Data for the current page. Should be just enough to fill */
  std::vector<uint8_t> data_;

  /* Sizes on screen of rendered columns and address */
  QSize addr_size_;
  QSize col_size_;
  QSize dbl_col_size_;

  /* Dimension of the screen in lines of characters */
  int32_t num_rows_;
  int32_t num_cols_;

  QTextEdit *te_memory_;
  QScrollBar *sb_memory_;

  Layout layout_;
};

#endif // BEEB_MEMORY_VIEW_H
