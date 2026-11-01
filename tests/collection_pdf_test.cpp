#include "pdf/pdf_export.h"
#include "persistence/sqlite_project_repository.h"
#include <QBuffer>
#include <QFile>
#include <QProcess>
#include <QScopeGuard>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <system_error>
using namespace restoration;

namespace {
QString textFrom(const QString& path) {
  QProcess process;
  process.start(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
  if (!process.waitForFinished() || process.exitCode() != 0)
    throw std::runtime_error("Cannot read PDF");
  return QString::fromUtf8(process.readAllStandardOutput());
}
QByteArray bytes(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    throw std::runtime_error("Cannot read fixture");
  return file.readAll();
}
void write(const QString& path, const QByteArray& content) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
    throw std::runtime_error("Cannot write fixture");
}
QByteArray photo() {
  QImage image(180, 300, QImage::Format_RGB32);
  image.fill(QColor(QStringLiteral("#B36B34")));
  QBuffer buffer;
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return buffer.data();
}
} // namespace

class CollectionPdfTest final : public QObject {
  Q_OBJECT
private slots:
  void initTestCase() { initializePdfFonts(); }

  void allProjectsFreshPagesAndSavedFields() {
    QTemporaryDir data, output;
    SqliteProjectRepository repository(data.path());
    repository.initialize();
    ProjectDraft last{QStringLiteral("Zulu"), {}, {}, QStringLiteral("Final saved note")};
    last.status = ProjectStatus::Finished;
    last.color = QStringLiteral("#315A7D80");
    last.photo = photo();
    last.steps = {{QUuid::createUuid(), QStringLiteral("Preserved work step"), true, 4}};
    repository.create(last);
    repository.create({QStringLiteral("Alpha"), {}, {}, QStringLiteral("First duplicate")});
    repository.create({QStringLiteral("Alpha"), {}, {}, QStringLiteral("Second duplicate")});
    const auto database = data.filePath(QStringLiteral("restaurierungsheft.sqlite3"));
    const auto before = bytes(database);
    const auto path = output.filePath(QStringLiteral("all.pdf"));
    exportCollectionPdf(repository, inspectExportTarget(path, data.path()), false, data.path());
    const auto pages = textFrom(path).split(QChar('\f'), Qt::SkipEmptyParts);
    QCOMPARE(pages.size(), 3);
    QVERIFY(pages[0].contains(QStringLiteral("First duplicate")));
    QVERIFY(!pages[0].contains(QStringLiteral("Second duplicate")));
    QVERIFY(pages[1].contains(QStringLiteral("Second duplicate")));
    QVERIFY(!pages[1].contains(QStringLiteral("Zulu")));
    QVERIFY(pages[2].contains(last.name));
    QVERIFY(pages[2].contains(last.note));
    QVERIFY(pages[2].contains(last.color));
    QVERIFY(pages[2].contains(last.steps[0].text));
    QVERIFY(pages[2].contains(QStringLiteral("Fertig")));
    QVERIFY(pages[2].contains(QStringLiteral("1 von 1 Schritten erledigt")));
    for (qsizetype index = 0; index < pages.size(); ++index)
      QVERIFY(pages[index].contains(QStringLiteral("Seite %1 von 3").arg(index + 1)));
    QCOMPARE(bytes(database), before);
  }

