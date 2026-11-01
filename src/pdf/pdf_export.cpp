#include "pdf/pdf_export.h"
#include <QCoreApplication>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <exception>
#include <filesystem>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace restoration {
namespace {
[[noreturn]] void fileFailure(const QString& detail) {
  throw ExportError(QCoreApplication::translate(
                        "App", "Die PDF konnte nicht sicher gespeichert werden. "
                               "Prüfe Ordner, Zugriffsrechte, freien Speicher und eine mögliche "
                               "Dateisperre. Du kannst ein anderes Ziel wählen.\n%1")
                        .arg(detail));
}
std::filesystem::path nativePath(const QString& path) {
#ifdef Q_OS_WIN
  return std::filesystem::path(path.toStdWString());
#else
  return std::filesystem::path(QFile::encodeName(path).constData());
#endif
}
QByteArray identity(const QString& path) {
#ifdef Q_OS_WIN
  const HANDLE handle =
      CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
    fileFailure(QCoreApplication::translate("App", "Die Dateiidentität ist nicht lesbar."));
  BY_HANDLE_FILE_INFORMATION info{};
  const bool ok = GetFileInformationByHandle(handle, &info) != 0;
  CloseHandle(handle);
  if (!ok || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    fileFailure(QCoreApplication::translate(
        "App", "Dateiverweise oder unbekannte Dateiidentität sind nicht zulässig."));
  return QByteArray::number(static_cast<qulonglong>(info.dwVolumeSerialNumber)) + ':' +
         QByteArray::number(static_cast<qulonglong>(info.nFileIndexHigh)) + ':' +
         QByteArray::number(static_cast<qulonglong>(info.nFileIndexLow));
#else
  struct stat info{};
  if (::lstat(QFile::encodeName(path).constData(), &info) != 0 || S_ISLNK(info.st_mode))
    fileFailure(QCoreApplication::translate(
        "App", "Dateiverweise oder unbekannte Dateiidentität sind nicht zulässig."));
  return QByteArray::number(static_cast<qulonglong>(info.st_dev)) + ':' +
         QByteArray::number(static_cast<qulonglong>(info.st_ino));
#endif
}
QByteArray revision(const QString& path) {
#ifdef Q_OS_WIN
  const QFileInfo info(path);
  return QByteArray::number(info.size()) + ':' +
         QByteArray::number(info.lastModified().toMSecsSinceEpoch()) + ':' +
         QByteArray::number(info.birthTime().toMSecsSinceEpoch());
#else
  struct stat info{};
  if (::lstat(QFile::encodeName(path).constData(), &info) != 0)
    fileFailure(QCoreApplication::translate("App", "Der Zielzustand ist nicht mehr lesbar."));
#ifdef Q_OS_MACOS
  const auto modified = info.st_mtimespec;
  const auto changed = info.st_ctimespec;
#else
  const auto modified = info.st_mtim;
  const auto changed = info.st_ctim;
#endif
  return QByteArray::number(static_cast<qlonglong>(info.st_size)) + ':' +
         QByteArray::number(static_cast<qlonglong>(modified.tv_sec)) + ':' +
         QByteArray::number(static_cast<qlonglong>(modified.tv_nsec)) + ':' +
         QByteArray::number(static_cast<qlonglong>(changed.tv_sec)) + ':' +
         QByteArray::number(static_cast<qlonglong>(changed.tv_nsec));
#endif
}
} // namespace

QString suggestedPdfName(const QString& name) {
  QString result = name.normalized(QString::NormalizationForm_C).trimmed();
  result.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x{0000}-\\x{001f}\\x{007f}]")),
                 QStringLiteral("_"));
  result.remove(QRegularExpression(QStringLiteral("^[. ]+|[. ]+$")));
  if (result.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
    result.chop(4);
  while (result.toUtf8().size() > 160) {
    if (result.back().isLowSurrogate())
      result.chop(2);
    else
      result.chop(1);
  }
  result.remove(QRegularExpression(QStringLiteral("[. ]+$")));
  if (result.isEmpty())
    result = QCoreApplication::translate("App", "Restaurierungsprojekt");
  if (QRegularExpression(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9¹²³]|LPT[1-9¹²³])(?:\\.|$)"),
                         QRegularExpression::CaseInsensitiveOption)
          .match(result)
          .hasMatch())
    result.prepend(QChar('_'));
  return result + QStringLiteral(".pdf");
}

