#include "i18n/languages.h"
#include "pdf/pdf_export.h"
#include "persistence/sqlite_project_repository.h"
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest>
#include <atomic>
#include <filesystem>
#include <thread>
#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <unistd.h>
#endif
using namespace restoration;
namespace {
void writeBytes(const QString& path, const QByteArray& bytes) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
    throw std::runtime_error("Fixture write failed");
}
QByteArray readBytes(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    throw std::runtime_error("Fixture read failed");
  return file.readAll();
}
QString run(const QString& program, const QStringList& args) {
  QProcess process;
  process.start(program, args);
  if (!process.waitForFinished(30000) || process.exitStatus() != QProcess::NormalExit ||
      process.exitCode() != 0)
    throw std::runtime_error(
        (program + QString::fromUtf8(process.readAllStandardError())).toStdString());
  return QString::fromUtf8(process.readAllStandardOutput());
}
QString compact(QString value) {
  return value.normalized(QString::NormalizationForm_KC)
      .remove(QRegularExpression(QStringLiteral("\\s")));
}
QByteArray picture(const char* type = "PNG") {
  QImage image(640, 320, QImage::Format_RGB32);
  image.fill(QColor(QStringLiteral("#a25b27")));
  QByteArray data;
  QBuffer buffer(&data);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, type);
  return data;
}
QString artifact(const QString& name) {
  QDir().mkpath(QStringLiteral("verification/pdf"));
  return QDir::current().absoluteFilePath(QStringLiteral("verification/pdf/") + name);
}
void makePdf(const QString& path, const ProjectDraft& draft) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    throw std::runtime_error("PDF fixture open failed");
  renderProjectPdf(draft, file);
}
} // namespace

class PdfTest final : public QObject {
  Q_OBJECT
private slots:
  void newPdfWithoutHardLinksAndLateCollision() {
    QTemporaryDir data, output;
    SqliteProjectRepository repository(data.path());
    repository.initialize();
    ProjectDraft draft;
    draft.name = QStringLiteral("Portable PDF");
    const auto saved = repository.create(draft);
    const auto path = output.filePath(QStringLiteral("portable.pdf"));
    const auto denied = [](const QString&, const QString&) {
      return std::make_error_code(std::errc::operation_not_supported);
    };
    exportProjectPdf(repository, saved.id, inspectExportTarget(path, data.path()), false,
                     data.path(), renderProjectPdf, denied);
    QVERIFY(readBytes(path).startsWith("%PDF-"));
    const auto original = readBytes(path);
    QVERIFY_EXCEPTION_THROWN(exportProjectPdf(repository, saved.id,
                                              inspectExportTarget(path, data.path()), false,
                                              data.path(), renderProjectPdf, denied),
                             ExportError);
    QCOMPARE(readBytes(path), original);
    const auto raced = output.filePath(QStringLiteral("raced.pdf"));
    const auto collision = [&raced](const QString&, const QString&) {
      writeBytes(raced, "Another writer");
      return std::make_error_code(std::errc::operation_not_supported);
    };
    QVERIFY_EXCEPTION_THROWN(exportProjectPdf(repository, saved.id,
                                              inspectExportTarget(raced, data.path()), false,
                                              data.path(), renderProjectPdf, collision),
                             ExportError);
    QCOMPARE(readBytes(raced), QByteArray("Another writer"));
  }

