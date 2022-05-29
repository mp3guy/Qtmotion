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
  QPlainTextEdit* currentTextEdit = qobject_cast<QPlainTextEdit*>(obj);

  if (currentTextEdit == nullptr && currentViewport != nullptr && event->type() == QEvent::Paint) {
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

void EventHandler::moveToPosition(QPlainTextEdit* textEdit, int newPos, bool visualMode) {
  QTextBlock targetBlock = textEdit->document()->findBlock(newPos);

  if (!targetBlock.isValid()) {
    targetBlock = textEdit->document()->lastBlock();
  }

  bool overwriteMode = textEdit->overwriteMode();
  TextEditor::TextEditorWidget* baseEditor = qobject_cast<TextEditor::TextEditorWidget*>(textEdit);
  bool visualBlockMode = baseEditor && baseEditor->multiTextCursor().hasMultipleCursors();

  bool selectNextCharacter = (overwriteMode || visualMode) && !visualBlockMode;
  bool keepSelection = visualMode || visualBlockMode;

  QTextCursor textCursor = textEdit->textCursor();
  textCursor.setPosition(
      selectNextCharacter ? newPos - 1 : newPos,
      keepSelection ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);

  if (baseEditor) {
    baseEditor->setTextCursor(textCursor);
  } else {
    textEdit->setTextCursor(textCursor);
  }

  if (visualBlockMode) {
    baseEditor->setTextCursor(baseEditor->textCursor());
  }
}

bool EventHandler::handleKeyPress(QKeyEvent* e) {
  if (e->key() == Qt::Key_Escape) {
    // Exit the process
    if (state_ == State::WaitingForSelectionOrMoreCharacters) {
      textEdit_->viewport()->update();
    }

    reset();

    return true;
  } else if (state_ == State::BeforeFirstCharacter && !isModifierKey(e->key())) {
    QChar target(e->key());
    target = target.toLower();

    if (e->modifiers() == Qt::ShiftModifier) {
      target = target.toUpper();
    }

    if (textEdit_) {
      target_.findMatchingPositions(textEdit_, target);
    }

    if (target_.numMatches() > 0) {
      state_ = State::WaitingForSelectionOrMoreCharacters;
      textEdit_->viewport()->update();
    }

    return true;
  } else if (state_ == State::WaitingForSelectionOrMoreCharacters && !isModifierKey(e->key())) {
    QChar target(e->key());
    target = target.toLower();

    if (e->modifiers() == Qt::ShiftModifier) {
      target = target.toUpper();
    }

    int newPos = target_.getTargetPos(target);

    if (newPos >= 0) {
      QPlainTextEdit* textEdit = textEdit_;
      QWidget* viewport = textEdit_->viewport();
      reset();

      if (textEdit) {
        moveToPosition(textEdit, newPos, isVisualMode());
      }

      viewport->update();
    }

    return true;
  }
  return false;
}

void EventHandler::handlePaintEvent(QPaintEvent* paintEvent) {
  if (state_ != State::Inactive) {
    QTextCursor tc = textEdit_->textCursor();
    QFontMetrics fm(textEdit_->font());
    QPainter painter(textEdit_->viewport());

    textEdit_->viewport()->update();

    QPen pen;
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    QFont font = textEdit_->font();
    font.setBold(true);
    painter.setFont(font);

    auto drawRectText = [&, this](const QRect& rect, const QString& string) {
      if (rect.intersects(textEdit_->viewport()->rect())) {
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect);
        painter.setPen(pen);
        const int textHeight = rect.bottom() - fm.descent();
        painter.drawText(rect.left(), textHeight, string);
      }
    };

    {
      QString toDraw = "Qtmotion: ";

      if (state_ == State::BeforeFirstCharacter) {
        toDraw.append(QString("Waiting for input"));
      } else if (state_ == State::WaitingForSelectionOrMoreCharacters) {
        toDraw.append(
            QString("Query \"") + target_.query() + "\" found in " +
            QString::fromStdString(std::to_string(target_.numMatches())) + " locations");
      }

      const QRect textBoundingBox = fm.boundingRect(toDraw);
      const int textWidth = fm.horizontalAdvance(toDraw);
      QRect rect;
      rect.setLeft(textEdit_->viewport()->width() - textWidth);
      rect.setWidth(textWidth);
      rect.setTop(0);
      rect.setHeight(textBoundingBox.height());

      pen.setColor(QColor(170, 170, 255, 255));
      painter.setBrush(QBrush(QColor(54, 54, 85, 255)));
      drawRectText(rect, toDraw);
    }

    for (int i = 0; i < target_.numMatches(); ++i) {
      const TargetString::Target target = target_.getTarget(i);
      tc.setPosition(target.position);
      QRect rect = textEdit_->cursorRect(tc);

      int targetCharFontWidth =
          fm.horizontalAdvance(textEdit_->document()->characterAt(target.position));

      if (targetCharFontWidth == 0) {
        targetCharFontWidth = fm.horizontalAdvance(" ");
      }

      rect.setWidth(targetCharFontWidth);

      pen.setColor(QColor(170, 170, 255, 255));
      painter.setBrush(QBrush(QColor(54, 54, 85, 255)));
      drawRectText(rect, target.value);
    }

    painter.end();
  }
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
