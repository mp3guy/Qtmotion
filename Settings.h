#pragma once

#include <string>

namespace Qtmotion {

class Settings {
 public:
  Settings();

  bool passFilename() const;
  void passFilename(const bool value);

  const std::string& command() const;
  void command(const std::string& value);

  void Load();
  void Save();

  bool operator==(const Settings& other) const;
  bool operator!=(const Settings& other) const;

 private:
  bool passFilename_ = false;
  std::string command_;
};

inline bool Settings::passFilename() const {
  return passFilename_;
}
inline void Settings::passFilename(const bool value) {
  passFilename_ = value;
}

inline const std::string& Settings::command() const {
  return command_;
}

inline void Settings::command(const std::string& value) {
  command_ = value;
}

inline bool Settings::operator==(const Settings& other) const {
  return passFilename_ == other.passFilename_ && command_ == other.command_;
}

inline bool Settings::operator!=(const Settings& other) const {
  return !(*this == other);
}

} // namespace Qtmotion
