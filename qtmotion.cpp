#include "qtmotion.h"
#include "qtmotionconstants.h"

#include <iostream>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <texteditor/texteditor.h>

#include <QAction>
#include <QApplication>
#include <QChar>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPainter>
#include <QPair>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QString>
#include <QTextBlock>
#include <QTextDocument>
#include <QtPlugin>

namespace Qtmotion {
namespace Internal {

constexpr bool verbose = true;

inline void log(const std::string& str) {
  if constexpr (verbose) {
    std::cout << "Qtmotion: " << str << std::endl;
  }
}

void moveToPosition(QPlainTextEdit* editor, int newPos, bool visualMode) {
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

class QtmotionTarget : public QObject {
  Q_OBJECT
 public:
  QtmotionTarget(void) {
    targetPositions_.clear();
  }

  void findMatchingPositions(QPlainTextEdit* editor, const QChar& target) {
    targetPositions_.clear();

    if (editor == nullptr) {
      return;
    }

    currentGroup_ = 0;
    QTextDocument* doc = editor->document();
    int cursorPos = editor->textCursor().position();

    const QPoint bottomRight(editor->viewport()->width() - 1, editor->viewport()->height() - 1);
    const int startPos = editor->cursorForPosition(QPoint(0, 0)).position();
    const int endPos = editor->cursorForPosition(bottomRight).position();

    bool notCaseSensitive = target.category() != QChar::Letter_Uppercase;

    // Go up and down from the current position matching the target query
    for (int offset = 1; cursorPos - offset >= startPos || cursorPos + offset <= endPos; offset++) {
      if (cursorPos + offset <= endPos) {
        QChar c = doc->characterAt(cursorPos + offset);

        if (notCaseSensitive) {
          c = c.toLower();
        }

        if (c == target) {
          targetPositions_ << (cursorPos + offset);
        }
      }

      if (cursorPos - offset >= startPos) {
        QChar c = doc->characterAt(cursorPos - offset);

        if (notCaseSensitive) {
          c = c.toLower();
        }

        if (c == target) {
          targetPositions_ << (cursorPos - offset);
        }
      }
    }

    log("Found " + std::string(1, target.toLatin1()) + " in " +
        std::to_string(targetPositions_.size()) + " positions");
  }

  bool isEmpty() const {
    return targetPositions_.size() == 0;
  }

  void nextGroup(void) {
    currentGroup_++;
    if (currentGroup_ >= getGroupNum()) {
      currentGroup_ = 0;
    }
  }

  void previousGroup(void) {
    currentGroup_--;
    if (currentGroup_ < 0) {
      currentGroup_ = getGroupNum() - 1;
      if (currentGroup_ < 0) {
        currentGroup_ = 0;
      }
    }
  }

  void clear() {
    currentGroup_ = 0;
    targetPositions_.clear();
  }

  int getFirstTargetIndex(void) const {
    return (int)(currentGroup_ * kKeyOrder_.size());
  }

  int getLastTargetIndex(void) const {
    int onePastLastIndex = (int)(currentGroup_ * kKeyOrder_.size() + kKeyOrder_.size());
    if (onePastLastIndex > targetPositions_.size()) {
      onePastLastIndex = targetPositions_.size();
    }
    return onePastLastIndex;
  }

  QPair<int, QChar> getTarget(int i) const {
    if (i < 0 || i > targetPositions_.size()) {
      return QPair<int, QChar>(int(-1), QChar(0));
    } else {
      return QPair<int, QChar>(targetPositions_[i], QChar(kKeyOrder_[i % kKeyOrder_.size()]));
    }
  }

  int getGroupNum(void) {
    if (targetPositions_.size() == 0) {
      return 0;
    } else {
      return ((int)targetPositions_.size() - 1) / (int)kKeyOrder_.size() + 1;
    }
  }

  int getTargetPos(const QChar& c) const {
    auto it = std::find(kKeyOrder_.begin(), kKeyOrder_.end(), c.toLatin1());

    if (it != kKeyOrder_.end()) {
      const int pos =
          std::distance(kKeyOrder_.begin(), it) + currentGroup_ * (int)kKeyOrder_.size();

      if (pos < targetPositions_.size()) {
        return targetPositions_[pos];
      } else {
        return -1;
      }
    }

    return -1;
  }

 private:
  static constexpr std::array<char, 52> kKeyOrder_ = {
      'j', 'f', 'k', 'd', 'l', 's', 'a', 'h', 'g', 'u', 'r', 'n', 'v', 't', 'i', 'e', 'm', 'c',
      'o', 'w', 'x', 'p', 'q', 'z', 'b', 'y', 'J', 'F', 'K', 'D', 'L', 'S', 'A', 'H', 'G', 'U',
      'R', 'N', 'V', 'T', 'I', 'E', 'M', 'C', 'O', 'W', 'X', 'P', 'Q', 'Z', 'B', 'Y'};

  int currentGroup_;
  QVector<int> targetPositions_;
};

class QtmotionHandler : public QObject {
  Q_OBJECT

 public:
  QtmotionHandler(QObject* parent = nullptr) : QObject(parent) {}

  ~QtmotionHandler() {}

 public slots:
  void easyMotionForEntireScreenTriggered(void) {
    initQtmotion();
  }

 private slots:
  void doInstallEventFilter() {
    if (textEdit_) {
      textEdit_->installEventFilter(this);
      textEdit_->viewport()->installEventFilter(this);
    }
  }

 private:
  void installEventFilter() {
    // Postpone installEventFilter() so plugin gets next key event first.
    QMetaObject::invokeMethod(this, "doInstallEventFilter", Qt::QueuedConnection);
  }

  void initQtmotion() {
    resetQtmotion();

    currentEditor_ = Core::EditorManager::currentEditor();

    if (setEditor(currentEditor_)) {
      state_ = QtmotionState::BeforeFirstCharacter;
      installEventFilter();
      log("Initiated");
    } else {
      currentEditor_ = nullptr;
    }
  }

  void resetQtmotion(void) {
    if (setEditor(currentEditor_)) {
      QWidget* viewport = textEdit_->viewport();
      textEdit_->removeEventFilter(this);
      viewport->removeEventFilter(this);
      textEdit_ = nullptr;
    }
    target_.clear();
    state_ = QtmotionState::Inactive;
    currentEditor_ = nullptr;
  }

  bool isVisualMode() const {
    return textEdit_ && textEdit_->textCursor().hasSelection();
  }

  bool eventFilter(QObject* obj, QEvent* event) {
    QWidget* currentViewport = qobject_cast<QWidget*>(obj);

    if (currentViewport != nullptr && event->type() == QEvent::Paint) {
      // Handle the painter event last to prevent
      // the area painted by Qtmotion to be overidden
      currentViewport->removeEventFilter(this);
      QCoreApplication::sendEvent(currentViewport, event);
      currentViewport->installEventFilter(this);
      QPaintEvent* paintEvent = static_cast<QPaintEvent*>(event);
      handlePaintEvent(paintEvent);
      return true;
    } else if (event->type() == QEvent::KeyPress && textEdit_) {
      installEventFilter();

      QKeyEvent* e = static_cast<QKeyEvent*>(event);
      bool keyPressHandled = handleKeyPress(e);
      return keyPressHandled;
    } else if (event->type() == QEvent::ShortcutOverride) {
      installEventFilter();

      // Handle ESC key press.
      QKeyEvent* e = static_cast<QKeyEvent*>(event);
      if (e->key() == Qt::Key_Escape) {
        return handleKeyPress(e);
      }
    }

    return false;
  }

  bool handleKeyPress(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) {
      // Exit the process
      log("Cancelling");

      QtmotionState tmpState = state_;
      if (tmpState == QtmotionState::WaitingForSelectionOrMoreCharacters) {
        textEdit_->viewport()->update();
      }

      resetQtmotion();

      return true;
    } else if (state_ == QtmotionState::BeforeFirstCharacter && !isModifierKey(e->key())) {
      log("Just triggered with " + e->text().toStdString());

      QChar target(e->key());
      target = target.toLower();

      if (e->modifiers() == Qt::ShiftModifier) {
        target = target.toUpper();
      }

      if (textEdit_) {
        target_.findMatchingPositions(textEdit_, target);
      } else {
        log("textEdit_ is nullptr");
      }

      if (!target_.isEmpty()) {
        state_ = QtmotionState::WaitingForSelectionOrMoreCharacters;
        textEdit_->viewport()->update();
      }

      return true;
    } else if (
        state_ == QtmotionState::WaitingForSelectionOrMoreCharacters && !isModifierKey(e->key())) {
      if (e->key() == Qt::Key_Return) {
        log("Cycling group");

        if (e->modifiers() == Qt::ShiftModifier) {
          // Shift + Enter makes Qtmotion show previous
          // group of target positions
          target_.previousGroup();
        } else {
          // Enter makes Qtmotion show next
          // group of target positions
          target_.nextGroup();
        }
        textEdit_->viewport()->update();
      } else {
        QChar target(e->key());
        target = target.toLower();
        log("Selected " + e->text().toStdString());

        if (e->modifiers() == Qt::ShiftModifier) {
          target = target.toUpper();
        }

        int newPos = target_.getTargetPos(target);

        if (newPos >= 0) {
          QPlainTextEdit* plainEdit = textEdit_;
          QWidget* viewport = textEdit_->viewport();
          resetQtmotion();

          if (plainEdit) {
            log("Moving to position " + std::to_string(newPos));
            moveToPosition(plainEdit, newPos, isVisualMode());
          }

          viewport->update();
        }
      }
      return true;
    }
    return false;
  }

  bool handlePaintEvent(QPaintEvent*) {
    if (state_ == QtmotionState::WaitingForSelectionOrMoreCharacters && !target_.isEmpty()) {
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
          int textHeight = rect.bottom() - fm.descent();
          painter.drawText(rect.left(), textHeight, QString(target.second));
        }
      }

      painter.end();
    }
    return false;
  }

