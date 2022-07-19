#include "OptPageMainWidget.h"

#include "Settings.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace Qtmotion {

OptPageMainWidget::OptPageMainWidget(const Settings* settings) {
  QVBoxLayout* layout = new QVBoxLayout(this);

  QHBoxLayout* commandRow = new QHBoxLayout;
  layout->addLayout(commandRow);
  QLabel* label = new QLabel("Command:");
  commandRow->addWidget(label);
  command_ = new QLineEdit(QString::fromStdString(settings->command()));
  commandRow->addWidget(command_);

  QHBoxLayout* checkBoxRow = new QHBoxLayout;
  layout->addLayout(checkBoxRow);
  passFilename_ = new QCheckBox("Pass the current open file folder as an argument to the command");
  passFilename_->setChecked(settings->passFilename());
  checkBoxRow->addWidget(passFilename_);

  layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
}

Settings OptPageMainWidget::GenerateSettings() const {
  Settings settings;
  settings.passFilename(passFilename_->isChecked());
  settings.command(command_->text().toStdString());
  return settings;
}

} // namespace Qtmotion
