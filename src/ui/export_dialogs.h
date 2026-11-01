#pragma once
#include <QString>
class QWidget;
namespace restoration {
class ExportDialogs {
public:
  virtual ~ExportDialogs() = default;
  virtual QString choose(QWidget* parent, const QString& suggestedName);
  virtual bool confirmReplacement(QWidget* parent, const QString& path);
  void rememberExport(const QString& path);

private:
  QString lastDirectory_;
};
} // namespace restoration
