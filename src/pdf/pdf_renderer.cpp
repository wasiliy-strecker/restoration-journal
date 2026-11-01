#include "pdf/pdf_renderer.h"
#include "application/colors.h"
#include "application/photo_importer.h"
#include <QCoreApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <QRawFont>
#include <QTextBoundaryFinder>
#include <QTextLayout>
#include <algorithm>

static void initializeFontResource() { Q_INIT_RESOURCE(export_fonts); }

namespace restoration {
namespace {
constexpr qreal left = 48;
constexpr qreal top = 80;
constexpr qreal bottom = 790;
constexpr qreal width = 499;
const QColor ink(QStringLiteral("#302E35"));
const QColor muted(QStringLiteral("#69626A"));
const QColor accent(QStringLiteral("#74536F"));
const QColor soft(QStringLiteral("#F1EAF0"));
const QColor paper(QStringLiteral("#F8F5F1"));
const QColor border(QStringLiteral("#DDD4DB"));
const QColor green(QStringLiteral("#356A52"));
const QColor greenSoft(QStringLiteral("#EDF5EF"));
const QColor amber(QStringLiteral("#906018"));
const QColor amberSoft(QStringLiteral("#FBF2DF"));

bool emojiCluster(const QString& text, const QRawFont& regular, const QRawFont& emoji) {
  bool needsEmoji = false;
  bool hasVisibleCharacter = false;
  for (const auto code : text.toUcs4()) {
    // Selectors, joiners and subdivision flag tags belong to the surrounding emoji.
    if (code == 0xfe0f || code == 0x200d || (code >= 0xe0020 && code <= 0xe007f)) {
      needsEmoji = true;
      continue;
    }
    if (code == 0xfe0e)
      continue;
    if (!emoji.supportsCharacter(code))
      return false;
    hasVisibleCharacter = true;
    needsEmoji =
        needsEmoji || code >= 0x1f000 || code == 0x20e3 || !regular.supportsCharacter(code);
  }
  return hasVisibleCharacter && needsEmoji;
}

QString printable(QString text, const QRawFont& font, const QRawFont& emoji) {
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
  QString result;
  QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, text);
  qsizetype start = 0;
  for (auto end = boundaries.toNextBoundary(); end != -1; end = boundaries.toNextBoundary()) {
    const auto cluster = text.mid(start, end - start);
    start = end;
    if (emojiCluster(cluster, font, emoji)) {
      result += cluster;
      continue;
    }
    for (const auto code : cluster.toUcs4()) {
      const auto category = QChar::category(code);
      if (code == '\n' || (category != QChar::Other_Control && category != QChar::Other_Format &&
                           font.supportsCharacter(code))) {
        const auto scalar = static_cast<char32_t>(code);
        result += QString::fromUcs4(&scalar, 1);
      } else {
        // Keep an explicit, lossless notation for characters outside the bundled fonts.
        // This changes only the PDF representation, never the stored project.
        result +=
            QStringLiteral("[U+%1]").arg(static_cast<uint>(code), 4, 16, QChar('0')).toUpper();
      }
    }
  }
  return result;
}

// The first pass measures pages without painting. The second replays the same saved snapshot,
// allowing page-of-total footers without retaining every photo or page in memory.
class PagePainter {
public:
  PagePainter(QPdfWriter& writer, QPainter* painter = nullptr, int totalPages = 0)
      : writer_(writer), painter_(painter), totalPages_(totalPages),
        regular_(QStringLiteral(":/assets/fonts/DejaVuSans.ttf"), 11),
        bold_(QStringLiteral(":/assets/fonts/DejaVuSans-Bold.ttf"), 11),
        emoji_(QStringLiteral(":/assets/fonts/NotoEmoji.ttf"), 11) {
    if (!regular_.isValid() || !bold_.isValid() || !emoji_.isValid())
      throw ExportError(QCoreApplication::translate("App", "Die mitgelieferte PDF-Schrift fehlt."));
  }

