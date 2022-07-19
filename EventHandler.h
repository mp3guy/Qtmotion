#pragma once

#include "Settings.h"
#include "TargetString.h"

#include <QObject>

class QPlainTextEdit;
class QKeyEvent;
class QPaintEvent;
template <typename T>
class QFutureInterface;

namespace Core {
class IEditor;
class FutureProgress;
} // namespace Core

namespace Qtmotion {
class EventHandler : public QObject {
  Q_OBJECT

 public:
  EventHandler();
  void updateCommand(const Settings& settings);

 public slots:
  void triggerBeforeChar();
  void triggerAfterChar();
  void triggerBeforeCharSelect();
  void triggerAfterCharSelect();
  void triggerCommand();

 private slots:
  void installEventFilter();

 private:
  void enqueueEventFilter();

  void trigger(const bool beforeChar, const bool selection);

  void reset();

  bool eventFilter(QObject* obj, QEvent* event) override;

  bool handleKeyPress(QKeyEvent* e);

  void handlePaintEvent(QPaintEvent*);

  static bool isModifierKey(int key);

  void moveToPosition(QPlainTextEdit* textEdit, int newPos);

  bool setEditor(Core::IEditor* e);

  void runCommand(QFutureInterface<void>& future);

  enum class State { Inactive, WaitingForInput };
  bool beforeChar_ = false;
  bool selection_ = false;

  Core::IEditor* currentEditor_ = nullptr;
  QPlainTextEdit* textEdit_ = nullptr;
  State state_ = State::Inactive;
  TargetString target_;
  std::vector<EventHandler*> handlers_;
  Settings commandSettings_;
  std::string filePath_;
};
} // namespace Qtmotion
