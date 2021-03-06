#include "TargetString.h"

#include <iostream>
#include <unordered_set>

#include <QPlainTextEdit>
#include <QTextDocument>

namespace Qtmotion {
void TargetString::appendQuery(QPlainTextEdit* textEdit, const QChar& query) {
  findMatchingPositions(textEdit, query, query_, selectables_, potentialSelectables_);
}

void TargetString::findMatchingPositions(
    QPlainTextEdit* textEdit,
    const QChar& query,
    QString& aggregateQuery,
    std::vector<Target>& selectables,
    std::vector<Target>& potentialSelectables) {
  if (textEdit == nullptr) {
    return;
  }

  QTextDocument* doc = textEdit->document();

  std::vector<int> matchingPositions;

  // First time new query
  if (aggregateQuery.length() == 0) {
    int cursorPos = textEdit->textCursor().position();

    const QPoint bottomRight(textEdit->viewport()->width() - 1, textEdit->viewport()->height() - 1);
    const int startPos = textEdit->cursorForPosition(QPoint(0, 0)).position();
    const int endPos = textEdit->cursorForPosition(bottomRight).position();

    // Cursor currently offscreen
    if (cursorPos < startPos || cursorPos > endPos) {
      cursorPos = (startPos + endPos) / 2;
    }

    bool notCaseSensitive = query.category() != QChar::Letter_Uppercase;

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
  } else {
    // If we have run before, clear out incompatible matches
    for (const auto& selectable : selectables) {
      if (doc->characterAt(selectable.position + aggregateQuery.length()) == query) {
        matchingPositions.push_back(selectable.position);
      }
    }

    for (const auto& potentialSelectable : potentialSelectables) {
      if (doc->characterAt(potentialSelectable.position + aggregateQuery.length()) == query) {
        matchingPositions.push_back(potentialSelectable.position);
      }
    }
  }

  selectables.clear();
  potentialSelectables.clear();

  // Now, find characters the set of characters that don't follow any of the matching positions
  std::vector<char> validCharChoices;
  validCharChoices.insert(validCharChoices.end(), kKeyOrder_.begin(), kKeyOrder_.end());

  for (const auto position : matchingPositions) {
    std::erase(
        validCharChoices,
        doc->characterAt(position + aggregateQuery.length() + 1).toUpper().toLatin1());
    std::erase(
        validCharChoices,
        doc->characterAt(position + aggregateQuery.length() + 1).toLower().toLatin1());
  }

  // Provide the initial set of selectables
  for (size_t i = 0; i < validCharChoices.size() && i < matchingPositions.size(); i++) {
    selectables.push_back(
        Target{.position = matchingPositions[i], .selector = QString(validCharChoices[i])});
  }

  // Backup the ambiguous ones for later
  for (size_t i = validCharChoices.size(); i < matchingPositions.size(); i++) {
    potentialSelectables.push_back(Target{
        .position = matchingPositions[i],
        .selector = QString(doc->characterAt(matchingPositions[i]))});
  }

  aggregateQuery += query;
}

void TargetString::backspace(QPlainTextEdit* textEdit) {
  if (query_.length()) {
    const QString queryLessOne = query_.mid(0, query_.length() - 1);

    reset();

    for (const QChar c : queryLessOne) {
      findMatchingPositions(textEdit, c, query_, selectables_, potentialSelectables_);
    }
  }
}

const QString& TargetString::query() const {
  return query_;
}

void TargetString::reset() {
  query_ = QString();
  selectables_.clear();
  potentialSelectables_.clear();
}

const std::vector<TargetString::Target>& TargetString::selectables() const {
  return selectables_;
}

const std::vector<TargetString::Target>& TargetString::potentialSelectables() const {
  return potentialSelectables_;
}

int TargetString::getPositionForCharSelection(const QChar& c) const {
  for (const auto& selectable : selectables_) {
    if (selectable.selector == c) {
      return selectable.position;
    }
  }
  return -1;
}
} // namespace Qtmotion