  int pages() const { return page_; }
  int projects() const { return projects_; }
  QString firstTitle() const { return firstTitle_; }

  void project(const ProjectDraft& saved) {
    const auto value = validateDraft(saved);
    if (projects_++ == 0)
      firstTitle_ = value.name;
    newPage();
    text(value.name, 24, 31, true, accent);
    y_ += 9;
    status(value.status);
    y_ += 16;
    const auto image =
        value.photo.isEmpty() ? QImage() : decodePhoto(value.photo, maxPhotoSide).preview;
    if (!overview(value, image)) {
      if (!image.isNull())
        photo(image);
      field(QCoreApplication::translate("App", "Gegenstand / Material"), value.material);
      field(QCoreApplication::translate("App", "Farbe"), value.color, parseColor(value.color));
      field(QCoreApplication::translate("App", "Aufbewahrungsort"), value.location);
    }
    if (!value.note.isEmpty()) {
      heading(QCoreApplication::translate("App", "Notiz"), 58);
      card(value.note, paper, ink);
    }
    heading(QCoreApplication::translate("App", "Arbeitsschritte"), 80);
    if (value.steps.isEmpty()) {
      card(QCoreApplication::translate("App", "Keine Arbeitsschritte hinterlegt."), soft, muted);
      return;
    }
    const auto completed = std::count_if(value.steps.cbegin(), value.steps.cend(),
                                         [](const ChecklistStep& step) { return step.completed; });
    text(QCoreApplication::translate("App", "%1 von %2 Schritten erledigt")
             .arg(QLocale().toString(completed), QLocale().toString(value.steps.size())),
         9, 15, false, muted);
    reserve(15);
    rect(QRectF(left, y_, width, 4), soft);
    if (completed > 0)
      rect(QRectF(left, y_,
                  width * static_cast<qreal>(completed) / static_cast<qreal>(value.steps.size()),
                  4),
           green);
    y_ += 15;
    for (const auto& step : value.steps) {
      card(QStringLiteral("%1: %2").arg(step.completed
                                            ? QCoreApplication::translate("App", "☑ Erledigt")
                                            : QCoreApplication::translate("App", "☐ Offen"),
                                        step.text),
           step.completed ? greenSoft : paper, step.completed ? green : ink);
      y_ += 5;
    }
  }

private:
  QFont font(int size, bool strong) const {
    QFont result(QStringLiteral("DejaVu Sans"));
    result.setPixelSize(size);
    result.setBold(strong);
    result.setStyleStrategy(QFont::NoFontMerging);
    return result;
  }

  QString checked(const QString& value, bool strong) const {
    return printable(value, strong ? bold_ : regular_, emoji_);
  }

  void format(QTextLayout& layout, int size, bool strong) const {
    QList<QTextLayout::FormatRange> ranges;
    const auto value = layout.text();
    QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, value);
    qsizetype start = 0;
    for (auto end = boundaries.toNextBoundary(); end != -1; end = boundaries.toNextBoundary()) {
      if (emojiCluster(value.mid(start, end - start), strong ? bold_ : regular_, emoji_)) {
        QFont emojiFont(QStringLiteral("Noto Emoji"));
        emojiFont.setPixelSize(size);
        emojiFont.setWeight(QFont::Normal);
        emojiFont.setStyleStrategy(QFont::NoFontMerging);
        QTextCharFormat character;
        character.setFont(emojiFont);
        ranges.append({static_cast<int>(start), static_cast<int>(end - start), character});
      }
      start = end;
    }
    layout.setFormats(ranges);
  }

  qreal measure(const QString& value, int size, bool strong) const {
    QTextLayout layout(checked(value, strong), font(size, strong), &writer_);
    format(layout, size, strong);
    layout.beginLayout();
    const auto line = layout.createLine();
    layout.endLayout();
    return line.isValid() ? line.naturalTextWidth() : 0;
  }

