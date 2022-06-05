#include "Plugin.h"

#include <iostream>

#include <coreplugin/actionmanager/actionmanager.h>

#include <QAction>

#include "EventHandler.h"

namespace Qtmotion {
Plugin::Plugin() : handler_(std::make_unique<EventHandler>()) {}

bool Plugin::initialize(const QStringList&, QString*) {
  QAction* searchBeforeChar = new QAction(tr("Search before char"), this);
  constexpr std::string_view kSearchBeforeId = "Qtmotion.SearchBeforeChar";
  Core::Command* searchBeforeCmd = Core::ActionManager::registerAction(
      searchBeforeChar,
      std::string(kSearchBeforeId).c_str(),
      Core::Context(Core::Constants::C_EDIT_MODE));
  searchBeforeCmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+,")));
  connect(searchBeforeChar, SIGNAL(triggered()), handler_.get(), SLOT(triggerBeforeChar()));

  QAction* searchAfterChar = new QAction(tr("Search after char"), this);
  constexpr std::string_view kSearchAfterId = "Qtmotion.SearchAfterChar";
  Core::Command* searchAfterCmd = Core::ActionManager::registerAction(
      searchAfterChar,
      std::string(kSearchAfterId).c_str(),
      Core::Context(Core::Constants::C_EDIT_MODE));
  searchAfterCmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+.")));
  connect(searchAfterChar, SIGNAL(triggered()), handler_.get(), SLOT(triggerAfterChar()));

  return true;
}

void Plugin::extensionsInitialized() {}

ExtensionSystem::IPlugin::ShutdownFlag Plugin::aboutToShutdown() {
  return SynchronousShutdown;
}

} // namespace Qtmotion
