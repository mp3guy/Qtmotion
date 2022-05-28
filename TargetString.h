#pragma once

#include <QObject>

class QPlainTextEdit;

namespace Qtmotion {
class TargetString : public QObject {
  Q_OBJECT
 public:
  TargetString();

  void findMatchingPositions(QPlainTextEdit* editor, const QChar& target);

  bool isEmpty() const;
  void clear();

  void nextGroup();
  void previousGroup();
  int getGroupNum();

  int getFirstTargetIndex() const;
  int getLastTargetIndex() const;

  QPair<int, QChar> getTarget(int i) const;
  int getTargetPos(const QChar& c) const;

 private:
  static constexpr std::array<char, 52> kKeyOrder_ = {
      'j', 'f', 'k', 'd', 'l', 's', 'a', 'h', 'g', 'u', 'r', 'n', 'v', 't', 'i', 'e', 'm', 'c',
      'o', 'w', 'x', 'p', 'q', 'z', 'b', 'y', 'J', 'F', 'K', 'D', 'L', 'S', 'A', 'H', 'G', 'U',
      'R', 'N', 'V', 'T', 'I', 'E', 'M', 'C', 'O', 'W', 'X', 'P', 'Q', 'Z', 'B', 'Y'};

  int currentGroup_;
  QVector<int> targetPositions_;
};
} // namespace Qtmotion