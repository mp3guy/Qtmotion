#include "TargetString.h"

#include <iostream>

#include <QPlainTextEdit>
#include <QTextDocument>

namespace Qtmotion {
TargetString::TargetString() {
  targetPositions_.clear();
}

void TargetString::findMatchingPositions(QPlainTextEdit* editor, const QChar& target) {
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

  std::cout << "Found " << std::string(1, target.toLatin1()) << " in "
            << std::to_string(targetPositions_.size()) << " positions" << std::endl;
}

bool TargetString::isEmpty() const {
  return targetPositions_.size() == 0;
}

void TargetString::nextGroup() {
  currentGroup_++;
  if (currentGroup_ >= getGroupNum()) {
    currentGroup_ = 0;
  }
}

void TargetString::previousGroup() {
  currentGroup_--;
  if (currentGroup_ < 0) {
    currentGroup_ = getGroupNum() - 1;
    if (currentGroup_ < 0) {
      currentGroup_ = 0;
    }
  }
}

void TargetString::clear() {
  currentGroup_ = 0;
  targetPositions_.clear();
}

int TargetString::getFirstTargetIndex() const {
  return (int)(currentGroup_ * kKeyOrder_.size());
}

int TargetString::getLastTargetIndex() const {
  int onePastLastIndex = (int)(currentGroup_ * kKeyOrder_.size() + kKeyOrder_.size());
  if (onePastLastIndex > targetPositions_.size()) {
    onePastLastIndex = targetPositions_.size();
  }
  return onePastLastIndex;
}

QPair<int, QChar> TargetString::getTarget(int i) const {
  if (i < 0 || i > targetPositions_.size()) {
    return QPair<int, QChar>(int(-1), QChar(0));
  } else {
    return QPair<int, QChar>(targetPositions_[i], QChar(kKeyOrder_[i % kKeyOrder_.size()]));
  }
}

int TargetString::getGroupNum() {
  if (targetPositions_.size() == 0) {
    return 0;
  } else {
    return ((int)targetPositions_.size() - 1) / (int)kKeyOrder_.size() + 1;
  }
}

int TargetString::getTargetPos(const QChar& c) const {
  auto it = std::find(kKeyOrder_.begin(), kKeyOrder_.end(), c.toLatin1());

  if (it != kKeyOrder_.end()) {
    const int pos = std::distance(kKeyOrder_.begin(), it) + currentGroup_ * (int)kKeyOrder_.size();

    if (pos < targetPositions_.size()) {
      return targetPositions_[pos];
    } else {
      return -1;
    }
  }

  return -1;
}
} // namespace Qtmotion
