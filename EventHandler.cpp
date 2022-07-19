#include "EventHandler.h"

#include <cstdlib>
#include <iostream>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <texteditor/texteditor.h>
#include <utils/mapreduce.h>

#include <QFutureInterface>
#include <QPainter>
#include <QPlainTextEdit>
#include <QTextBlock>

namespace Qtmotion {
EventHandler::EventHandler() {
  handlers_.push_back(this);
}

void EventHandler::triggerBeforeChar() {
  trigger(true, false);
}

void EventHandler::triggerAfterChar() {
  trigger(false, false);
}

void EventHandler::triggerBeforeCharSelect() {
  trigger(true, true);
}

void EventHandler::triggerAfterCharSelect() {
  trigger(false, true);
}

void EventHandler::updateCommand(const Settings& settings) {
  commandSettings_ = settings;
}

void EventHandler::triggerCommand() {
  const auto editor = Core::EditorManager::currentEditor();

  if (editor && commandSettings_.passFilename()) {
    filePath_ = editor->document()->filePath().absolutePath().toString().toStdString();
  } else {
    filePath_ = "";
  }

  if ((editor && commandSettings_.passFilename()) || !commandSettings_.passFilename()) {
    Core::ProgressManager::addTask(
        Utils::map(handlers_, &EventHandler::runCommand), tr("Command"), Utils::Id("Command"));
  }
}

void EventHandler::runCommand(QFutureInterface<void>& future) {
  future.setProgressRange(0, 100);

  const std::string commandToRun =
      commandSettings_.command() + (filePath_.length() ? " " + filePath_ : "");

  while (!future.isCanceled()) {
    future.setProgressValue(50);
    std::system(commandToRun.c_str());
    break;
  }
}

void EventHandler::trigger(const bool beforeChar, const bool selection) {
  if (Core::EditorManager::currentEditor()->widget()->hasFocus()) {
    reset();

    currentEditor_ = Core::EditorManager::currentEditor();

    if (setEditor(currentEditor_)) {
      state_ = State::WaitingForInput;
      enqueueEventFilter();
      beforeChar_ = beforeChar;
      selection_ = selection;
      textEdit_->viewport()->update();
    } else {
      currentEditor_ = nullptr;
    }
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

  beforeChar_ = false;
  selection_ = false;
  target_.reset();
  state_ = State::Inactive;
  currentEditor_ = nullptr;
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

void EventHandler::moveToPosition(QPlainTextEdit* textEdit, int newPos) {
  QTextBlock targetBlock = textEdit->document()->findBlock(newPos);

  if (!targetBlock.isValid()) {
    targetBlock = textEdit->document()->lastBlock();
  }

  QTextCursor textCursor = textEdit->textCursor();
  textCursor.setPosition(
      beforeChar_ ? newPos : newPos + 1,
      selection_ ? QTextCursor::MoveMode::KeepAnchor : QTextCursor::MoveMode::MoveAnchor);

  textEdit->moveCursor(QTextCursor::End);
  textEdit->setTextCursor(textCursor);
}

bool EventHandler::handleKeyPress(QKeyEvent* e) {
  if (e->key() == Qt::Key_Escape) {
    // Exit the process
    if (state_ == State::WaitingForInput) {
      textEdit_->viewport()->update();
    }

    reset();

    return true;
  } else if (state_ == State::WaitingForInput && !isModifierKey(e->key())) {
    if (e->key() == Qt::Key_Backspace) {
      target_.backspace(textEdit_);
    } else {
      QChar target(e->key());
      target = target.toLower();

      if (e->modifiers() == Qt::ShiftModifier) {
        target = target.toUpper();
      }

      int newPos = target_.getPositionForCharSelection(target);

      if (newPos >= 0) {
        QPlainTextEdit* textEdit = textEdit_;
        QWidget* viewport = textEdit_->viewport();

        if (textEdit) {
          moveToPosition(textEdit, newPos);
        }

        reset();

        viewport->update();
      } else if (textEdit_) {
        target_.appendQuery(textEdit_, target);
      }
    }

    return true;
  }
  return false;
}

void EventHandler::handlePaintEvent(QPaintEvent*) {
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

      if (state_ == State::WaitingForInput) {
        toDraw.append(
            QString("Query \"") + target_.query() + "\" found in " +
            QString::fromStdString(std::to_string(
                target_.selectables().size() + target_.potentialSelectables().size())) +
            " locations");
      }

      const QRect textBoundingBox = fm.boundingRect(toDraw);
      const int textWidth = fm.horizontalAdvance(toDraw);
      QRect rect;
      rect.setLeft(textEdit_->viewport()->width() - textWidth);
      rect.setWidth(textWidth);
      rect.setTop(0);
      rect.setHeight(textBoundingBox.height());

      if (target_.selectables().size() > 0) {
        pen.setColor(QColor(170, 170, 255, 255));
        painter.setBrush(QBrush(QColor(54, 54, 85, 255)));
      } else {
        pen.setColor(QColor(255, 170, 170, 255));
        painter.setBrush(QBrush(QColor(85, 54, 54, 255)));
      }
      drawRectText(rect, toDraw);
    }

    for (size_t i = 0; i < target_.selectables().size(); ++i) {
      const TargetString::Target target = target_.selectables()[i];
      tc.setPosition(target.position);

      QRect rect = textEdit_->cursorRect(tc);

      const QChar character = textEdit_->document()->characterAt(target.position);

      rect.setWidth(fm.horizontalAdvance(character));

      pen.setColor(QColor(170, 170, 255, 255));
      painter.setBrush(QBrush(QColor(54, 54, 85, 255)));
      drawRectText(rect, target.selector);
    }

    for (size_t i = 0; i < target_.potentialSelectables().size(); ++i) {
      const TargetString::Target target = target_.potentialSelectables()[i];
      tc.setPosition(target.position);

      QRect rect = textEdit_->cursorRect(tc);

      rect.setWidth(fm.horizontalAdvance(target.selector));

      pen.setColor(QColor(255, 170, 170, 255));
      painter.setBrush(QBrush(QColor(85, 54, 54, 255)));
      drawRectText(rect, target.selector);
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