// Both paths are explicitly named at the small UI/service boundary.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ExportTarget inspectExportTarget(const QString& path, const QString& dataDirectory) {
  const QFileInfo file(path);
  if (path.isEmpty() || !file.isAbsolute() ||
      file.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) != 0)
    fileFailure(
        QCoreApplication::translate("App", "Wähle einen absoluten Dateipfad mit der Endung .pdf."));
  const auto directory = file.dir().canonicalPath();
  if (directory.isEmpty() || !QFileInfo(directory).isWritable())
    fileFailure(
        QCoreApplication::translate("App", "Der Zielordner fehlt oder ist nicht beschreibbar."));
  const auto protectedDirectory = QFileInfo(dataDirectory).canonicalFilePath();
  if (!protectedDirectory.isEmpty() &&
      (directory == protectedDirectory || directory.startsWith(protectedDirectory + QChar('/'))))
    fileFailure(QCoreApplication::translate(
        "App", "Wähle einen Exportordner außerhalb des App-Datenverzeichnisses."));
  // Freeze the resolved parent. A later parent replacement is detected before publication.
  ExportTarget target;
  target.path = QDir(directory).filePath(file.fileName());
  target.directoryIdentity = identity(directory);
  const QFileInfo resolved(target.path);
  if (resolved.isSymLink())
    fileFailure(
        QCoreApplication::translate("App", "Symbolische Dateiverweise sind kein Exportziel."));
  target.exists = resolved.exists();
  if (!target.exists)
    return target;
  if (!resolved.isFile() || !resolved.isReadable() || !resolved.isWritable())
    fileFailure(QCoreApplication::translate(
        "App", "Das Ziel muss eine lesbare und beschreibbare normale PDF-Datei sein."));
  target.identity = identity(target.path);
  target.revision = revision(target.path);
  // Also reject aliases of our own database and lock, even with a .pdf name elsewhere.
  for (const auto& name :
       {QStringLiteral("restaurierungsheft.sqlite3"), QStringLiteral("collection.lock"),
        QStringLiteral("language.txt"), QStringLiteral("restaurierungsheft.sqlite3-journal"),
        QStringLiteral("restaurierungsheft.sqlite3-wal"),
        QStringLiteral("restaurierungsheft.sqlite3-shm")}) {
    const auto protectedFile = QDir(dataDirectory).filePath(name);
    if (QFileInfo::exists(protectedFile) && identity(protectedFile) == target.identity)
      fileFailure(QCoreApplication::translate(
          "App", "App-Daten und Sperrdateien dürfen nicht ersetzt werden."));
  }
  QFile input(target.path);
  if (!input.open(QIODevice::ReadOnly))
    fileFailure(input.errorString());
  QCryptographicHash hash(QCryptographicHash::Sha256);
  if (!hash.addData(&input))
    fileFailure(input.errorString());
  target.digest = hash.result();
  if (identity(target.path) != target.identity || revision(target.path) != target.revision)
    fileFailure(
        QCoreApplication::translate("App", "Das Ziel hat sich während der Prüfung geändert."));
  return target;
}

