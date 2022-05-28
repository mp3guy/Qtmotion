#include "Plugin.h"

#include <iostream>

#include <coreplugin/actionmanager/actionmanager.h>

#include <QAction>

#include "EventHandler.h"

namespace Qtmotion {
Plugin::Plugin() : handler_(std::make_unique<EventHandler>()) {}

Plugin::~Plugin() {}

bool Plugin::initialize(const QStringList&, QString*) {
  QAction* searchEntireScreen = new QAction(tr("Search entire screen"), this);

  constexpr std::string_view kSearchScreenId = "Qtmotion.SearchScreen";

  Core::Command* searchScreenCmd = Core::ActionManager::registerAction(
      searchEntireScreen,
      std::string(kSearchScreenId).c_str(),
      Core::Context(Core::Constants::C_EDIT_MODE));

  searchScreenCmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+;")));

  connect(searchEntireScreen, SIGNAL(triggered()), handler_.get(), SLOT(triggerKeyPressed()));

  return true;
}

void Plugin::extensionsInitialized() {}

ExtensionSystem::IPlugin::ShutdownFlag Plugin::aboutToShutdown() {
  return SynchronousShutdown;
}

} // namespace Qtmotion
