#include "EventHandler.h"

#include <iostream>

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditor.h>

#include <QPainter>
#include <QPlainTextEdit>
#include <QTextBlock>

namespace Qtmotion {
EventHandler::EventHandler() {}

EventHandler::~EventHandler() {}

void EventHandler::triggerKeyPressed() {
  reset();

  currentEditor_ = Core::EditorManager::currentEditor();

  if (setEditor(currentEditor_)) {
    state_ = State::BeforeFirstCharacter;
    enqueueEventFilter();
    std::cout << "Initiated" << std::endl;
  } else {
    currentEditor_ = nullptr;
  }
}

void EventHandler::installEventFilter() {
  if (textEdit_) {
    textEdit_->installEventFilter(this);
    textEdit_->viewport()->installEventFilter(this);
  }
}

void EventHandler::enqueueEventFilter() {
  // Postpone installEventFilter() so plugin gets next key event first.
  QMetaObject::invokeMethod(this, "installEventFilter", Qt::QueuedConnection);
}

void EventHandler::reset() {
  if (setEditor(currentEditor_)) {
    QWidget* viewport = textEdit_->viewport();
    textEdit_->removeEventFilter(this);
    viewport->removeEventFilter(this);
    textEdit_ = nullptr;
  }
  target_.clear();
  state_ = State::Inactive;
  currentEditor_ = nullptr;
}

bool EventHandler::isVisualMode() const {
  return textEdit_ && textEdit_->textCursor().hasSelection();
}

bool EventHandler::eventFilter(QObject* obj, QEvent* event) {
  QWidget* currentViewport = qobject_cast<QWidget*>(obj);

  if (currentViewport != nullptr && event->type() == QEvent::Paint) {
    // Handle the painter event last to prevent the area painted from being overwritten
    currentViewport->removeEventFilter(this);
    QCoreApplication::sendEvent(currentViewport, event);
    currentViewport->installEventFilter(this);
    QPaintEvent* paintEvent = static_cast<QPaintEvent*>(event);
    handlePaintEvent(paintEvent);
    return true;
  } else if (event->type() == QEvent::KeyPress && textEdit_) {
    enqueueEventFilter();

    QKeyEvent* e = static_cast<QKeyEvent*>(event);
    bool keyPressHandled = handleKeyPress(e);
    return keyPressHandled;
  } else if (event->type() == QEvent::ShortcutOverride) {
    enqueueEventFilter();

    // Handle ESC key press.
    QKeyEvent* e = static_cast<QKeyEvent*>(event);
    if (e->key() == Qt::Key_Escape) {
      return handleKeyPress(e);
    }
  }

  return false;
}

void EventHandler::moveToPosition(QPlainTextEdit* editor, int newPos, bool visualMode) {
  QTextBlock targetBlock = editor->document()->findBlock(newPos);

  if (!targetBlock.isValid()) {
    targetBlock = editor->document()->lastBlock();
  }

  bool overwriteMode = editor->overwriteMode();
  TextEditor::TextEditorWidget* baseEditor = qobject_cast<TextEditor::TextEditorWidget*>(editor);
  bool visualBlockMode = baseEditor && baseEditor->multiTextCursor().hasMultipleCursors();

  bool selectNextCharacter = (overwriteMode || visualMode) && !visualBlockMode;
  bool keepSelection = visualMode || visualBlockMode;

  QTextCursor textCursor = editor->textCursor();
  textCursor.setPosition(
      selectNextCharacter ? newPos - 1 : newPos,
      keepSelection ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);

  if (baseEditor) {
    baseEditor->setTextCursor(textCursor);
  } else {
    editor->setTextCursor(textCursor);
  }

  if (visualBlockMode) {
    baseEditor->setTextCursor(baseEditor->textCursor());
  }
}

bool EventHandler::handleKeyPress(QKeyEvent* e) {
  if (e->key() == Qt::Key_Escape) {
    // Exit the process
    std::cout << "Cancelling" << std::endl;

    State tmpState = state_;
    if (tmpState == State::WaitingForSelectionOrMoreCharacters) {
      textEdit_->viewport()->update();
    }

    reset();

    return true;
  } else if (state_ == State::BeforeFirstCharacter && !isModifierKey(e->key())) {
    std::cout << "Just triggered with " << e->text().toStdString() << std::endl;

    QChar target(e->key());
    target = target.toLower();

    if (e->modifiers() == Qt::ShiftModifier) {
      target = target.toUpper();
    }

    if (textEdit_) {
      target_.findMatchingPositions(textEdit_, target);
    } else {
      std::cout << "textEdit_ is nullptr" << std::endl;
    }

    if (!target_.isEmpty()) {
      state_ = State::WaitingForSelectionOrMoreCharacters;
      textEdit_->viewport()->update();
    }

    return true;
  } else if (state_ == State::WaitingForSelectionOrMoreCharacters && !isModifierKey(e->key())) {
    if (e->key() == Qt::Key_Return) {
      std::cout << "Cycling group" << std::endl;

      if (e->modifiers() == Qt::ShiftModifier) {
        // Shift + Enter makes show previous
        // group of target positions
        target_.previousGroup();
      } else {
        // Enter makes show next
        // group of target positions
        target_.nextGroup();
      }
      textEdit_->viewport()->update();
    } else {
      QChar target(e->key());
      target = target.toLower();
      std::cout << "Selected " << e->text().toStdString() << std::endl;

      if (e->modifiers() == Qt::ShiftModifier) {
        target = target.toUpper();
      }

      int newPos = target_.getTargetPos(target);

      if (newPos >= 0) {
        QPlainTextEdit* plainEdit = textEdit_;
        QWidget* viewport = textEdit_->viewport();
        reset();

        if (plainEdit) {
          std::cout << "Moving to position " << std::to_string(newPos) << std::endl;
          moveToPosition(plainEdit, newPos, isVisualMode());
        }

        viewport->update();
      }
    }
    return true;
  }
  return false;
}

bool EventHandler::handlePaintEvent(QPaintEvent*) {
  if (state_ == State::WaitingForSelectionOrMoreCharacters && !target_.isEmpty()) {
    QTextCursor tc = textEdit_->textCursor();
    QFontMetrics fm(textEdit_->font());
    QPainter painter(textEdit_->viewport());

    QPen pen;
    pen.setColor(QColor(255, 0, 0, 255));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.setBrush(QBrush(QColor(255, 255, 0, 255)));
    painter.setFont(textEdit_->font());

    for (int i = target_.getFirstTargetIndex(); i < target_.getLastTargetIndex(); ++i) {
      QPair<int, QChar> target = target_.getTarget(i);
      tc.setPosition(target.first);
      QRect rect = textEdit_->cursorRect(tc);

      int targetCharFontWidth =
          fm.horizontalAdvance(textEdit_->document()->characterAt(target.first));

      if (targetCharFontWidth == 0) {
        targetCharFontWidth = fm.horizontalAdvance(QChar(ushort(' ')));
      }

      rect.setWidth(targetCharFontWidth);

      if (rect.intersects(textEdit_->viewport()->rect())) {
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect);
        painter.setPen(pen);
        const int textHeight = rect.bottom() - fm.descent();
        painter.drawText(rect.left(), textHeight, QString(target.second));
      }
    }

    painter.end();
  }
  return false;
}

bool EventHandler::isModifierKey(int key) {
  return key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt ||
      key == Qt::Key_Meta;
}

bool EventHandler::setEditor(Core::IEditor* e) {
  if (e == nullptr) {
    return false;
  }

  QWidget* widget = e->widget();
  textEdit_ = qobject_cast<QPlainTextEdit*>(widget);
  return textEdit_ != nullptr;
}
} // namespace Qtmotion