  bool isModifierKey(int key) {
    return key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt ||
        key == Qt::Key_Meta;
  }

  bool setEditor(Core::IEditor* e) {
    if (e == nullptr) {
      return false;
    }

    QWidget* widget = e->widget();
    textEdit_ = qobject_cast<QPlainTextEdit*>(widget);
    return textEdit_ != nullptr;
  }

  enum class QtmotionState { Inactive, BeforeFirstCharacter, WaitingForSelectionOrMoreCharacters };

  Core::IEditor* currentEditor_ = nullptr;
  QPlainTextEdit* textEdit_ = nullptr;
  QtmotionState state_ = QtmotionState::Inactive;
  QtmotionTarget target_;
};

QtmotionPlugin::QtmotionPlugin() : m_handler(std::make_unique<QtmotionHandler>()) {}

QtmotionPlugin::~QtmotionPlugin() {}

bool QtmotionPlugin::initialize(const QStringList&, QString*) {
  QAction* easyMotionSearchEntireScreen = new QAction(tr("Search entire screen"), this);

  Core::Command* searchScreenCmd = Core::ActionManager::registerAction(
      easyMotionSearchEntireScreen,
      Constants::SEARCH_SCREEN_ID,
      Core::Context(Core::Constants::C_EDIT_MODE));

  searchScreenCmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+;")));

  connect(
      easyMotionSearchEntireScreen,
      SIGNAL(triggered()),
      m_handler.get(),
      SLOT(easyMotionForEntireScreenTriggered()));

  return true;
}

void QtmotionPlugin::extensionsInitialized() {}

ExtensionSystem::IPlugin::ShutdownFlag QtmotionPlugin::aboutToShutdown() {
  return SynchronousShutdown;
}

} // namespace Internal
} // namespace Qtmotion

#include "qtmotion.moc"
