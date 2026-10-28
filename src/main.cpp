#include "persistence/data_directory.h"
#include "persistence/sqlite_project_repository.h"
#include "ui/main_window.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QTranslator>

int main(int argc, char* argv[]) {
  QApplication application(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("de.appfabrik.restaurierungsheft"));
  QCoreApplication::setOrganizationDomain(QStringLiteral("appfabrik.de"));
  QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
  QApplication::setApplicationDisplayName(QStringLiteral("RestaurierungsHeft"));
  QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/branding/icon-256.png")));
  QTranslator translator;
  if (translator.load(QStringLiteral("qtbase_de"),
                      QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
    application.installTranslator(&translator);
  }
  QCommandLineParser parser;
  parser.setApplicationDescription(
      QStringLiteral("Eine lokale Sammlung für deine Restaurierungsprojekte."));
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption dataOption(QStringLiteral("data-dir"),
                                QStringLiteral("Isoliertes absolutes Datenverzeichnis."),
                                QStringLiteral("pfad"));
  parser.addOption(dataOption);
  parser.process(application);
  try {
    restoration::initializePdfFonts();
    const auto directory =
        restoration::resolveDataDirectory(parser.value(dataOption), parser.isSet(dataOption));
    restoration::StorageController controller(
        [directory] { return std::make_unique<restoration::SqliteProjectRepository>(directory); });
    restoration::MainWindow window(controller, directory);
    window.show();
    return QApplication::exec();
  } catch (const std::exception& error) {
    QMessageBox::critical(nullptr, QStringLiteral("RestaurierungsHeft kann nicht starten"),
                          QString::fromUtf8(error.what()));
    return 1;
  }
}
