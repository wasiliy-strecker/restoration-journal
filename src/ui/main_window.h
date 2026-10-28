#pragma once

#include "ui/export_dialogs.h"
#include "ui/project_editor.h"

#include <QMainWindow>
#include <QPointer>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace restoration {

class MainWindow final : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(StorageController& controller, QString dataDirectory, QWidget* parent = nullptr,
             std::shared_ptr<ExportDialogs> exportDialogs = {});

protected:
  void closeEvent(QCloseEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  void refresh(std::optional<qint64> preferredId = std::nullopt, int fallbackRow = 0);
  void updateSelection();
  void updateActions();
  std::optional<RestorationProject> selectedProject() const;
  void openEditor(bool editing);
  void deleteSelected();
  void showAbout();
  void exportSelected();
  std::shared_ptr<ExportDialogs> exportDialogs_;
  bool choosingExport_ = false;
  StorageController& controller_;
  QString dataDirectory_;
  ProjectList projects_;
  ProjectList visibleProjects_;
  QPointer<ProjectEditor> editor_;
  QLineEdit* search_;
  QTableWidget* table_;
  QPlainTextEdit* note_;
  QPlainTextEdit* checklist_;
  PhotoView* photo_;
  std::optional<ProjectDraft> selectedDetails_;
  QImage selectedPreview_;
  quint64 previewGeneration_ = 0;
  QLabel* detailTitle_;
  QLabel* state_;
  QLabel* feedback_;
  QLabel* count_;
  QPushButton* add_;
  QPushButton* edit_;
  QPushButton* remove_;
  QPushButton* reload_;
  QPushButton* export_;
  bool ready_ = false;
};

} // namespace restoration
