#include "pdf/pdf_renderer.h"
#include "application/photo_importer.h"
#include <QCoreApplication>
#include <QLocale>

#include <QFontDatabase>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <QRawFont>
#include <QTextLayout>

static void initializeFontResource() { Q_INIT_RESOURCE(export_fonts); }

namespace restoration {
namespace {
constexpr qreal left = 48;
constexpr qreal top = 64;
constexpr qreal bottom = 776;
constexpr qreal width = 499;

QString printable(QString text, const QRawFont& font) {
  text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  text.replace(QChar('\r'), QChar('\n'));
  text.replace(QChar(0x2028), QChar('\n'));
  text.replace(QChar(0x2029), QChar('\n'));
  text.replace(QChar('\t'), QStringLiteral("    "));
  for (qsizetype index = 0; index < text.size(); ++index) {
    const auto unit = text.at(index);
    if (unit.isHighSurrogate() && index + 1 < text.size() && text.at(index + 1).isLowSurrogate()) {
      ++index;
    } else if (unit.isSurrogate()) {
      throw ExportError(QCoreApplication::translate(
          "App",
          "Der Text enthält ein ungültiges Unicode-Zeichen. Bitte korrigiere es vor dem Export."));
    }
  }
  text = text.normalized(QString::NormalizationForm_C);
  for (const auto code : text.toUcs4()) {
    if (code == '\n')
      continue;
    const auto category = QChar::category(code);
    if (category == QChar::Other_Control || category == QChar::Other_Format ||
        category == QChar::Other_Surrogate || !font.supportsCharacter(code)) {
      throw ExportError(QCoreApplication::translate(
                            "App", "Das Zeichen U+%1 wird von der PDF-Schrift nicht unterstützt. "
                                   "Bitte ersetze es im Projekt und speichere erneut.")
                            .arg(static_cast<uint>(code), 4, 16, QChar('0')));
    }
  }
  return text;
}

class PagePainter {
public:
  PagePainter(QPdfWriter& writer, QPainter& painter) : writer_(writer), painter_(painter) {
    font_.setFamily(QStringLiteral("DejaVu Sans"));
    font_.setPixelSize(11);
    font_.setStyleStrategy(QFont::NoFontMerging);
    decorate();
  }

  void text(const QString& value, int size = 11, bool heading = false) {
    if (heading)
      reserve(42);
    QFont font = font_;
    font.setPixelSize(size);
    for (const auto& paragraph : value.split(QChar('\n'), Qt::KeepEmptyParts)) {
      if (paragraph.isEmpty()) {
        reserve(16);
        y_ += 16;
        continue;
      }
      QTextLayout layout(paragraph, font, &writer_);
      QTextOption option;
      option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
      layout.setTextOption(option);
      layout.beginLayout();
      QVector<QTextLine> lines;
      while (true) {
        auto line = layout.createLine();
        if (!line.isValid())
          break;
        line.setLineWidth(width);
        lines.append(line);
      }
      layout.endLayout();
      for (const auto& line : lines) {
        const auto height = std::max(qreal(16), line.height() + 3);
        reserve(height);
        painter_.setPen(QColor(QStringLiteral("#24352f")));
        line.draw(&painter_, QPointF(left, y_));
        y_ += height;
      }
    }
    if (heading)
      y_ += 6;
  }

  void photo(const QImage& image) {
    const auto size = image.size().scaled(QSize(static_cast<int>(width), 230), Qt::KeepAspectRatio);
    reserve(size.height() + 14);
    painter_.drawImage(QRectF(QPointF(left, y_), QSizeF(size)), image);
    y_ += size.height() + 14;
  }
  void gap() { y_ += 10; }

private:
  void reserve(qreal height) {
    if (y_ + height <= bottom)
      return;
    if (!writer_.newPage())
      throw ExportError(QCoreApplication::translate(
          "App", "Eine weitere PDF-Seite konnte nicht geschrieben werden."));
    ++page_;
    y_ = top;
    decorate();
  }
  void decorate() {
    painter_.setFont(font_);
    painter_.setPen(QColor(QStringLiteral("#53645a")));
    painter_.drawText(QPointF(left, 34), QStringLiteral("Restoration Journal"));
    painter_.drawLine(QPointF(left, 44), QPointF(left + width, 44));
    painter_.drawText(
        QPointF(left, 812),
        QCoreApplication::translate("App", "Seite %1").arg(QLocale().toString(page_)));
  }
  QPdfWriter& writer_;
  QPainter& painter_;
  QFont font_;
  int page_ = 1;
  qreal y_ = top;
};
} // namespace

void initializePdfFonts() {
  initializeFontResource();
  if (QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/DejaVuSans.ttf")) < 0)
    throw ExportError(QCoreApplication::translate(
        "App", "Die mitgelieferte PDF-Schrift konnte nicht geladen werden."));
}

void renderProjectPdf(const ProjectDraft& project, QIODevice& output) {
  const auto valid = validateDraft(project);
  QRawFont raw(QStringLiteral(":/assets/fonts/DejaVuSans.ttf"), 11);
  if (!raw.isValid())
    throw ExportError(QCoreApplication::translate("App", "Die mitgelieferte PDF-Schrift fehlt."));
  // Validate every field before writing. Never fall back to system fonts or replacement boxes.
  const auto name = printable(valid.name, raw);
  const auto material = printable(valid.material, raw);
  const auto location = printable(valid.location, raw);
  const auto note = printable(valid.note, raw);
  QStringList steps;
  for (const auto& step : valid.steps)
    steps.append(printable(QStringLiteral("%1  %2").arg(
                               step.completed ? QCoreApplication::translate("App", "☑ Erledigt")
                                              : QCoreApplication::translate("App", "☐ Offen"),
                               step.text),
                           raw));
  const auto image =
      valid.photo.isEmpty() ? QImage() : decodePhoto(valid.photo, maxPhotoSide).preview;
  QPdfWriter writer(&output);
  writer.setResolution(72);
  writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                                   QMarginsF(0, 0, 0, 0), QPageLayout::Point));
  writer.setTitle(name);
  writer.setCreator(QStringLiteral("Restoration Journal"));
  QPainter painter;
  if (!painter.begin(&writer))
    throw ExportError(
        QCoreApplication::translate("App", "Die PDF-Erzeugung konnte nicht gestartet werden."));
  PagePainter page(writer, painter);
  page.text(name, 19, true);
  page.text(QCoreApplication::translate("App", "Status: %1").arg(displayStatus(valid.status)));
  if (!material.isEmpty())
    page.text(QCoreApplication::translate("App", "Gegenstand / Material: %1").arg(material));
  if (!location.isEmpty())
    page.text(QCoreApplication::translate("App", "Aufbewahrungsort: %1").arg(location));
  page.gap();
  if (!image.isNull())
    page.photo(image);
  if (!note.isEmpty()) {
    page.text(QCoreApplication::translate("App", "Notiz"), 14, true);
    page.text(note);
    page.gap();
  }
  if (!steps.isEmpty()) {
    page.text(QCoreApplication::translate("App", "Arbeitsschritte"), 14, true);
    for (const auto& step : steps) {
      page.text(step);
      page.gap();
    }
  }
  if (!painter.end())
    throw ExportError(QCoreApplication::translate(
        "App", "Die PDF konnte nicht vollständig abgeschlossen werden."));
  // Writer destruction finalizes the device before returning to the export service.
}
} // namespace restoration
