#include "cassette_window.h"
#include "spdlog/spdlog.h"

#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

CassetteWindow::CassetteWindow(QWidget *parent)
    : QMainWindow(parent) //
    , cassette_data_{} //
{
  setWindowTitle("Cassette");
  setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

  // Add a "no tape" label
  no_data_label_ = new QLabel("No data. Load a UEF file.", this);
  no_data_label_->setAlignment(Qt::AlignmentFlag::AlignCenter);

  // Add a table view to show data when it is loaded
  table_view_ = new QTableView(this);
  // Set the table to be read-only
  table_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  model_ = new QStandardItemModel(this);

  // Add a stacked view
  stack_ = new QStackedWidget(this);
  stack_->addWidget(no_data_label_);
  stack_->addWidget(table_view_);

  // Prefer the label
  stack_->setCurrentWidget(no_data_label_);

  // Add a button bar
  auto button_bar = new QWidget(this);
  auto bb_layout = new QHBoxLayout(button_bar);

  auto cancel_btn = new QPushButton("Cancel", this);
  assert(connect(cancel_btn, &QPushButton::clicked, this, &CassetteWindow::close_cassette_window));
  bb_layout->addWidget(cancel_btn);

  load_file_btn_ = new QPushButton("Load File", this);
  load_file_btn_->setEnabled(false);
  assert(connect(load_file_btn_, &QPushButton::clicked, [&]() {
    spdlog::error( "load_file clicked");
                   // If there's a selected file
                   auto sel_item = table_view_->selectionModel()->selectedRows().first();
                   auto row = sel_item.row();
    spdlog::error( "  sel_item : {}", sel_item.row());
                   auto model_index = model_->index(row, 0);
                   auto name = model_->data(model_index).toString().toStdString();
    spdlog::error( "      name : {}", name);
                   // Emit the right message
                   auto tape_file = cassette_data_.at(name);
                   emit load_cassette_file(tape_file);
                 }
  ));
  bb_layout->addWidget(load_file_btn_);

  auto load_uef_btn = new QPushButton("Load UEF", this);
  assert(connect(load_uef_btn, &QPushButton::clicked, this, &CassetteWindow::load_uef_file));

  bb_layout->
      addWidget(load_uef_btn);
  button_bar->
      setLayout(bb_layout);

  auto w = new QWidget();
  auto layout = new QVBoxLayout(w);
  layout->
      addWidget(stack_);
  layout->
      addWidget(button_bar);

  setCentralWidget(w);
}

void CassetteWindow::load_uef_file() {
  QFileDialog fileDialog(this);
  fileDialog.setWindowTitle("Open UEF File");
  fileDialog.setFileMode(QFileDialog::ExistingFiles);
  fileDialog.setNameFilter("UEF files (*.uef);;All files (*.*)");
  fileDialog.setViewMode(QFileDialog::List);

  // Show the dialog modally
  if (fileDialog.exec() == QDialog::Accepted) {
    QStringList fileNames = fileDialog.selectedFiles();
    if (!fileNames.isEmpty()) {
      QString fileName = fileNames.first();
      try {
        auto uef = std::make_shared<UefData>(UefData::FromFile(fileName.toStdString()));
        cassette_data_ = load_tape_data_from_uef(*uef);
        populate_tape_data(cassette_data_);
        emit tape_inserted(uef);
      } catch (std::exception &e) {//catch
        auto msg = fmt::format("Could not open file {},  :{}", fileName.toStdString(), e.what());
        QMessageBox::warning(this, "Error", QString::fromStdString(msg));
      }
    }
  }
}

void
CassetteWindow::populate_tape_data(const std::map<std::string, std::shared_ptr<TapeFile>> &tape_data) {
  model_->clear();
  model_->setColumnCount(4);
  QLocale locale;
  model_->setHorizontalHeaderLabels({"File", "Size", "Load Addr", "Exec Addr"});
  for (const auto &tape_file : tape_data) {
    QList<QStandardItem *> row_items;
    const auto td = tape_file.second;
    row_items.append(new QStandardItem(QString::fromStdString(td->name)));
    row_items.append(new QStandardItem(locale.toString(td->data.size())));
    row_items.append(new QStandardItem("&" + QString::number(td->load_addr, 16)));
    row_items.append(new QStandardItem("&" + QString::number(td->exec_addr, 16)));

    model_->appendRow(row_items);
  }
  table_view_->setModel(model_);
  table_view_->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  assert(connect(table_view_->selectionModel(), &QItemSelectionModel::selectionChanged, [&]() {
    load_file_btn_->setEnabled(true);
  }));
  stack_->setCurrentWidget(table_view_);
}