#pragma once

#include <memory>

#include <extensionsystem/iplugin.h>

namespace Qtmotion {
namespace Internal {
class QtmotionHandler;

class QtmotionPlugin : public ExtensionSystem::IPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Qtmotion.json")

 public:
  QtmotionPlugin();
  ~QtmotionPlugin() override;

  bool initialize(const QStringList& arguments, QString* errorString) override;
  void extensionsInitialized() override;
  ShutdownFlag aboutToShutdown() override;

 private:
  std::unique_ptr<QtmotionHandler> handler_;
};

} // namespace Internal
} // namespace Qtmotion
