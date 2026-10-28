#include "ui/export_dialogs.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>

namespace restoration {
QString ExportDialogs::choose(QWidget* parent, const QString& suggestedName) {
  const auto documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  return QFileDialog::getSaveFileName(parent, QStringLiteral("Projekt als PDF exportieren"),
                                      QDir(documents).filePath(suggestedName),
                                      QStringLiteral("PDF-Dokument (*.pdf)"), nullptr,
                                      QFileDialog::DontConfirmOverwrite);
}
bool ExportDialogs::confirmReplacement(QWidget* parent, const QString& path) {
  QMessageBox question(QMessageBox::Question, QStringLiteral("PDF ersetzen?"),
                       QStringLiteral("Die Datei existiert bereits:\n%1\n\nMöchtest du sie nach "
                                      "erfolgreicher PDF-Erzeugung ersetzen?")
                           .arg(path),
                       QMessageBox::NoButton, parent);
  question.setTextFormat(Qt::PlainText);
  auto* cancel = question.addButton(QStringLiteral("Abbrechen"), QMessageBox::RejectRole);
  auto* replace = question.addButton(QStringLiteral("Ersetzen"), QMessageBox::AcceptRole);
  replace->setShortcut(QKeySequence(QStringLiteral("Alt+E")));
  question.setDefaultButton(cancel);
  question.setEscapeButton(cancel);
  question.exec();
  return question.clickedButton() == replace;
}
} // namespace restoration