namespace {
void exportPdf(const ExportTarget& target, bool replaceApproved, const QString& dataDirectory,
               const std::function<void(QIODevice&)>& render, const PdfLinkAttempt& link) {
  if (target.exists && !replaceApproved)
    throw ExportError(QCoreApplication::translate(
        "App", "Das Ersetzen der vorhandenen PDF wurde nicht bestätigt."));
  if (inspectExportTarget(target.path, dataDirectory) != target)
    throw ExportError(QCoreApplication::translate(
        "App", "Das PDF-Ziel hat sich geändert. Bitte wähle es erneut."));
  QTemporaryFile temporary(
      QFileInfo(target.path).dir().filePath(QStringLiteral(".restaurierungsheft-XXXXXX.tmp")));
  if (!temporary.open())
    fileFailure(temporary.errorString());
  render(temporary);
  if (temporary.error() != QFileDevice::NoError || !temporary.flush())
    fileFailure(temporary.errorString());
  temporary.close();
  if (temporary.error() != QFileDevice::NoError)
    fileFailure(temporary.errorString());
  if (inspectExportTarget(target.path, dataDirectory) != target)
    throw ExportError(QCoreApplication::translate(
        "App", "Das PDF-Ziel wurde inzwischen angelegt oder verändert. "
               "Keine Datei wurde ersetzt. Bitte wähle das Ziel erneut."));
  std::error_code error;
  if (!target.exists) {
    if (link)
      error = link(temporary.fileName(), target.path);
    else
      std::filesystem::create_hard_link(nativePath(temporary.fileName()), nativePath(target.path),
                                        error);
    if (error == std::errc::file_exists)
      fileFailure(QString::fromStdString(error.message()));
    if (error) {
      QFile source(temporary.fileName());
      if (!source.open(QIODevice::ReadOnly))
        fileFailure(source.errorString());
      QFile output(target.path);
      if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        fileFailure(output.errorString());
      QByteArray createdIdentity;
      try {
        createdIdentity = identity(target.path);
        while (!source.atEnd()) {
          const auto chunk = source.read(64LL * 1024);
          if (source.error() != QFileDevice::NoError)
            fileFailure(source.errorString());
          if (output.write(chunk) != chunk.size())
            fileFailure(output.errorString());
        }
        if (!output.flush())
          fileFailure(output.errorString());
        output.close();
        if (output.error() != QFileDevice::NoError)
          fileFailure(output.errorString());
      } catch (...) {
        const auto failure = std::current_exception();
        output.close();
        try {
          if (!createdIdentity.isEmpty() && identity(target.path) == createdIdentity)
            QFile::remove(target.path);
        } catch (const std::exception&) {
          std::rethrow_exception(failure);
        }
        std::rethrow_exception(failure);
      }
    }
  } else {
#ifdef Q_OS_WIN
    if (!MoveFileExW(reinterpret_cast<LPCWSTR>(temporary.fileName().utf16()),
                     reinterpret_cast<LPCWSTR>(target.path.utf16()), MOVEFILE_REPLACE_EXISTING))
      fileFailure(QCoreApplication::translate("App", "Windows-Dateiersatz fehlgeschlagen (%1).")
                      .arg(GetLastError()));
#else
    std::filesystem::rename(nativePath(temporary.fileName()), nativePath(target.path), error);
    if (error)
      fileFailure(QString::fromStdString(error.message()));
#endif
  }
  // QTemporaryFile removes only its own temporary name on all exit paths.
}
} // namespace

void exportProjectPdf(ProjectRepository& repository, qint64 id, const ExportTarget& target,
                      bool replaceApproved, const QString& dataDirectory,
                      const PdfRenderer& renderer, const PdfLinkAttempt& link) {
  exportPdf(
      target, replaceApproved, dataDirectory,
      [&](QIODevice& output) { renderer(repository.find(id), output); }, link);
}

void exportCollectionPdf(ProjectRepository& repository, const ExportTarget& target,
                         bool replaceApproved, const QString& dataDirectory,
                         const CollectionPdfRenderer& renderer, const PdfLinkAttempt& link) {
  exportPdf(
      target, replaceApproved, dataDirectory,
      [&](QIODevice& output) {
        repository.withSavedProjects(
            [&](const ProjectSource& source) { renderer(source, output); });
      },
      link);
}
} // namespace restoration
