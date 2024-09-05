#include "spdlog/cfg/env.h"
#include "spdlog/spdlog.h"
#include "crtc_view.h"
#include "debuggable_crtc.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <QApplication>
#include <QLayout>
#include <QMainWindow>

int main(int argc, char *argv[]) {
  spdlog::cfg::load_env_levels();

  // CReate logs
  auto logger = spdlog::basic_logger_mt("BusDance", "logs/bus-log.txt", true);
  logger->flush_on(spdlog::level::debug);


  QApplication a(argc, argv);
  QMainWindow w;

  auto crtc_view = new CrtcView();
  auto crtc = new DebuggableCrtc();
  crtc_view->set_crtc(crtc);
  crtc_view->set_mode(0);

  w.setCentralWidget(crtc_view);

  w.show();
  auto x = a.exec();
  spdlog::shutdown();
  return x;
}
