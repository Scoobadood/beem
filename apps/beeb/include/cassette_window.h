#ifndef LIBS_BEEB_CASETTE_WINDOW_H_
#define LIBS_BEEB_CASETTE_WINDOW_H_

#include <QMainWindow>
#include <QTableView>
#include <QLabel>

#include <UEF/uef.h>
#include <QStackedWidget>
#include <QPushButton>
#include <QStandardItemModel>

class CassetteWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit CassetteWindow(QWidget *parent = nullptr);

  signals:
  void close_cassette_window();
  void load_cassette_file(const std::shared_ptr<TapeFile>&);
  void tape_inserted(std::shared_ptr<UefData> uef);

 private:
  void load_uef_file();
  void populate_tape_data( const std::map<std::string, std::shared_ptr<TapeFile>> & tape_data);
  QTableView * table_view_;
  QLabel * no_data_label_;
  QStackedWidget * stack_;
  QPushButton * load_file_btn_;
  QStandardItemModel * model_;
  std::map<std::string, std::shared_ptr<TapeFile>> cassette_data_;
};

#endif // LIBS_BEEB_CASETTE_WINDOW_H_
