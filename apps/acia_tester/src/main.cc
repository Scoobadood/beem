#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include "spdlog/sinks/basic_file_sink.h"
#include "acia_test_window.h"
#include "debuggable_acia.h"

#include <QApplication>


int main(int argc, char *argv[]) {
  spdlog::cfg::load_env_levels();

  // Create logs
  auto logger = spdlog::basic_logger_mt("BusDance", "logs/bus-log.txt", true);
  logger->flush_on(spdlog::level::debug);


  QApplication a(argc, argv);

  auto acia = std::make_shared<DebuggableAcia>(0xfe08);

  auto main_window = new AciaTestWindow(acia);

  main_window->show();
  auto x = a.exec();
  spdlog::shutdown();
  return x;
}
