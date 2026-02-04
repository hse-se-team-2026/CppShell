#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <string_view>
#endif

namespace cppshell {

/**
 * A snapshot-like environment store.
 *
 * The shell keeps a base environment and can create derived environments with
 * per-command overrides. Derived environments are used when spawning external
 * processes.
 */
class Environment {
 public:
  /** Captures the current process environment into an internal map. */
  Environment();

  /** Returns a variable value, if present. */
  [[nodiscard]] std::optional<std::string> Get(const std::string& name) const;

  /** Sets or overwrites a variable in this environment map. */
  void Set(const std::string& name, const std::string& value);

  /** Removes a variable from this environment map. */
  void Unset(const std::string& name);

  /** Returns a copy of this environment with overrides applied. */
  [[nodiscard]] Environment WithOverrides(
      const std::unordered_map<std::string, std::string>& overrides) const;

  /** Returns environment as a list of strings `NAME=VALUE` suitable for exec. */
  [[nodiscard]] std::vector<std::string> ToEnvStrings() const;

#ifdef _WIN32
  /**
   * Returns a Windows environment block for CreateProcessW.
   *
   * The block is a sequence of null-terminated wide strings `NAME=VALUE`,
   * terminated by an additional null character.
   */
  [[nodiscard]] std::wstring ToWindowsEnvironmentBlock() const;
#endif

 private:
  std::unordered_map<std::string, std::string> vars_;
};

}  // namespace cppshell