  void initTestCase() { initializePdfFonts(); }
  void localizedContents_data() {
    QTest::addColumn<QString>("language");
    for (const auto& language : supportedLanguages())
      QTest::newRow(qPrintable(language)) << language;
  }
  void localizedContents() {
    QFETCH(QString, language);
    Translations translations;
    translations.install(language);
    const auto restore = qScopeGuard([] { QLocale::setDefault(QLocale("de-DE")); });
    const auto tr = [](const char* source) { return QCoreApplication::translate("App", source); };
    ProjectDraft draft{QStringLiteral("Étoile – ação"),
                       QStringLiteral("Eiche & Öl"),
                       QStringLiteral("Werkstatt 2"),
                       QStringLiteral("Grüße, café, coração\n"),
                       ProjectStatus::Started,
                       picture(),
                       {{QUuid::createUuid(), QStringLiteral("Kanten prüfen – ação"), true, 0},
                        {QUuid::createUuid(), QStringLiteral("Kanten prüfen – ação"), false, 2}}};
    draft.color = QStringLiteral("#315A7D80");
    draft.note += QStringLiteral("Lange Notiz. ").repeated(180);
    const auto path = artifact(QStringLiteral("localized-") + language + QStringLiteral(".pdf"));
    makePdf(path, draft);
    const auto text = run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
    for (const auto& expected :
         {draft.name, draft.material, draft.location,
          tr("Status: %1").arg(displayStatus(draft.status)), draft.color, tr("Notiz"),
          tr("Arbeitsschritte"), tr("☑ Erledigt"), tr("☐ Offen"),
          tr("Seite %1 von %2").arg(QLocale().toString(1), QLocale().toString(2))})
      QVERIFY2(compact(text).contains(compact(expected)), qPrintable(expected));
    QCOMPARE(text.count(QStringLiteral("Kanten prüfen – ação")), 2);
    QVERIFY(run(QStringLiteral("pdfimages"), {QStringLiteral("-list"), path})
                .contains(QStringLiteral("image")));
    const auto fonts = run(QStringLiteral("pdffonts"), {path});
    QVERIFY(fonts.contains(QStringLiteral("DejaVuSans")));
    const auto prefix = artifact(QStringLiteral("localized-") + language);
    run(QStringLiteral("pdftoppm"), {QStringLiteral("-f"), QStringLiteral("1"),
                                     QStringLiteral("-singlefile"), QStringLiteral("-scale-to"),
                                     QStringLiteral("1200"), QStringLiteral("-png"), path, prefix});
    QVERIFY(!QImage(prefix + QStringLiteral(".png")).isNull());
  }
  void minimal() {
    const auto path = artifact(QStringLiteral("minimal.pdf"));
    makePdf(path, {QStringLiteral("Schrank"), {}, {}, {}});
    const auto text = run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
    QVERIFY(text.contains(QStringLiteral("Schrank")));
    QVERIFY(text.contains(QStringLiteral("Geplant")));
    QVERIFY(text.contains(QStringLiteral("Keine Arbeitsschritte hinterlegt.")));
    QVERIFY(run(QStringLiteral("pdfinfo"), {path})
                .contains(QRegularExpression(QStringLiteral("Pages:\\s+1"))));
    const auto fonts = run(QStringLiteral("pdffonts"), {path});
    QVERIFY(fonts.contains(QStringLiteral("DejaVuSans")));
    QVERIFY(fonts.contains(QRegularExpression(QStringLiteral("yes\\s+(yes|no)\\s+yes"))));
  }
  void fullContents_data() {
    QTest::addColumn<ProjectStatus>("status");
    QTest::addColumn<QByteArray>("format");
    for (const auto status :
         {ProjectStatus::Planned, ProjectStatus::Started, ProjectStatus::Finished})
      for (const auto* format : {"PNG", "JPEG"})
        QTest::newRow((storedStatus(status).toUtf8() + format).constData())
            << status << QByteArray(format);
  }
  void fullContents() {
    QFETCH(ProjectStatus, status);
    QFETCH(QByteArray, format);
    ProjectDraft draft{
        QStringLiteral("Ömer – großes Erbstück € 42,50"),
        QStringLiteral("Eiche & Öl"),
        QStringLiteral("Werkstatt <oben>"),
        QStringLiteral("ÄÖÜ äöü ß O\u0308mer\n\n<img src=\"file:///private/secret\">\n"
                       "https://example.invalid/ '); DROP TABLE projects"),
        status,
        picture(format.constData()),
        {{QUuid::createUuid(), QStringLiteral("Oberfläche reinigen"), true, 0},
         {QUuid::createUuid(), QStringLiteral("Oberfläche versiegeln"), false, 2}}};
    const auto path =
        artifact(storedStatus(status) + QString::fromLatin1(format) + QStringLiteral(".pdf"));
    makePdf(path, draft);
    const auto text = compact(run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")}));
    for (const auto& field :
         {draft.name, draft.material, draft.location, draft.note, displayStatus(status),
          QStringLiteral("☑ Erledigt"), QStringLiteral("☐ Offen")})
      QVERIFY2(text.contains(compact(field)), qPrintable(field));
    QVERIFY(text.indexOf(compact(draft.steps[0].text)) <
            text.indexOf(compact(draft.steps[1].text)));
    const auto images = run(QStringLiteral("pdfimages"), {QStringLiteral("-list"), path});
    QVERIFY(images.contains(QRegularExpression(QStringLiteral("image\\s+640\\s+320"))));
    run(QStringLiteral("pdftoppm"),
        {QStringLiteral("-scale-to"), QStringLiteral("1100"), QStringLiteral("-png"),
         QStringLiteral("-singlefile"), path, path + QStringLiteral("-page")});
    const QImage page(path + QStringLiteral("-page.png"));
    QVERIFY(!page.isNull());
    int minX = page.width(), maxX = 0, minY = page.height(), maxY = 0;
    for (int y = 0; y < page.height(); ++y) {
      for (int x = 0; x < page.width(); ++x) {
        const auto color = page.pixelColor(x, y);
        if (std::abs(color.red() - 162) < 10 && std::abs(color.green() - 91) < 10 &&
            std::abs(color.blue() - 39) < 10) {
          minX = std::min(x, minX);
          maxX = std::max(x, maxX);
          minY = std::min(y, minY);
          maxY = std::max(y, maxY);
        }
      }
    }
    QVERIFY(maxX > minX && maxY > minY);
    QVERIFY(std::abs(qreal(maxX - minX) / (maxY - minY) - 2.0) < 0.03);
  }
  void paginationAndLongStep() {
    ProjectDraft draft{QStringLiteral("Langes Restaurierungsprojekt"), {}, {}, {}};
    for (int index = 0; index < 180; ++index)
      draft.note += QStringLiteral("Notizmarker%1 – Ömer erhält das Erbstück.\n\n")
                        .arg(index, 3, 10, QChar('0'));
    draft.note += QStringLiteral("WORTSTART") + QString(3500, QChar('W')) +
                  QStringLiteral("WORTENDE\nLetzteNotizzeile");
    QString longStep;
    for (int index = 0; index < 180; ++index)
      longStep +=
          QStringLiteral("Schrittteil%1 reinigen und versiegeln. ").arg(index, 3, 10, QChar('0'));
    draft.steps = {{QUuid::createUuid(), longStep, false, 0},
                   {QUuid::createUuid(), QStringLiteral("AllerletzteSchlusszeile"), true, 4}};
    const auto path = artifact(QStringLiteral("long.pdf"));
    makePdf(path, draft);
    const auto text = compact(run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")}));
    QVERIFY(text.contains(QStringLiteral("AllerletzteSchlusszeile")));
    QVERIFY(text.contains(QStringLiteral("LetzteNotizzeile")));
    // Repeated page furniture may separate a word spanning pages.
    auto content = text;
    content.remove(compact(QStringLiteral("Restoration Journal")));
    content.remove(compact(QStringLiteral("· Projektübersicht")));
    content.remove(QRegularExpression(QStringLiteral("Seite\\d+von\\d+")));
    QVERIFY(content.contains(QStringLiteral("WORTSTART") + QString(3500, QChar('W')) +
                             QStringLiteral("WORTENDE")));
    for (int index = 0; index < 180; ++index) {
      QVERIFY(content.contains(QStringLiteral("Notizmarker%1").arg(index, 3, 10, QChar('0'))));
      QVERIFY(content.contains(QStringLiteral("Schrittteil%1").arg(index, 3, 10, QChar('0'))));
    }
    QXmlStreamReader xml(
        run(QStringLiteral("pdftotext"), {QStringLiteral("-bbox"), path, QStringLiteral("-")}));
    int pages = 0;
    while (!xml.atEnd()) {
      xml.readNext();
      if (xml.isStartElement() && xml.name() == QStringLiteral("page"))
        ++pages;
      if (xml.isStartElement() && xml.name() == QStringLiteral("word")) {
        const auto a = xml.attributes();
        QVERIFY(a.value(QStringLiteral("xMin")).toDouble() >= 47);
        QVERIFY(a.value(QStringLiteral("xMax")).toDouble() <= 548);
        const auto y = a.value(QStringLiteral("yMax")).toDouble();
        QVERIFY(y <= 821);
      }
    }
    QVERIFY(!xml.hasError());
    QVERIFY(pages > 10);
    run(QStringLiteral("pdftoppm"),
        {QStringLiteral("-scale-to"), QStringLiteral("1100"), QStringLiteral("-png"), path,
         artifact(QStringLiteral("long"))});
  }
  void storedEmojiFieldsAndSequences() {
    QTemporaryDir data, destination;
    SqliteProjectRepository repository(data.path());
    repository.initialize();
    const auto emoji = QString::fromUcs4(
        U"🧵 🪵 🛠️ 👩🏽‍🔧 🇩🇪 👨‍👩‍👧‍👦 1️⃣");
    ProjectDraft draft{QStringLiteral("Name ") + emoji, QStringLiteral("Material ") + emoji,
                       QStringLiteral("Location ") + emoji, QStringLiteral("Note ") + emoji};
    draft.steps = {{QUuid::createUuid(), QStringLiteral("Step ") + emoji, true, 0}};
    const auto saved = repository.create(draft);
    const auto before = readBytes(data.filePath(QStringLiteral("restaurierungsheft.sqlite3")));
    const auto path = destination.filePath(QStringLiteral("emoji.pdf"));
    exportProjectPdf(repository, saved.id, inspectExportTarget(path, data.path()), false,
                     data.path());
    writeBytes(artifact(QStringLiteral("emoji.pdf")), readBytes(path));
    run(QStringLiteral("pdftoppm"),
        {QStringLiteral("-f"), QStringLiteral("1"), QStringLiteral("-singlefile"),
         QStringLiteral("-scale-to"), QStringLiteral("1600"), QStringLiteral("-png"), path,
         artifact(QStringLiteral("emoji"))});
    const auto text = run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
    // Qt 6.8's PDF Unicode map covers BMP characters only. Verify ordinary text,
    // embedded emoji outlines and actual rendering rather than promising emoji copy/paste.
    for (const auto& field : {"Name", "Material", "Location", "Note", "Step"})
      QVERIFY2(text.contains(QString::fromLatin1(field)), qPrintable(text));
    const auto fonts = run(QStringLiteral("pdffonts"), {path});
    QVERIFY2(fonts.contains(QStringLiteral("NotoEmoji")), qPrintable(fonts));
    QCOMPARE(readBytes(data.filePath(QStringLiteral("restaurierungsheft.sqlite3"))), before);
    QCOMPARE(repository.find(saved.id), draft);
    const QImage rendered(artifact(QStringLiteral("emoji.png")));
    QVERIFY(!rendered.isNull());
    ProjectDraft plain{QStringLiteral("Name "), QStringLiteral("Material "),
                       QStringLiteral("Location "), QStringLiteral("Note ")};
    plain.steps = {{QUuid::createUuid(), QStringLiteral("Step "), true, 0}};
    const auto plainPath = destination.filePath(QStringLiteral("plain.pdf"));
    makePdf(plainPath, plain);
    const auto plainImage = destination.filePath(QStringLiteral("plain"));
    run(QStringLiteral("pdftoppm"),
        {QStringLiteral("-f"), QStringLiteral("1"), QStringLiteral("-singlefile"),
         QStringLiteral("-scale-to"), QStringLiteral("1600"), QStringLiteral("-png"), plainPath,
         plainImage});
    const QImage withoutEmoji(plainImage + QStringLiteral(".png"));
    QCOMPARE(rendered.size(), withoutEmoji.size());
    // Every field must gain painted glyphs, including the bold title and the checklist.
    for (const auto& band :
         {QPair{80, 115}, QPair{165, 200}, QPair{213, 248}, QPair{295, 330}, QPair{402, 437}}) {
      int changed = 0;
      const qreal scale = static_cast<qreal>(rendered.height()) / 842;
      for (int y = qRound(band.first * scale); y < qRound(band.second * scale); ++y)
        for (int x = qRound(48 * scale); x < qRound(547 * scale); ++x)
          if (rendered.pixel(x, y) != withoutEmoji.pixel(x, y))
            ++changed;
      QVERIFY2(changed > 100, "Emoji outlines are missing from a rendered field");
    }
  }
  void missingGlyphsUseExplicitNotation() {
    const auto path = artifact(QStringLiteral("missing-glyphs.pdf"));
    const auto note = QStringLiteral("Before ") + QChar(0x0001) + QChar(0x200b) +
                      QString::fromUcs4(U"\U0010ffff") + QStringLiteral(" after");
    makePdf(path, {QStringLiteral("Project"), {}, {}, note});
    const auto text = run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
    QVERIFY(text.contains(QStringLiteral("Before [U+0001][U+200B][U+10FFFF] after")));
  }
  void invalidUnicodeStillFailsBeforeWriting() {
    QBuffer output;
    output.open(QIODevice::WriteOnly);
    QVERIFY_EXCEPTION_THROWN(
        renderProjectPdf({QStringLiteral("Projekt"), {}, {}, QString(QChar(0xd800))}, output),
        ExportError);
    QVERIFY(output.data().isEmpty());
  }
  void safeNames_data() {
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("expected");
    QTest::newRow("unicode") << QStringLiteral(" O\u0308mers Erbstück ")
                             << QStringLiteral("Ömers Erbstück.pdf");
    QTest::newRow("reserved") << QStringLiteral("CON.foo") << QStringLiteral("_CON.foo.pdf");
    QTest::newRow("superscript") << QStringLiteral("LPT¹") << QStringLiteral("_LPT¹.pdf");
    QTest::newRow("empty") << QStringLiteral(" ... ")
                           << QStringLiteral("Restaurierungsprojekt.pdf");
    QTest::newRow("extension") << QStringLiteral("Projekt.PDF") << QStringLiteral("Projekt.pdf");
    QTest::newRow("illegal") << QStringLiteral("a/b:c*") << QStringLiteral("a_b_c_.pdf");
  }
  void safeNames() {
    QFETCH(QString, name);
    QFETCH(QString, expected);
    QCOMPARE(suggestedPdfName(name), expected);
    QVERIFY(suggestedPdfName(QString(1000, QChar(0x00d6))).toUtf8().size() <= 164);
  }
  void savedIdPhotoAndUnchangedDatabase() {
    QTemporaryDir directory, destination;
    SqliteProjectRepository repository(directory.path());
    repository.initialize();
    const auto source = destination.filePath(QStringLiteral("source.jpg"));
    writeBytes(source, picture("JPEG"));
    const ProjectDraft first{
        QStringLiteral("Gleich"),
        {},
        {},
        QStringLiteral("Richtige Notiz"),
        ProjectStatus::Finished,
        readBytes(source),
        {{QUuid::createUuid(), QStringLiteral("Richtiger Schritt"), false, 0}}};
    const auto saved = repository.create(first);
    repository.create({first.name, {}, {}, QStringLiteral("Falsche Notiz")});
    QVERIFY(QFile::remove(source));
    const auto dbPath = directory.filePath(QStringLiteral("restaurierungsheft.sqlite3"));
    const auto before = readBytes(dbPath);
    const auto path = destination.filePath(QStringLiteral("Ömer.pdf"));
    exportProjectPdf(repository, saved.id, inspectExportTarget(path, directory.path()), false,
                     directory.path());
    const auto text = run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")});
    QVERIFY(text.contains(QStringLiteral("Richtige Notiz")));
    QVERIFY(!text.contains(QStringLiteral("Falsche Notiz")));
    QVERIFY(run(QStringLiteral("pdfimages"), {QStringLiteral("-list"), path})
                .contains(QStringLiteral("640")));
    QCOMPARE(repository.find(saved.id), first);
    QCOMPARE(readBytes(dbPath), before);
    QCOMPARE(QDir(destination.path())
                 .entryList({QStringLiteral("*.tmp")}, QDir::Files | QDir::Hidden)
                 .size(),
             0);
  }
  void safeReplacementAndFailures_data() {
    QTest::addColumn<int>("failure");
    for (int i = 0; i < 8; ++i)
      QTest::newRow(qPrintable(QString::number(i))) << i;
  }
  void safeReplacementAndFailures() {
    QFETCH(int, failure);
    QTemporaryDir directory, destination;
    SqliteProjectRepository repository(directory.path());
    repository.initialize();
    const auto saved = repository.create({QStringLiteral("Projekt"), {}, {}, {}});
    const auto path = destination.filePath(QStringLiteral("target.pdf"));
    const auto dbPath = directory.filePath(QStringLiteral("restaurierungsheft.sqlite3"));
    const auto databaseBefore = readBytes(dbPath);
    if (failure != 2)
      writeBytes(path, QByteArray("old pdf bytes"));
    const auto target = inspectExportTarget(path, directory.path());
    bool invoked = false;
    PdfRenderer renderer = [&](const ProjectDraft& snapshot, QIODevice& output) {
      invoked = true;
      QCOMPARE(snapshot.name, QStringLiteral("Projekt"));
      QCOMPARE(QFileInfo::exists(path), failure != 2);
      if (failure != 2)
        QCOMPARE(readBytes(path), QByteArray("old pdf bytes"));
      output.write("partial");
      if (failure == 1)
        throw ExportError(QStringLiteral("Später Rendererfehler"));
      if (failure == 2 || failure == 3)
        writeBytes(path, QByteArray("concurrent"));
      if (failure == 4) {
        QFile::remove(path);
        writeBytes(path, QByteArray("old pdf bytes"));
      }
      if (failure == 5) {
        QFile::setPermissions(destination.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner);
      }
      if (failure == 6)
        QFile::setPermissions(path, QFileDevice::ReadOwner);
      if (failure == 7)
        throw ExportError(QStringLiteral("Abschlussfehler"));
    };
    QVERIFY_EXCEPTION_THROWN(
        exportProjectPdf(repository, saved.id, target, failure != 0, directory.path(), renderer),
        ExportError);
    QCOMPARE(invoked, failure != 0);
    QFile::setPermissions(destination.path(),
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QCOMPARE(readBytes(path),
             failure == 2 || failure == 3 ? QByteArray("concurrent") : QByteArray("old pdf bytes"));
    QCOMPARE(readBytes(dbPath), databaseBefore);
    QCOMPARE(QDir(destination.path())
                 .entryList({QStringLiteral("*.tmp")}, QDir::Files | QDir::Hidden)
                 .size(),
             0);
    exportProjectPdf(repository, saved.id, inspectExportTarget(path, directory.path()), true,
                     directory.path());
    QVERIFY(run(QStringLiteral("pdftotext"), {path, QStringLiteral("-")})
                .contains(QStringLiteral("Projekt")));
    QCOMPARE(readBytes(dbPath), databaseBefore);
  }
  void invalidPathsAndAliases() {
    QTemporaryDir directory, destination;
    SqliteProjectRepository repository(directory.path());
    repository.initialize();
    for (const auto& path : {QString(), QStringLiteral("relative.pdf"),
                             destination.filePath(QStringLiteral("wrong.txt")),
                             destination.filePath(QStringLiteral("missing/target.pdf")),
                             directory.filePath(QStringLiteral("own.pdf"))})
      QVERIFY_EXCEPTION_THROWN(inspectExportTarget(path, directory.path()), ExportError);
    const auto folder = destination.filePath(QStringLiteral("folder.pdf"));
    QDir().mkdir(folder);
    QVERIFY_EXCEPTION_THROWN(inspectExportTarget(folder, directory.path()), ExportError);
#ifdef Q_OS_UNIX
    const auto alias = destination.filePath(QStringLiteral("alias.pdf"));
    QVERIFY(QFile::link(directory.filePath(QStringLiteral("restaurierungsheft.sqlite3")), alias));
    QVERIFY_EXCEPTION_THROWN(inspectExportTarget(alias, directory.path()), ExportError);
    QVERIFY(QFile::remove(alias));
    std::filesystem::create_hard_link(
        directory.filePath(QStringLiteral("restaurierungsheft.sqlite3")).toStdString(),
        alias.toStdString());
    QVERIFY_EXCEPTION_THROWN(inspectExportTarget(alias, directory.path()), ExportError);
#endif
  }
  void snapshotUnderConcurrentTransactions() {
    QTemporaryDir directory;
    SqliteProjectRepository repository(directory.path());
    repository.initialize();
    const auto photoA = picture();
    const auto photoB = picture("JPEG");
    const auto saved = repository.create({QStringLiteral("Projekt"),
                                          {},
                                          {},
                                          QStringLiteral("A"),
                                          ProjectStatus::Planned,
                                          photoA,
                                          {{QUuid::createUuid(), QStringLiteral("A"), false, 0}}});
    const auto dbPath = directory.filePath(QStringLiteral("restaurierungsheft.sqlite3"));
    {
      auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("wal-setup"));
      db.setDatabaseName(dbPath);
      QVERIFY(db.open());
      QSqlQuery query(db);
      QVERIFY(query.exec(QStringLiteral("PRAGMA journal_mode=WAL")));
    }
    QSqlDatabase::removeDatabase(QStringLiteral("wal-setup"));
    std::atomic<bool> failed = false;
    std::atomic<int> writes = 0;
    std::jthread writer([&] {
      {
        auto db =
            QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("snapshot-writer"));
        db.setDatabaseName(dbPath);
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=3000"));
        if (!db.open()) {
          failed = true;
          return;
        }
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("PRAGMA foreign_keys=ON"))) {
          failed = true;
          return;
        }
        for (int index = 0; index < 100; ++index) {
          const bool second = index % 2 == 0;
          const auto marker = second ? QStringLiteral("B") : QStringLiteral("A");
          if (!db.transaction()) {
            failed = true;
            break;
          }
          query.prepare(QStringLiteral("UPDATE projects SET note=?,status=?,photo=? WHERE id=?"));
          query.addBindValue(marker);
          query.addBindValue(second ? QStringLiteral("finished") : QStringLiteral("planned"));
          query.addBindValue(second ? photoB : photoA);
          query.addBindValue(saved.id);
          if (!query.exec()) {
            failed = true;
            break;
          }
          QThread::usleep(200);
          query.prepare(
              QStringLiteral("UPDATE checklist_steps SET text=?,completed=? WHERE project_id=?"));
          query.addBindValue(marker);
          query.addBindValue(second ? 1 : 0);
          query.addBindValue(saved.id);
          if (!query.exec() || !db.commit()) {
            failed = true;
            break;
          }
          ++writes;
        }
      }
      QSqlDatabase::removeDatabase(QStringLiteral("snapshot-writer"));
    });
    for (int index = 0; index < 150; ++index) {
      const auto snapshot = repository.find(saved.id);
      const bool second = snapshot.note == QStringLiteral("B");
      QCOMPARE(snapshot.steps.size(), 1);
      QCOMPARE(snapshot.steps.first().text, snapshot.note);
      QCOMPARE(snapshot.steps.first().completed, second);
      QCOMPARE(snapshot.status, second ? ProjectStatus::Finished : ProjectStatus::Planned);
      QCOMPARE(snapshot.photo, second ? photoB : photoA);
      QThread::usleep(100);
    }
    writer.join();
    QVERIFY(!failed);
    QCOMPARE(writes.load(), 100);
  }
  void qSaveFileDoesNotProvideNoClobber() {
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("race.pdf"));
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("export");
    writeBytes(path, QByteArray("new unapproved target"));
    QVERIFY(file.commit());
    QCOMPARE(readBytes(path), QByteArray("export"));
    // This measured behavior is why our service uses a hardlink for new targets.
  }
  void actualWriteFailure() {
#ifdef Q_OS_LINUX
    QTemporaryDir directory, destination;
    SqliteProjectRepository repository(directory.path());
    repository.initialize();
    const auto saved = repository.create({QStringLiteral("Projekt"), {}, {}, {}});
    const auto path = destination.filePath(QStringLiteral("old.pdf"));
    writeBytes(path, QByteArray("untouched"));
    const auto target = inspectExportTarget(path, directory.path());
    QVERIFY_EXCEPTION_THROWN(exportProjectPdf(repository, saved.id, target, true, directory.path(),
                                              [](const ProjectDraft& draft, QIODevice& output) {
                                                auto* file = qobject_cast<QFile*>(&output);
                                                if (!file)
                                                  throw std::runtime_error("Expected file");
                                                const int full = ::open("/dev/full", O_WRONLY);
                                                if (full < 0)
                                                  throw std::runtime_error("Expected /dev/full");
                                                const int result = ::dup2(full, file->handle());
                                                ::close(full);
                                                if (result < 0)
                                                  throw std::runtime_error("dup2 failed");
                                                renderProjectPdf(draft, output);
                                              }),
                             ExportError);
    QCOMPARE(readBytes(path), QByteArray("untouched"));
    QCOMPARE(QDir(destination.path())
                 .entryList({QStringLiteral("*.tmp")}, QDir::Files | QDir::Hidden)
                 .size(),
             0);
#else
    QSKIP("Der reale /dev/full-Schreibfehler ist Linux-spezifisch.");
#endif
  }
};
QTEST_MAIN(PdfTest)
#include "pdf_test.moc"