  QStringList wrap(const QString& raw, qreal available, int size, bool strong) const {
    QStringList result;
    for (const auto& paragraph : checked(raw, strong).split(QChar('\n'), Qt::KeepEmptyParts)) {
      if (paragraph.isEmpty()) {
        result.append(QString());
        continue;
      }
      QTextLayout layout(paragraph, font(size, strong), &writer_);
      format(layout, size, strong);
      QTextOption option;
      option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
      layout.setTextOption(option);
      layout.beginLayout();
      while (true) {
        auto line = layout.createLine();
        if (!line.isValid())
          break;
        line.setLineWidth(available);
        result.append(paragraph.mid(line.textStart(), line.textLength()));
      }
      layout.endLayout();
    }
    return result;
  }

  void draw(const QString& raw, const QPointF& baseline, int size, bool strong,
            const QColor& color) {
    const auto value = checked(raw, strong);
    if (!painter_)
      return;
    painter_->setPen(color);
    QTextLayout layout(value, font(size, strong), &writer_);
    format(layout, size, strong);
    layout.beginLayout();
    const auto line = layout.createLine();
    layout.endLayout();
    if (line.isValid())
      line.draw(painter_, baseline - QPointF(0, line.ascent()));
  }

  void rect(const QRectF& area, const QColor& fill) {
    if (painter_)
      painter_->fillRect(area, fill);
  }

  void box(const QRectF& area, const QColor& fill) {
    if (!painter_)
      return;
    painter_->setPen(Qt::NoPen);
    painter_->setBrush(fill);
    painter_->drawRoundedRect(area, 6, 6);
  }

  void newPage() {
    if (page_ > 0 && painter_ && !writer_.newPage())
      throw ExportError(QCoreApplication::translate(
          "App", "Eine weitere PDF-Seite konnte nicht geschrieben werden."));
    ++page_;
    y_ = top;
    rect(QRectF(0, 0, 595, 8), accent);
    draw(QCoreApplication::translate("App", "Restoration Journal · Projektübersicht"),
         QPointF(left, 40), 10, true, accent);
    rect(QRectF(left, 55, width, 1), border);
    rect(QRectF(left, 804, width, 1), border);
    draw(QStringLiteral("Restoration Journal"), QPointF(left, 818), 8, true, accent);
    const auto footer = QCoreApplication::translate("App", "Seite %1 von %2")
                            .arg(QLocale().toString(page_), QLocale().toString(totalPages_));
    draw(footer, QPointF(left + width - measure(footer, 8, false), 818), 8, false, muted);
  }

  void reserve(qreal height) {
    if (y_ + height > bottom)
      newPage();
  }

  void text(const QString& value, int size, qreal leading, bool strong, const QColor& color) {
    for (const auto& line : wrap(value, width, size, strong)) {
      reserve(leading);
      draw(line, QPointF(left, y_ + size), size, strong, color);
      y_ += leading;
    }
  }

  void heading(const QString& title, qreal minimum) {
    reserve(minimum);
    y_ += 12;
    draw(title, QPointF(left, y_ + 14), 14, true, accent);
    y_ += 26;
  }

  void status(ProjectStatus value) {
    const auto label = QCoreApplication::translate("App", "Status: %1").arg(displayStatus(value));
    const auto fill = value == ProjectStatus::Finished  ? greenSoft
                      : value == ProjectStatus::Started ? amberSoft
                                                        : soft;
    const auto color = value == ProjectStatus::Finished  ? green
                       : value == ProjectStatus::Started ? amber
                                                         : accent;
    reserve(30);
    box(QRectF(left, y_, std::min(width, measure(label, 10, true) + 26), 28), fill);
    draw(label, QPointF(left + 13, y_ + 18), 10, true, color);
    y_ += 28;
  }

