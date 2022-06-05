#pragma once

#include "TargetString.h"

#include <QObject>

class QPlainTextEdit;
class QKeyEvent;
class QPaintEvent;

namespace Core {
class IEditor;
}

namespace Qtmotion {
class EventHandler : public QObject {
  Q_OBJECT

 public:
  EventHandler() = default;

 public slots:
  void triggerBeforeChar();
  void triggerAfterChar();

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

  enum class State { Inactive, WaitingForInput };
  bool beforeChar_ = false;
  bool selection_ = false;

  Core::IEditor* currentEditor_ = nullptr;
  QPlainTextEdit* textEdit_ = nullptr;
  State state_ = State::Inactive;
  TargetString target_;
};
} // namespace Qtmotion
