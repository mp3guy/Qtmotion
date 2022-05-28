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
#include <QDebug>
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

template <class Editor>
QPair<int, int> getFirstAndLastVisiblePosition(Editor* editor) {
  QTextCursor cursor = editor->textCursor();
  QTextDocument* doc = editor->document();
  int currentLine = doc->findBlock(cursor.position()).blockNumber();
  int cursorHeight = editor->cursorRect().height();
  int lineCountToFirstVisibleLine = editor->cursorRect().top() / cursorHeight;
  int firstVisibleLineNum = currentLine - lineCountToFirstVisibleLine;
  if (firstVisibleLineNum < 0) {
    firstVisibleLineNum = 0;
  }
  int maxLineNumOnScreen = (editor->viewport()->height() / cursorHeight);
  if (maxLineNumOnScreen < 1) {
    maxLineNumOnScreen = 1;
  }
  int firstPos = doc->findBlockByNumber(firstVisibleLineNum).position();
  int lastVisibleLineNum = firstVisibleLineNum + maxLineNumOnScreen - 1;
  QTextBlock lastVisibleTextBlock = doc->findBlockByNumber(lastVisibleLineNum);
  if (!lastVisibleTextBlock.isValid()) {
    lastVisibleTextBlock = doc->lastBlock();
  }
  int lastPos = lastVisibleTextBlock.position() + lastVisibleTextBlock.length() - 1;

  log("Visible lines " + std::to_string(firstVisibleLineNum + 1) + "->" +
      std::to_string(lastVisibleLineNum + 1));

  return QPair<int, int>(firstPos, lastPos);
}

template <class Editor>
void moveToPosition(Editor* editor, int newPos, bool visualMode) {
  QTextBlock targetBlock = editor->document()->findBlock(newPos);
  if (!targetBlock.isValid())
    targetBlock = editor->document()->lastBlock();

  bool overwriteMode = editor->overwriteMode();
  TextEditor::TextEditorWidget* baseEditor = qobject_cast<TextEditor::TextEditorWidget*>(editor);
  bool visualBlockMode = baseEditor && baseEditor->multiTextCursor().hasMultipleCursors();

  bool selectNextCharacter = (overwriteMode || visualMode) && !visualBlockMode;
  bool keepSelection = visualMode || visualBlockMode;

  QTextCursor textCursor = editor->textCursor();
  textCursor.setPosition(
      selectNextCharacter ? newPos : newPos + 1,
      keepSelection ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);

  if (baseEditor)
    baseEditor->setTextCursor(textCursor);
  else
    editor->setTextCursor(textCursor);

  if (visualBlockMode) {
    baseEditor->setTextCursor(baseEditor->textCursor());
  }
}

class QtmotionTarget : public QObject {
  Q_OBJECT
 public:
  QtmotionTarget(void) {
    m_targetPos.clear();
  }

  template <class QEditor>
  void searchTargetFromScreen(QEditor* editor, const QChar& target) {
    m_targetPos.clear();
    if (editor == nullptr) {
      return;
    }
    m_currentGroup = 0;
    QTextDocument* doc = editor->document();
    int cursorPos = editor->textCursor().position();
    QPair<int, int> visibleRange = getFirstAndLastVisiblePosition(editor);
    int firstPos = visibleRange.first;
    int lastPos = visibleRange.second;
    bool notCaseSensative = target.category() != QChar::Letter_Uppercase;
    for (int offset = 1; cursorPos - offset >= firstPos || cursorPos + offset <= lastPos;
         offset++) {
      if (cursorPos + offset <= lastPos) {
        QChar c = doc->characterAt(cursorPos + offset);
        if (notCaseSensative) {
          c = c.toLower();
        }
        if (c == target) {
          m_targetPos << (cursorPos + offset);
        }
      }
      if (cursorPos - offset >= firstPos) {
        QChar c = doc->characterAt(cursorPos - offset);
        if (notCaseSensative) {
          c = c.toLower();
        }
        if (c == target) {
          m_targetPos << (cursorPos - offset);
        }
      }
    }
  }

  int size() const {
    return m_targetPos.size();
  }

  bool isEmpty() const {
    return m_targetPos.size() == 0;
  }

  void nextGroup(void) {
    m_currentGroup++;
    if (m_currentGroup >= getGroupNum()) {
      m_currentGroup = 0;
    }
  }

  void previousGroup(void) {
    m_currentGroup--;
    if (m_currentGroup < 0) {
      m_currentGroup = getGroupNum() - 1;
      if (m_currentGroup < 0) {
        m_currentGroup = 0;
      }
    }
  }

  void clear() {
    m_currentGroup = 0;
    m_targetPos.clear();
  }

  int getFirstTargetIndex(void) const {
    return (int)(m_currentGroup * kKeyOrder.size());
  }

