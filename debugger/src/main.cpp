

#include "deeb_window.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  DeebWindow w;
  w.show();
  return a.exec();
}
