#include "ui/export_dialogs.h"
#include "ui/export_file_dialog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>

namespace restoration {
QString ExportDialogs::choose(QWidget* parent, const QString& suggestedName) {
  const auto documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#ifdef Q_OS_MACOS
  return chooseExportFile(parent, QDir(documents), suggestedName, true, true);
#else
  return QFileDialog::getSaveFileName(
      parent, QCoreApplication::translate("App", "Projekt als PDF exportieren"),
      QDir(documents).filePath(suggestedName),
      QCoreApplication::translate("App", "PDF-Dokument (*.pdf)"), nullptr,
      QFileDialog::DontConfirmOverwrite);
#endif
}
bool ExportDialogs::confirmReplacement(QWidget* parent, const QString& path) {
  QMessageBox question(
      QMessageBox::Question, QCoreApplication::translate("App", "PDF ersetzen?"),
      QCoreApplication::translate("App", "Die Datei existiert bereits:\n%1\n\nMöchtest du sie nach "
                                         "erfolgreicher PDF-Erzeugung ersetzen?")
          .arg(path),
      QMessageBox::NoButton, parent);
  question.setTextFormat(Qt::PlainText);
  auto* cancel =
      question.addButton(QCoreApplication::translate("App", "Abbrechen"), QMessageBox::RejectRole);
  auto* replace =
      question.addButton(QCoreApplication::translate("App", "Ersetzen"), QMessageBox::AcceptRole);
  replace->setShortcut(QKeySequence(QStringLiteral("Alt+E")));
  question.setDefaultButton(cancel);
  question.setEscapeButton(cancel);
  question.exec();
  return question.clickedButton() == replace;
}
} // namespace restoration