  int getLastTargetIndex(void) const {
    int onePastLastIndex = (int)(m_currentGroup * kKeyOrder.size() + kKeyOrder.size());
    if (onePastLastIndex > m_targetPos.size()) {
      onePastLastIndex = m_targetPos.size();
    }
    return onePastLastIndex;
  }

  QPair<int, QChar> getTarget(int i) const {
    if (i < 0 || i > m_targetPos.size()) {
      return QPair<int, QChar>(int(-1), QChar(0));
    } else {
      return QPair<int, QChar>(m_targetPos[i], QChar(kKeyOrder[i % kKeyOrder.size()]));
    }
  }

  int getGroupNum(void) {
    if (m_targetPos.size() == 0) {
      return 0;
    } else {
      return ((int)m_targetPos.size() - 1) / (int)kKeyOrder.size() + 1;
    }
  }

  int getTargetPos(const QChar& c) const {
    auto it = std::find(kKeyOrder.begin(), kKeyOrder.end(), c.toLatin1());

    if (it != kKeyOrder.end()) {
      const int pos = std::distance(kKeyOrder.begin(), it) + m_currentGroup * (int)kKeyOrder.size();

      if (pos < m_targetPos.size()) {
        return m_targetPos[pos];
      } else {
        return -1;
      }
    }

    return -1;
  }

 private:
  static constexpr std::array<char, 52> kKeyOrder = {
      'j', 'f', 'k', 'd', 'l', 's', 'a', 'h', 'g', 'u', 'r', 'n', 'v', 't', 'i', 'e', 'm', 'c',
      'o', 'w', 'x', 'p', 'q', 'z', 'b', 'y', 'J', 'F', 'K', 'D', 'L', 'S', 'A', 'H', 'G', 'U',
      'R', 'N', 'V', 'T', 'I', 'E', 'M', 'C', 'O', 'W', 'X', 'P', 'Q', 'Z', 'B', 'Y'};

  int m_currentGroup;
  QVector<int> m_targetPos;
};

#define EDITOR(e) ((m_plainEdit != nullptr) ? m_plainEdit->e : m_textEdit->e)

class QtmotionHandler : public QObject {
  Q_OBJECT

 public:
  QtmotionHandler(QObject* parent = 0)
      : QObject(parent),
        m_currentEditor(nullptr),
        m_plainEdit(nullptr),
        m_textEdit(nullptr),
        m_fakeVimStatusWidget(0),
        m_state(DefaultState),
        m_easyMotionSearchRange(-1) {
    QMetaObject::invokeMethod(this, "findFakeVimStatusWidget", Qt::QueuedConnection);
  }

  ~QtmotionHandler() {}

 public slots:
  void easyMotionForEntireScreenTriggered(void) {
    initQtmotion();
  }

 private slots:
  void doInstallEventFilter() {
    if (m_plainEdit || m_textEdit) {
      EDITOR(installEventFilter(this));
      EDITOR(viewport())->installEventFilter(this);
    }
  }

  void findFakeVimStatusWidget() {
    QWidget* statusBar = Core::ICore::statusBar();
    foreach(QWidget * w, statusBar->findChildren<QWidget*>()) {
      if (QLatin1String(w->metaObject()->className()) ==
          QLatin1String("FakeVim::Internal::MiniBuffer")) {
        m_fakeVimStatusWidget = w->findChild<QLabel*>();
        break;
      }
    }
  }

 private:
  void installEventFilter() {
    // Postpone installEventFilter() so plugin gets next key event first.
    QMetaObject::invokeMethod(this, "doInstallEventFilter", Qt::QueuedConnection);
  }

  void initQtmotion() {
    resetQtmotion();
    m_currentEditor = Core::EditorManager::currentEditor();
    if (setEditor(m_currentEditor)) {
      m_state = QtmotionTriggered;
      installEventFilter();
      log("Initiated");
    } else {
      m_currentEditor = nullptr;
    }
  }

  void resetQtmotion(void) {
    if (setEditor(m_currentEditor)) {
      QWidget* viewport = EDITOR(viewport());
      EDITOR(removeEventFilter(this));
      viewport->removeEventFilter(this);
      unsetEditor();
    }
    m_target.clear();
    m_state = DefaultState;
    m_currentEditor = nullptr;
  }

