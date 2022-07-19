#include "Settings.h"

#include <coreplugin/icore.h>

namespace Qtmotion {

Settings::Settings() {}

void Settings::Load() {
  QSettings* s = Core::ICore::settings();
  s->beginGroup("QT_MOTION");
  passFilename_ = s->value("PASS_FILENAME", false).toBool();
  command_ = s->value("COMMAND", QString()).toString().toStdString();
  s->endGroup();
}

void Settings::Save() {
  QSettings* s = Core::ICore::settings();
  s->beginGroup("QT_MOTION");
  s->setValue("PASS_FILENAME", passFilename_);
  s->setValue("COMMAND", QString::fromStdString(command_));
  s->endGroup();
}

} // namespace Qtmotion
