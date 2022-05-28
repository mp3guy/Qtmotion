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
  EventHandler();
  ~EventHandler();

 public slots:
  void triggerKeyPressed();

 private slots:
  void installEventFilter();

 private:
  void enqueueEventFilter();

  void reset();

  bool isVisualMode() const;

  bool eventFilter(QObject* obj, QEvent* event);

  bool handleKeyPress(QKeyEvent* e);

  bool handlePaintEvent(QPaintEvent*);

  static bool isModifierKey(int key);
  static void moveToPosition(QPlainTextEdit* editor, int newPos, bool visualMode);

  bool setEditor(Core::IEditor* e);

  enum class State { Inactive, BeforeFirstCharacter, WaitingForSelectionOrMoreCharacters };

  Core::IEditor* currentEditor_ = nullptr;
  QPlainTextEdit* textEdit_ = nullptr;
  State state_ = State::Inactive;
  TargetString target_;
};
} // namespace Qtmotion
