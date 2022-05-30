#pragma once

#include <QObject>

class QPlainTextEdit;

namespace Qtmotion {
class TargetString {
 public:
  TargetString();

  void findMatchingPositions(QPlainTextEdit* textEdit, const QChar& query);
  void appendCharUpdateMatches(QPlainTextEdit* textEdit, const QChar& query);

  void clear();

  const QString& query() const;

  struct Target {
    int position;
    QChar selector;
  };

  const std::vector<Target>& selectables() const;
  const std::vector<int>& potentialSelectables() const;
  int getTargetPos(const QChar& c) const;

 private:
  static constexpr std::array<char, 52> kKeyOrder_ = {
      'j', 'f', 'k', 'd', 'l', 's', 'a', 'h', 'g', 'u', 'r', 'n', 'v', 't', 'i', 'e', 'm', 'c',
      'o', 'w', 'x', 'p', 'q', 'z', 'b', 'y', 'J', 'F', 'K', 'D', 'L', 'S', 'A', 'H', 'G', 'U',
      'R', 'N', 'V', 'T', 'I', 'E', 'M', 'C', 'O', 'W', 'X', 'P', 'Q', 'Z', 'B', 'Y'};

  QString query_;
  std::vector<Target> selectables_;
  std::vector<int> potentialSelectables_;
};
} // namespace Qtmotion
