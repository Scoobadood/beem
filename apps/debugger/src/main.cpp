

#include "deeb_window.h"

#include <spdlog/cfg/env.h>

#include <QApplication>

int main(int argc, char *argv[]) {
  spdlog::cfg::load_env_levels();
  QApplication a(argc, argv);
  DeebWindow w;
  w.show();
  auto x = a.exec();
  spdlog::shutdown();
  return x;
}
