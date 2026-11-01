#include "i18n/languages.h"
#include "persistence/data_directory.h"
#include "persistence/language_preferences.h"
#include "persistence/sqlite_project_repository.h"
#include "ui/main_window.h"
#include "ui/native_edit_menus.h"
#include <QCoreApplication>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char* argv[]) {
#ifdef Q_OS_LINUX
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME")) {
    qputenv("QT_QPA_PLATFORMTHEME", "gtk3");
  }
#endif
  QApplication application(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("de.appfabrik.restaurierungsheft"));
  QCoreApplication::setOrganizationDomain(QStringLiteral("appfabrik.de"));
  QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
  QApplication::setApplicationDisplayName(QStringLiteral("Restoration Journal"));
#ifdef Q_OS_LINUX
  QGuiApplication::setDesktopFileName(QCoreApplication::applicationName());
#endif
  QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/branding/icon-256.png")));
  restoration::Translations translations;
  translations.install(QStringLiteral("auto"));
  QCommandLineParser parser;
  parser.setApplicationDescription(
      QCoreApplication::translate("App", "Eine lokale Sammlung für deine Restaurierungsprojekte."));
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption dataOption(
      QStringLiteral("data-dir"),
      QCoreApplication::translate("App", "Isoliertes absolutes Datenverzeichnis."),
      QCoreApplication::translate("App", "pfad"));
  parser.addOption(dataOption);
  parser.process(application);
  try {
    restoration::initializePdfFonts();
    const auto directory =
        restoration::resolveDataDirectory(parser.value(dataOption), parser.isSet(dataOption));
    const auto preference = restoration::readLanguagePreference(directory);
    translations.install(preference.value);
#ifdef Q_OS_MACOS
    restoration::NativeEditMenus nativeEditMenus;
#endif
    restoration::StorageController controller(
        [directory] { return std::make_unique<restoration::SqliteProjectRepository>(directory); });
    restoration::MainWindow window(controller, directory, nullptr, {}, preference.value);
    window.show();
    if (preference.readFailed) {
      QTimer::singleShot(0, &window, [&window] {
        QMessageBox::warning(
            &window, QCoreApplication::translate("App", "Sprache"),
            QCoreApplication::translate(
                "App", "Die gespeicherte Sprache konnte nicht gelesen werden. Englisch wird "
                       "verwendet. Wähle die Sprache erneut in den Einstellungen."));
      });
    }
    return QApplication::exec();
  } catch (const std::exception& error) {
    QMessageBox::critical(
        nullptr, QCoreApplication::translate("App", "Restoration Journal kann nicht starten"),
        QString::fromUtf8(error.what()));
    return 1;
  }
}
