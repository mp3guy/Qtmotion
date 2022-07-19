#pragma once

#include <memory>

#include <extensionsystem/iplugin.h>

#include "Settings.h"

namespace Qtmotion {
class EventHandler;
class OptPageMain;

class Plugin : public ExtensionSystem::IPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Qtmotion.json")

 public:
  Plugin();

  bool initialize(const QStringList& arguments, QString* errorString) override;
  void extensionsInitialized() override;
  ShutdownFlag aboutToShutdown() override;

 private slots:
  void updateCommand();

 private:
  std::unique_ptr<EventHandler> handler_;
  std::unique_ptr<OptPageMain> optionsPage_ = nullptr;
  std::unique_ptr<Settings> settings_ = nullptr;
};
} // namespace Qtmotion
