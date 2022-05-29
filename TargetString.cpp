#include "TargetString.h"

#include <iostream>

#include <QPlainTextEdit>
#include <QTextDocument>

namespace Qtmotion {
TargetString::TargetString() {}

void TargetString::findMatchingPositions(QPlainTextEdit* textEdit, const QChar& query) {
  targetPositions_.clear();

  if (textEdit == nullptr) {
    return;
  }

  QTextDocument* doc = textEdit->document();
  int cursorPos = textEdit->textCursor().position();

  const QPoint bottomRight(textEdit->viewport()->width() - 1, textEdit->viewport()->height() - 1);
  const int startPos = textEdit->cursorForPosition(QPoint(0, 0)).position();
  const int endPos = textEdit->cursorForPosition(bottomRight).position();

  bool notCaseSensitive = query.category() != QChar::Letter_Uppercase;

  query_ = query;

  // Go up and down from the current position matching the target query
  for (int offset = 1; cursorPos - offset >= startPos || cursorPos + offset <= endPos; offset++) {
    if (cursorPos + offset <= endPos) {
      QChar c = doc->characterAt(cursorPos + offset);

      if (notCaseSensitive) {
        c = c.toLower();
      }

      if (c == query) {
        targetPositions_.push_back(cursorPos + offset);
      }
    }

    if (cursorPos - offset >= startPos) {
      QChar c = doc->characterAt(cursorPos - offset);

      if (notCaseSensitive) {
        c = c.toLower();
      }

      if (c == query) {
        targetPositions_.push_back(cursorPos - offset);
      }
    }
  }
}

const QString& TargetString::query() const {
  return query_;
}

void TargetString::clear() {
  targetPositions_.clear();
}

TargetString::Target TargetString::getTarget(int i) const {
  return Target{
      .position = targetPositions_[i],
      .value = i < kKeyOrder_.size() ? QString(kKeyOrder_[i]) : "*"};
}

int TargetString::numMatches() const {
  return targetPositions_.size();
}

int TargetString::getTargetPos(const QChar& c) const {
  auto it = std::find(kKeyOrder_.begin(), kKeyOrder_.end(), c.toLatin1());

  if (it != kKeyOrder_.end()) {
    const int pos = std::distance(kKeyOrder_.begin(), it);

    if (pos < targetPositions_.size()) {
      return targetPositions_[pos];
    } else {
      return -1;
    }
  }

  return -1;
}
} // namespace Qtmotion