  bool isVisualMode() const {
    if (m_fakeVimStatusWidget)
      return m_fakeVimStatusWidget->text().contains(QLatin1String("VISUAL"));
    return (m_plainEdit || m_textEdit) && EDITOR(textCursor()).hasSelection();
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
    } else if (event->type() == QEvent::KeyPress) {
      if (m_plainEdit || m_textEdit) {
        installEventFilter();
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        bool keyPressHandled = handleKeyPress(e);
        return keyPressHandled;
      }
    } else if (event->type() == QEvent::ShortcutOverride) {
      installEventFilter();
      // Handle ESC key press.
      QKeyEvent* e = static_cast<QKeyEvent*>(event);
      if (e->key() == Qt::Key_Escape)
        return handleKeyPress(e);
    }
    return false;
  }

  bool handleKeyPress(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) {
      // Exit the process
      log("Cancelling");

      QtmotionState tmpState = m_state;
      if (tmpState == WaitForInputTargetCode) {
        EDITOR(viewport()->update());
      }
      resetQtmotion();

      return true;

    } else if (m_state == QtmotionTriggered && !isModifierKey(e->key())) {
      log("Just triggered with " + e->text().toStdString());

      QChar target(e->key());
      target = target.toLower();
      if (e->modifiers() == Qt::ShiftModifier)
        target = target.toUpper();
      if (m_plainEdit) {
        m_target.searchTargetFromScreen(m_plainEdit, target);
      } else if (m_textEdit) {
        m_target.searchTargetFromScreen(m_textEdit, target);
      } else {
        qDebug() << "QtmotionHandler::handleKeyPress() => Error: current editor is null";
      }
      if (!m_target.isEmpty()) {
        m_state = WaitForInputTargetCode;
        EDITOR(viewport()->update());
      }
      return true;
    } else if (m_state == WaitForInputTargetCode && !isModifierKey(e->key())) {
      if (e->key() == Qt::Key_Return) {
        log("Cycling group");

        if (e->modifiers() == Qt::ShiftModifier) {
          // Shift + Enter makes Qtmotion show previous
          // group of target positions
          m_target.previousGroup();
        } else {
          // Enter makes Qtmotion show next
          // group of target positions
          m_target.nextGroup();
        }
        EDITOR(viewport()->update());
      } else {
        QChar target(e->key());
        target = target.toLower();
        log("Selected " + e->text().toStdString());

        if (e->modifiers() == Qt::ShiftModifier) {
          target = target.toUpper();
        }

        int newPos = m_target.getTargetPos(target);

        if (newPos >= 0) {
          QPlainTextEdit* plainEdit = m_plainEdit;
          QTextEdit* textEdit = m_textEdit;
          QWidget* viewport = EDITOR(viewport());
          resetQtmotion();

          if (plainEdit) {
            moveToPosition(plainEdit, newPos, isVisualMode());
          } else if (textEdit) {
            moveToPosition(textEdit, newPos, isVisualMode());
          }

          viewport->update();
        }
      }
      return true;
    }
    return false;
  }

  bool handlePaintEvent(QPaintEvent*) {
    if (m_state == WaitForInputTargetCode && !m_target.isEmpty()) {
      QTextCursor tc = EDITOR(textCursor());
      QFontMetrics fm(EDITOR(font()));
      QPainter painter(EDITOR(viewport()));
      QPen pen;
      pen.setColor(QColor(255, 0, 0, 255));
      painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
      painter.setBrush(QBrush(QColor(255, 255, 0, 255)));
      painter.setFont(EDITOR(font()));
      for (int i = m_target.getFirstTargetIndex(); i < m_target.getLastTargetIndex(); ++i) {
        QPair<int, QChar> target = m_target.getTarget(i);
        tc.setPosition(target.first);
        QRect rect = EDITOR(cursorRect(tc));

        int targetCharFontWidth =
            fm.horizontalAdvance(EDITOR(document())->characterAt(target.first));

        if (targetCharFontWidth == 0) {
          targetCharFontWidth = fm.horizontalAdvance(QChar(ushort(' ')));
        }

        rect.setWidth(targetCharFontWidth);

        if (rect.intersects(EDITOR(viewport()->rect()))) {
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
    if (e == nullptr)
      return false;
    QWidget* widget = e->widget();
    m_plainEdit = qobject_cast<QPlainTextEdit*>(widget);
    m_textEdit = qobject_cast<QTextEdit*>(widget);
    return m_plainEdit != nullptr || m_textEdit != nullptr;
  }

  void unsetEditor() {
    m_plainEdit = nullptr;
    m_textEdit = nullptr;
  }

  enum QtmotionState { DefaultState, QtmotionTriggered, WaitForInputTargetCode };

  Core::IEditor* m_currentEditor;
  QPlainTextEdit* m_plainEdit;
  QTextEdit* m_textEdit;
  QLabel* m_fakeVimStatusWidget;
  QtmotionState m_state;
  QtmotionTarget m_target;
  int m_easyMotionSearchRange;
};

QtmotionPlugin::QtmotionPlugin() : m_handler(std::make_unique<QtmotionHandler>()) {}

QtmotionPlugin::~QtmotionPlugin() {}

bool QtmotionPlugin::initialize(const QStringList&, QString*) {
  QAction* easyMotionSearchEntireScreen = new QAction(tr("Search entire screen"), this);
  Core::Command* searchScreenCmd = Core::ActionManager::registerAction(
      easyMotionSearchEntireScreen,
      Constants::SEARCH_SCREEN_ID,
      Core::Context(Core::Constants::C_GLOBAL));
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
