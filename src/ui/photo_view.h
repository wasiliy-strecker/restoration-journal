#pragma once
#include <QImage>
#include <QLabel>
namespace restoration {
class PhotoView final : public QLabel {
  Q_OBJECT
public:
  explicit PhotoView(QWidget* parent = nullptr);
  void showPhoto(const QImage& image);
  void showMessage(const QString& message);

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  void render();
  QImage image_;
};
} // namespace restoration
