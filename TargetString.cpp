#include "TargetString.h"

#include <iostream>
#include <unordered_set>

#include <QPlainTextEdit>
#include <QTextDocument>

namespace Qtmotion {
TargetString::TargetString() {}

void TargetString::findMatchingPositions(QPlainTextEdit* textEdit, const QChar& query) {
  if (textEdit == nullptr) {
    return;
  }

  QTextDocument* doc = textEdit->document();
  int cursorPos = textEdit->textCursor().position();

  const QPoint bottomRight(textEdit->viewport()->width() - 1, textEdit->viewport()->height() - 1);
  const int startPos = textEdit->cursorForPosition(QPoint(0, 0)).position();
  const int endPos = textEdit->cursorForPosition(bottomRight).position();

  bool notCaseSensitive = query.category() != QChar::Letter_Uppercase;

  std::vector<int> matchingPositions;

  // Go up and down from the current position matching the target query
  for (int offset = 1; cursorPos - offset >= startPos || cursorPos + offset <= endPos; offset++) {
    if (cursorPos + offset <= endPos) {
      QChar c = doc->characterAt(cursorPos + offset);

      if (notCaseSensitive) {
        c = c.toLower();
      }

      if (c == query) {
        matchingPositions.push_back(cursorPos + offset);
      }
    }

    if (cursorPos - offset >= startPos) {
      QChar c = doc->characterAt(cursorPos - offset);

      if (notCaseSensitive) {
        c = c.toLower();
      }

      if (c == query) {
        matchingPositions.push_back(cursorPos - offset);
      }
    }
  }

  // Now, find characters the set of characters that don't follow any of the matching positions
  std::vector<char> validCharChoices;
  validCharChoices.insert(validCharChoices.end(), kKeyOrder_.begin(), kKeyOrder_.end());

  for (const auto position : matchingPositions) {
    std::erase(validCharChoices, doc->characterAt(position + 1).toLatin1());
  }

  // Provide the initial set of selectables
  for (size_t i = 0; i < validCharChoices.size() && i < matchingPositions.size(); i++) {
    selectables_.push_back(
        Target{.position = matchingPositions[i], .selector = validCharChoices[i]});
  }

  // Backup the ambiguous ones for later
  for (size_t i = validCharChoices.size(); i < matchingPositions.size(); i++) {
    potentialSelectables_.push_back(matchingPositions[i]);
  }

  query_ = query;
}

void TargetString::appendCharUpdateMatches(QPlainTextEdit* textEdit, const QChar& query) {
  if (textEdit == nullptr || query_.isEmpty()) {
    return;
  }

  QTextDocument* doc = textEdit->document();

  // Go through the selectables and remove the ones that don't match the query
  std::vector<int> newPositions;

  for (const auto& selectable : selectables_) {
    if (doc->characterAt(selectable.position + query_.length()) == query) {
      newPositions.push_back(selectable.position);
    }
  }

  for (const auto& position : potentialSelectables_) {
    if (doc->characterAt(position + query_.length()) == query) {
      newPositions.push_back(position);
    }
  }

  selectables_.clear();
  potentialSelectables_.clear();

  // TODO refactor this and prev method
  //  Now, find characters the set of characters that don't follow any of the matching positions
  std::vector<char> validCharChoices;
  validCharChoices.insert(validCharChoices.end(), kKeyOrder_.begin(), kKeyOrder_.end());

  for (const auto position : newPositions) {
    std::erase(validCharChoices, doc->characterAt(position + query_.length() + 1).toLatin1());
  }

  // Provide the initial set of selectables
  for (size_t i = 0; i < validCharChoices.size() && i < newPositions.size(); i++) {
    selectables_.push_back(Target{.position = newPositions[i], .selector = validCharChoices[i]});
  }

  // Backup the ambiguous ones for later
  for (size_t i = validCharChoices.size(); i < newPositions.size(); i++) {
    potentialSelectables_.push_back(newPositions[i]);
  }

  query_ += query;
}

const QString& TargetString::query() const {
  return query_;
}

void TargetString::clear() {
  query_ = QString();
  selectables_.clear();
  potentialSelectables_.clear();
}

const std::vector<TargetString::Target>& TargetString::selectables() const {
  return selectables_;
}

const std::vector<int>& TargetString::potentialSelectables() const {
  return potentialSelectables_;
}

int TargetString::getTargetPos(const QChar& c) const {
  for (const auto& selectable : selectables_) {
    if (selectable.selector == c) {
      return selectable.position;
    }
  }
  return -1;
}
} // namespace Qtmotion
