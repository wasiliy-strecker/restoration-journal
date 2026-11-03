#pragma once
#include <QDialog>

namespace restoration {
enum class InformationPage : quint8 { About, Licenses };
class InformationDialog final : public QDialog {
public:
  InformationDialog(InformationPage page, const QString& dataDirectory, QWidget* parent);
};
} // namespace restoration