  void swatch(const QColor& color, const QPointF& position) {
    // Match Costume Planner's 4 × 3 transparency checkerboard and 28 × 21 point swatch.
    for (int row = 0; row < 3; ++row) {
      for (int column = 0; column < 4; ++column) {
        const int base = (row + column) % 2 == 0 ? 255 : 220;
        const qreal alpha = color.alphaF();
        const QColor blended(qRound(color.red() * alpha + base * (1 - alpha)),
                             qRound(color.green() * alpha + base * (1 - alpha)),
                             qRound(color.blue() * alpha + base * (1 - alpha)));
        rect(QRectF(position.x() + column * 7, position.y() + row * 7, 7, 7), blended);
      }
    }
    if (painter_) {
      painter_->setPen(QPen(muted, 0.5));
      painter_->setBrush(Qt::NoBrush);
      painter_->drawRect(QRectF(position, QSizeF(28, 21)));
    }
  }

  void field(const QString& label, const QString& value, const QColor& color = {}) {
    if (value.isEmpty())
      return;
    constexpr qreal labelWidth = 138;
    const qreal valueX = left + labelWidth + 14 + (color.isValid() ? 36 : 0);
    const auto labels = wrap(label, labelWidth - 24, 9, true);
    const auto lines = wrap(value, left + width - valueX - 14, 11, false);
    const auto length = std::max(labels.size(), lines.size());
    qsizetype offset = 0;
    while (offset < length) {
      reserve(45);
      const auto capacity = std::max<qsizetype>(1, static_cast<qsizetype>((bottom - y_ - 24) / 17));
      const auto count = std::min(capacity, length - offset);
      const auto height = std::max(qreal(42), static_cast<qreal>(count) * 17 + 24);
      box(QRectF(left, y_, width, height), paper);
      rect(QRectF(left, y_, 3, height), soft);
      for (qsizetype index = 0; index < count; ++index) {
        const auto baseline = y_ + 24 + static_cast<qreal>(index) * 17;
        if (offset + index < labels.size())
          draw(labels[offset + index], QPointF(left + 14, baseline), 9, true, muted);
        if (offset + index < lines.size())
          draw(lines[offset + index], QPointF(valueX, baseline), 11, false, ink);
      }
      if (color.isValid() && offset == 0)
        swatch(color, QPointF(left + labelWidth + 14, y_ + 10));
      y_ += height + 6;
      offset += count;
    }
  }

  void card(const QString& value, const QColor& fill, const QColor& color) {
    const auto lines = wrap(value, width - 28, 11, false);
    qsizetype offset = 0;
    while (offset < lines.size()) {
      reserve(41);
      const auto capacity = std::max<qsizetype>(1, static_cast<qsizetype>((bottom - y_ - 24) / 17));
      const auto count = std::min(capacity, lines.size() - offset);
      const auto height = static_cast<qreal>(count) * 17 + 24;
      box(QRectF(left, y_, width, height), fill);
      for (qsizetype index = 0; index < count; ++index)
        draw(lines[offset + index], QPointF(left + 14, y_ + 24 + static_cast<qreal>(index) * 17),
             11, false, color);
      y_ += height;
      offset += count;
    }
  }

  void photo(const QImage& image) {
    const auto size = QSizeF(image.size()).scaled(QSizeF(width - 20, 175), Qt::KeepAspectRatio);
    reserve(size.height() + 32);
    box(QRectF(left, y_, width, size.height() + 20), paper);
    if (painter_)
      painter_->drawImage(QRectF(QPointF(left + (width - size.width()) / 2, y_ + 10), size), image);
    y_ += size.height() + 32;
  }

  struct Detail {
    QString label;
    QStringList lines;
    QColor color;
  };

