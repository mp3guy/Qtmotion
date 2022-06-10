#pragma once

#include <QObject>

class QPlainTextEdit;

namespace Qtmotion {
class TargetString {
 public:
  TargetString() = default;

  void appendQuery(QPlainTextEdit* textEdit, const QChar& query);

  void reset();

  const QString& query() const;

  void backspace(QPlainTextEdit* textEdit);

  struct Target {
    int position;
    QString selector;
  };

  const std::vector<Target>& selectables() const;
  const std::vector<Target>& potentialSelectables() const;
  int getPositionForCharSelection(const QChar& c) const;

 private:
  static void findMatchingPositions(
      QPlainTextEdit* textEdit,
      const QChar& query,
      QString& aggregateQuery,
      std::vector<Target>& selectables,
      std::vector<Target>& potentialSelectables);

  static constexpr std::array<char, 52> kKeyOrder_ = {
      'j', 'f', 'k', 'd', 'l', 's', 'a', 'h', 'g', 'u', 'r', 'n', 'v', 't', 'i', 'e', 'm', 'c',
      'o', 'w', 'x', 'p', 'q', 'z', 'b', 'y', 'J', 'F', 'K', 'D', 'L', 'S', 'A', 'H', 'G', 'U',
      'R', 'N', 'V', 'T', 'I', 'E', 'M', 'C', 'O', 'W', 'X', 'P', 'Q', 'Z', 'B', 'Y'};

  QString query_;
  std::vector<Target> selectables_;
  std::vector<Target> potentialSelectables_;
};
} // namespace Qtmotion
