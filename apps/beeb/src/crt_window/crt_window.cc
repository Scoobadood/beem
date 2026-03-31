#include "crt_window.h"
#include "led_label_widget.h"
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <QMenuBar>
#include <QCloseEvent>
#include <QStatusBar>
#include <utility>

CrtWindow::CrtWindow(Beeb& beeb, QWidget *parent)
    : QMainWindow(parent) //
    , beeb_{beeb} //
{
  // Add a status bar LED for cassette drive
  cassette_drive_led_ = new LedLabelWidget("cassette\nmotor", this);
  statusBar()->addWidget(cassette_drive_led_);
  beeb_.add_cassette_listener([&](bool cassette_on){
    if( cassette_on) {
      cassette_drive_led_->turn_on();
    } else {
      cassette_drive_led_->turn_off();
    }
  });

  // Add a VDU view
  vdu_ = new VduView(this);
  setCentralWidget(vdu_);

  // Link the VDU
  beeb_.crt()->set_renderer([=](int32_t w, int32_t h, const std::vector<uint8_t> &scr_data) {
    vdu_->screen_changed(w, h, scr_data);
  });
  // And keyboard
  assert(connect(vdu_, &VduView::press_key, [&](uint8_t key) { beeb_.press_key(key); }));
  assert(connect(vdu_, &VduView::release_key, [&](uint8_t key) { beeb_.release_key(key); }));
  resize(640, 512);
}

CrtWindow::~CrtWindow() = default;

void CrtWindow::closeEvent(QCloseEvent *event) {
  // TODO: Shut down beeb worker
  event->accept();
}

void CrtWindow::data_requested(QWidget *source, uint16_t address, uint32_t num_bytes) {
  ((DataDisplayWidget *) source)->set_data(beeb_.get_memory_contents(address, num_bytes));
}