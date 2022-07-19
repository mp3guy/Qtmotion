#include "OptPageMain.h"

#include "OptPageMainWidget.h"
#include "Settings.h"

namespace Qtmotion {

OptPageMain::OptPageMain(Settings* settings, QObject* parent)
    : IOptionsPage(parent), mSettings(settings) {
  setId("QtmotionSettings");
  setDisplayName("General");
  setCategory("Qtmotion");
  setDisplayCategory("Qtmotion");
  setCategoryIcon(Utils::Icon(":/imgs/motion.png"));
}

QWidget* OptPageMain::widget() {
  if (nullptr == mWidget) {
    mWidget = new OptPageMainWidget(mSettings);
  }

  return mWidget;
}

void OptPageMain::apply() {
  const Settings newSettings = mWidget->GenerateSettings();

  if (newSettings != *mSettings) {
    *mSettings = newSettings;
    mSettings->Save();
    emit SettingsChanged();
  }
}

void OptPageMain::finish() {
  delete mWidget;
  mWidget = nullptr;
}

} // namespace Qtmotion
