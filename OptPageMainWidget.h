#pragma once

#include <QWidget>

class QCheckBox;
class QLineEdit;

namespace Qtmotion {

class Settings;

class OptPageMainWidget : public QWidget {
  Q_OBJECT

 public:
  explicit OptPageMainWidget(const Settings* settings);

  Settings GenerateSettings() const;

 private:
  QCheckBox* passFilename_ = nullptr;
  QLineEdit* command_ = nullptr;
};

} // namespace Qtmotion
