#include "persistence/sqlite_project_repository.h"
#include "ui/main_window.h"
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <atomic>

using namespace restoration;
namespace {
class Dialogs final : public ExportDialogs {
public:
  QString path;
  bool approved = false;
  int choices = 0;
  int confirmations = 0;
  QString choose(QWidget*, const QString&) override {
    ++choices;
    return path;
  }
  bool confirmReplacement(QWidget*, const QString&) override {
    ++confirmations;
    return approved;
  }
};
class SlowRepository final : public ProjectRepository {
public:
  TransferDraftList transferSnapshot() override {
    throw std::logic_error("Unexpected test transfer");
  }
  CollectionImportResult importEntries(const TransferDraftList&) override {
    throw std::logic_error("Unexpected test transfer");
  }
  SlowRepository(const QString& directory, std::shared_ptr<std::atomic<int>> reads)
      : inner_(directory), reads_(std::move(reads)) {}
  void initialize() override { inner_.initialize(); }
  ProjectList findAll() override { return inner_.findAll(); }
  ProjectDraft find(qint64 id) override {
    if (QThread::currentThread() == qApp->thread())
      throw std::runtime_error("GUI thread access");
    ++(*reads_);
    QThread::msleep(180);
    return inner_.find(id);
  }
  RestorationProject create(const ProjectDraft& draft) override { return inner_.create(draft); }
  RestorationProject update(qint64 id, const ProjectDraft& draft) override {
    return inner_.update(id, draft);
  }
  void remove(qint64 id) override { inner_.remove(id); }

private:
  SqliteProjectRepository inner_;
  std::shared_ptr<std::atomic<int>> reads_;
};
QByteArray bytes(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return file.readAll();
}
QString pdfText(const QString& path) {
  QProcess process;
  process.start(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
  if (!process.waitForFinished() || process.exitCode() != 0)
    return {};
  return QString::fromUtf8(process.readAllStandardOutput());
}
} // namespace
class PdfDialogsTest final : public QObject {
  Q_OBJECT
private slots:
  void initTestCase() { initializePdfFonts(); }
  void noSelectionAndRealPickerCancellation() {
    QTemporaryDir directory;
    StorageController controller(
        [&] { return std::make_unique<SqliteProjectRepository>(directory.path()); });
    MainWindow window(controller, directory.path());
    window.show();
    auto* button = window.findChild<QPushButton*>(QStringLiteral("exportPdf"));
    QVERIFY(!button->isEnabled());
    QSignalSpy loaded(&controller, &StorageController::loaded);
    QVERIFY(QTest::qWaitFor([&] { return loaded.size() == 1; }, 5000));
    QSignalSpy saved(&controller, &StorageController::saved);
    controller.save(0, {QStringLiteral("Projekt"), {}, {}, {}});
    QVERIFY(QTest::qWaitFor([&] { return saved.size() == 1; }, 5000));
    QVERIFY(QTest::qWaitFor([&] { return button->isEnabled(); }, 5000));
    bool seen = false;
    QTimer::singleShot(0, &window, [&] {
      auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
      if (dialog) {
        seen = true;
        dialog->reject();
      }
    });
    button->click();
    QVERIFY(seen);
    QVERIFY(!controller.isBusy());
    QVERIFY(button->isEnabled());
  }
  void replacementDefaultIsCancel() {
    ExportDialogs dialogs;
    bool seen = false;
    QTimer::singleShot(0, qApp, [&] {
      auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
      if (box) {
        seen = box->defaultButton()->text() == QStringLiteral("Abbrechen");
        QTest::keyClick(box, Qt::Key_Return);
      }
    });
    QVERIFY(!dialogs.confirmReplacement(nullptr, QStringLiteral("/tmp/beispiel.pdf")));
    QVERIFY(seen);
  }
  void cancelRejectReplaceAndRetry() {
    QTemporaryDir directory, destination;
    {
      SqliteProjectRepository repo(directory.path());
      repo.initialize();
      repo.create({QStringLiteral("Projekt"), {}, {}, {}});
    }
    const auto originalDb = bytes(directory.filePath(QStringLiteral("restaurierungsheft.sqlite3")));
    auto dialogs = std::make_shared<Dialogs>();
    StorageController controller(
        [&] { return std::make_unique<SqliteProjectRepository>(directory.path()); });
    MainWindow window(controller, directory.path(), nullptr, dialogs);
    window.show();
    auto* button = window.findChild<QPushButton*>(QStringLiteral("exportPdf"));
    QVERIFY(QTest::qWaitFor([&] { return button->isEnabled(); }, 5000));
    button->click(); // Empty picker result.
    QVERIFY(!controller.isBusy());
    dialogs->path = destination.filePath(QStringLiteral("existing.pdf"));
    {
      QFile file(dialogs->path);
      QVERIFY(file.open(QIODevice::WriteOnly));
      file.write("original");
    }
    button->click();
    QVERIFY(QTest::qWaitFor([&] { return !controller.isBusy(); }, 5000));
    QCOMPARE(dialogs->confirmations, 1);
    QCOMPARE(bytes(dialogs->path), QByteArray("original"));
    dialogs->approved = true;
    QSignalSpy exported(&controller, &StorageController::exported);
    button->click();
    QVERIFY(QTest::qWaitFor([&] { return exported.size() == 1; }, 5000));
    QVERIFY(pdfText(dialogs->path).contains(QStringLiteral("Projekt")));
    QSignalSpy failed(&controller, &StorageController::failed);
    dialogs->path = destination.filePath(QStringLiteral("missing/error.pdf"));
    button->click();
    QVERIFY(QTest::qWaitFor([&] { return failed.size() == 1; }, 5000));
    QVERIFY(window.findChild<QLabel*>(QStringLiteral("mainFeedback"))
                ->text()
                .contains(QStringLiteral("Zielordner")));
    dialogs->path = destination.filePath(QStringLiteral("retry.pdf"));
    button->click();
    QVERIFY(QTest::qWaitFor([&] { return exported.size() == 2; }, 5000));
    QCOMPARE(bytes(directory.filePath(QStringLiteral("restaurierungsheft.sqlite3"))), originalDb);
  }
  void busyResponsiveStableIdAndSavedSnapshot() {
    QTemporaryDir directory, destination;
    {
      SqliteProjectRepository repo(directory.path());
      repo.initialize();
      repo.create({QStringLiteral("Gleich"), {}, {}, QStringLiteral("Alter Listenstand")});
      repo.create({QStringLiteral("Gleich"), {}, {}, QStringLiteral("Falsches Projekt")});
    }
    auto reads = std::make_shared<std::atomic<int>>(0);
    auto dialogs = std::make_shared<Dialogs>();
    dialogs->path = destination.filePath(QStringLiteral("snapshot.pdf"));
    StorageController controller(
        [&] { return std::make_unique<SlowRepository>(directory.path(), reads); });
    MainWindow window(controller, directory.path(), nullptr, dialogs);
    window.show();
    auto* button = window.findChild<QPushButton*>(QStringLiteral("exportPdf"));
    QVERIFY(QTest::qWaitFor(
        [&] { return window.findChild<QPushButton*>(QStringLiteral("editProject"))->isEnabled(); },
        5000));
    const auto connection = QStringLiteral("external-snapshot-update");
    {
      auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
      db.setDatabaseName(directory.filePath(QStringLiteral("restaurierungsheft.sqlite3")));
      QVERIFY(db.open());
      QSqlQuery query(db);
      QVERIFY(query.exec(
          QStringLiteral("UPDATE projects SET note='Gespeicherter neuer Stand' WHERE id=1")));
    }
    QSqlDatabase::removeDatabase(connection);
    const auto before = bytes(directory.filePath(QStringLiteral("restaurierungsheft.sqlite3")));
    QSignalSpy exported(&controller, &StorageController::exported);
    int ticks = 0;
    QTimer timer;
    timer.setInterval(5);
    connect(&timer, &QTimer::timeout, &window, [&] { ++ticks; });
    timer.start();
    button->click();
    button->click();
    QVERIFY(controller.isBusy());
    QVERIFY(!window.close());
    QVERIFY(!window.findChild<QPushButton*>(QStringLiteral("addProject"))->isEnabled());
    window.findChild<QTableWidget*>(QStringLiteral("projectsTable"))->selectRow(1);
    QVERIFY(QTest::qWaitFor([&] { return exported.size() == 1; }, 5000));
    QCOMPARE(dialogs->choices, 1);
    QVERIFY(ticks > 5);
    const auto text = pdfText(dialogs->path);
    QVERIFY(text.contains(QStringLiteral("Gespeicherter neuer Stand")));
    QVERIFY(!text.contains(QStringLiteral("Falsches Projekt")));
    QVERIFY(!text.contains(QStringLiteral("Alter Listenstand")));
    QCOMPARE(bytes(directory.filePath(QStringLiteral("restaurierungsheft.sqlite3"))), before);
  }
  void destroyedWindowWhileWorkerFinishes() {
    QTemporaryDir directory, destination;
    {
      SqliteProjectRepository repo(directory.path());
      repo.initialize();
      repo.create({QStringLiteral("Projekt"), {}, {}, {}});
    }
    auto reads = std::make_shared<std::atomic<int>>(0);
    auto dialogs = std::make_shared<Dialogs>();
    dialogs->path = destination.filePath(QStringLiteral("finished.pdf"));
    StorageController controller(
        [&] { return std::make_unique<SlowRepository>(directory.path(), reads); });
    auto window = std::make_unique<MainWindow>(controller, directory.path(), nullptr, dialogs);
    window->show();
    QVERIFY(QTest::qWaitFor(
        [&] { return window->findChild<QPushButton*>(QStringLiteral("editProject"))->isEnabled(); },
        5000));
    const auto before = reads->load();
    QSignalSpy exported(&controller, &StorageController::exported);
    window->findChild<QPushButton*>(QStringLiteral("exportPdf"))->click();
    QVERIFY(QTest::qWaitFor([&] { return reads->load() > before; }, 5000));
    window.reset();
    QVERIFY(QTest::qWaitFor([&] { return exported.size() == 1; }, 5000));
    QVERIFY(pdfText(dialogs->path).contains(QStringLiteral("Projekt")));
  }
};
QTEST_MAIN(PdfDialogsTest)
#include "pdf_dialogs_test.moc"