  bool overview(const ProjectDraft& value, const QImage& image) {
    if (image.isNull())
      return false;
    constexpr qreal half = (width - 14) / 2;
    QVector<Detail> details;
    qreal required = 16;
    const QVector<QPair<QString, QString>> values = {
        {QCoreApplication::translate("App", "Gegenstand / Material"), value.material},
        {QCoreApplication::translate("App", "Farbe"), value.color},
        {QCoreApplication::translate("App", "Aufbewahrungsort"), value.location}};
    for (qsizetype index = 0; index < values.size(); ++index) {
      const auto& [label, content] = values[index];
      if (content.isEmpty())
        continue;
      const auto color = index == 1 ? parseColor(content) : QColor();
      const auto lines = wrap(content, half - 28 - (color.isValid() ? 36 : 0), 11, false);
      if (measure(label, 9, true) > half - 28)
        return false;
      details.append({label, lines, color});
      required += 34 + static_cast<qreal>(lines.size()) * 17;
    }
    if (details.isEmpty() || required > 235)
      return false;
    const auto height = std::max(qreal(195), required);
    reserve(height + 12);
    box(QRectF(left, y_, half, height), paper);
    auto detailY = y_ + 16;
    for (const auto& detail : details) {
      draw(detail.label, QPointF(left + 14, detailY + 9), 9, true, muted);
      const qreal x = left + 14 + (detail.color.isValid() ? 36 : 0);
      for (qsizetype index = 0; index < detail.lines.size(); ++index)
        draw(detail.lines[index], QPointF(x, detailY + 33 + static_cast<qreal>(index) * 17), 11,
             false, ink);
      if (detail.color.isValid())
        swatch(detail.color, QPointF(left + 14, detailY + 18));
      detailY += 34 + static_cast<qreal>(detail.lines.size()) * 17;
    }
    const auto photoX = left + half + 14;
    box(QRectF(photoX, y_, half, height), paper);
    const auto size =
        QSizeF(image.size()).scaled(QSizeF(half - 16, height - 16), Qt::KeepAspectRatio);
    if (painter_)
      painter_->drawImage(
          QRectF(QPointF(photoX + (half - size.width()) / 2, y_ + (height - size.height()) / 2),
                 size),
          image);
    y_ += height + 12;
    return true;
  }

  QPdfWriter& writer_;
  QPainter* painter_;
  int totalPages_;
  QRawFont regular_;
  QRawFont bold_;
  QRawFont emoji_;
  int page_ = 0;
  int projects_ = 0;
  QString firstTitle_;
  qreal y_ = top;
};
} // namespace

void initializePdfFonts() {
  initializeFontResource();
  for (const auto* path : {":/assets/fonts/DejaVuSans.ttf", ":/assets/fonts/DejaVuSans-Bold.ttf",
                           ":/assets/fonts/NotoEmoji.ttf"}) {
    if (QFontDatabase::addApplicationFont(QString::fromLatin1(path)) < 0)
      throw ExportError(QCoreApplication::translate(
          "App", "Die mitgelieferte PDF-Schrift konnte nicht geladen werden."));
  }
}

void renderProjectsPdf(const ProjectSource& source, QIODevice& output) {
  QPdfWriter writer(&output);
  writer.setResolution(72);
  writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                                   QMarginsF(0, 0, 0, 0), QPageLayout::Point));
  PagePainter measure(writer);
  source([&measure](const ProjectDraft& project) { measure.project(project); });
  if (measure.projects() == 0)
    throw ExportError(QCoreApplication::translate("App", "Die Sammlung enthält keine Projekte."));
  writer.setTitle(measure.projects() == 1
                      ? measure.firstTitle()
                      : QCoreApplication::translate("App", "Restaurierungsprojekte"));
  writer.setCreator(QStringLiteral("Restoration Journal"));
  QPainter painter;
  if (!painter.begin(&writer))
    throw ExportError(
        QCoreApplication::translate("App", "Die PDF-Erzeugung konnte nicht gestartet werden."));
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  PagePainter page(writer, &painter, measure.pages());
  source([&page](const ProjectDraft& project) { page.project(project); });
  if (!painter.end())
    throw ExportError(QCoreApplication::translate(
        "App", "Die PDF konnte nicht vollständig abgeschlossen werden."));
}

void renderProjectPdf(const ProjectDraft& project, QIODevice& output) {
  renderProjectsPdf([&project](const ProjectReceiver& receive) { receive(project); }, output);
}
} // namespace restoration
