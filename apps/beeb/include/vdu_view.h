#ifndef BEEB_VDU_VIEW_H
#define BEEB_VDU_VIEW_H

#include <QWidget>
#include <mutex>
#include <vector>

class VduView : public QWidget {
 Q_OBJECT

 public:
  explicit VduView(QWidget *parent = nullptr);

  // Called from the emulation thread — thread-safe.
  void screen_changed(int32_t width, int32_t height, const std::vector<uint8_t> &scr_data);

  void keyPressEvent(QKeyEvent *event) override;

  void keyReleaseEvent(QKeyEvent *event) override;

  void enterEvent(QEnterEvent *event) override;

  void leaveEvent(QEvent *event) override;

  bool eventFilter(QObject *object, QEvent *event) override;

 signals:
  void press_key(uint8_t key);
  void release_key(uint8_t key);

 protected:
  void paintEvent(QPaintEvent *event) override;

 private slots:
  // Called on the main thread via QueuedConnection to copy the back buffer
  // into the QImage and trigger a repaint.
  void consume_frame();

 private:
  // Back buffer — written by the emulation thread, read by consume_frame().
  std::mutex frame_mutex_;
  std::vector<uint8_t> frame_buffer_;
  int32_t frame_w_{0};
  int32_t frame_h_{0};

  // QImage — written and read only on the main thread.
  int32_t last_width_;
  int32_t last_height_;
  QImage *image_;
};

#endif //BEEB_VDU_VIEW_H