  void bothPassesShareOneReadSnapshot() {
    QTemporaryDir data;
    SqliteProjectRepository repository(data.path());
    repository.initialize();
    ProjectDraft original{QStringLiteral("Stable"), {}, {}, QStringLiteral("Before")};
    original.photo = photo();
    original.color = QStringLiteral("#315A7D80");
    original.steps = {{QUuid::createUuid(), QStringLiteral("Saved step"), false, 9}};
    const auto saved = repository.create(original);
    const auto connection = QUuid::createUuid().toString();
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(connection); });
    auto other = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    other.setDatabaseName(data.filePath(QStringLiteral("restaurierungsheft.sqlite3")));
    QVERIFY(other.open());
    QSqlQuery query(other);
    QVERIFY(query.exec(QStringLiteral("PRAGMA journal_mode=WAL")));
    query.finish();
    repository.withSavedProjects([&](const ProjectSource& source) {
      TransferDraftList first;
      source([&](const ProjectDraft& project) { first.append(project); });
      QCOMPARE(first, TransferDraftList{original});
      QVERIFY(query.exec(QStringLiteral("UPDATE projects SET note='After',photo=NULL,color=''")));
      QVERIFY(query.exec(QStringLiteral("UPDATE checklist_steps SET completed=1")));
      QVERIFY(query.exec(QStringLiteral("INSERT INTO projects(name) VALUES('Added later')")));
      TransferDraftList second;
      source([&](const ProjectDraft& project) { second.append(project); });
      QCOMPARE(second, first);
    });
    QCOMPARE(repository.find(saved.id).note, QStringLiteral("After"));
    QCOMPARE(repository.findAll().size(), 2);
    QVERIFY_EXCEPTION_THROWN(repository.withSavedProjects([](const ProjectSource& source) {
      source([](const ProjectDraft&) { throw ExportError(QStringLiteral("Late render failure")); });
    }),
                             ExportError);
    repository.create({QStringLiteral("After failed export"), {}, {}, {}});
    QCOMPARE(repository.findAll().size(), 3);
  }

  void emptyAndLateRenderFailurePreserveTarget() {
    QTemporaryDir data, output;
    SqliteProjectRepository repository(data.path());
    repository.initialize();
    const auto path = output.filePath(QStringLiteral("existing.pdf"));
    write(path, "Existing PDF");
    auto target = inspectExportTarget(path, data.path());
    QVERIFY_EXCEPTION_THROWN(exportCollectionPdf(repository, target, true, data.path()),
                             ExportError);
    QCOMPARE(bytes(path), QByteArray("Existing PDF"));
    repository.create({QStringLiteral("Alpha"), {}, {}, {}});
    repository.create({QStringLiteral("Zulu"), {}, {}, QString::fromUcs4(U"🧵")});
    const auto failingRenderer = [](const ProjectSource& source, QIODevice& device) {
      int visited = 0;
      renderProjectsPdf(
          [&](const ProjectReceiver& receive) {
            source([&](const ProjectDraft& project) {
              if (++visited == 2)
                throw ExportError(QStringLiteral("Late renderer failure"));
              receive(project);
            });
          },
          device);
    };
    QVERIFY_EXCEPTION_THROWN(
        exportCollectionPdf(repository, target, true, data.path(), failingRenderer), ExportError);
    QCOMPARE(bytes(path), QByteArray("Existing PDF"));
    QCOMPARE(
        QDir(output.path()).entryList({QStringLiteral("*.tmp")}, QDir::Files | QDir::Hidden).size(),
        0);
    exportCollectionPdf(repository, target, true, data.path());
    const auto exported = textFrom(path);
    QVERIFY(exported.contains(QStringLiteral("Alpha")));
    QVERIFY(exported.contains(QStringLiteral("Zulu")));
    QVERIFY(bytes(path).contains("NotoEmoji-Regular"));
    QCOMPARE(repository.findAll().size(), 2);
  }

  void replacementCollisionAndPortableNewFile() {
    QTemporaryDir data, output;
    SqliteProjectRepository repository(data.path());
    repository.initialize();
    repository.create({QStringLiteral("Project"), {}, {}, {}});
    const auto path = output.filePath(QStringLiteral("all.pdf"));
    const auto denied = [](const QString&, const QString&) {
      return std::make_error_code(std::errc::operation_not_supported);
    };
    exportCollectionPdf(repository, inspectExportTarget(path, data.path()), false, data.path(),
                        renderProjectsPdf, denied);
    QVERIFY(bytes(path).startsWith("%PDF-"));
    const auto original = bytes(path);
    QVERIFY_EXCEPTION_THROWN(
        exportCollectionPdf(repository, inspectExportTarget(path, data.path()), false, data.path()),
        ExportError);
    QCOMPARE(bytes(path), original);
    auto target = inspectExportTarget(path, data.path());
    QVERIFY_EXCEPTION_THROWN(
        exportCollectionPdf(repository, target, true, data.path(),
                            [&](const ProjectSource& source, QIODevice& device) {
                              renderProjectsPdf(source, device);
                              write(path, "Concurrent change");
                            }),
        ExportError);
    QCOMPARE(bytes(path), QByteArray("Concurrent change"));
    exportCollectionPdf(repository, inspectExportTarget(path, data.path()), true, data.path());
    QVERIFY(textFrom(path).contains(QStringLiteral("Project")));
  }
};
QTEST_MAIN(CollectionPdfTest)
#include "collection_pdf_test.moc"
