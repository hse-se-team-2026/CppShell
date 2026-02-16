#include "cppshell/environment.hpp"

#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <vector>
#endif

#ifndef _WIN32
extern char **environ;
#endif

namespace cppshell {

namespace {

#ifdef _WIN32
std::string WideToUtf8(const std::wstring &value) {
  if (value.empty()) {
    return {};
  }

  const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr,
                                       0, nullptr, nullptr);
  if (size <= 0) {
    return {};
  }

  std::string out(static_cast<size_t>(size - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, out.data(), size, nullptr,
                      nullptr);
  return out;
}

std::wstring Utf8ToWide(const std::string &value) {
  if (value.empty()) {
    return {};
  }

  const int size =
      MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
  if (size <= 0) {
    return {};
  }

  std::wstring out(static_cast<size_t>(size - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(), size);
  return out;
}
#endif

} // namespace

Environment::Environment() {
#ifdef _WIN32
  LPWCH block = GetEnvironmentStringsW();
  if (block == nullptr) {
    return;
  }

  // block: sequence of null-terminated strings, terminated by extra null.
  for (LPWCH p = block; *p != L'\0';) {
    std::wstring entry(p);
    p += entry.size() + 1;

    // Skip hidden variables like "=C:".
    const size_t eq = entry.find(L'=');
    if (eq == std::wstring::npos || eq == 0 || entry[0] == L'=') {
      continue;
    }

    const std::wstring name_w = entry.substr(0, eq);
    const std::wstring value_w = entry.substr(eq + 1);
    vars_.emplace(WideToUtf8(name_w), WideToUtf8(value_w));
  }

  FreeEnvironmentStringsW(block);
#else
  if (environ == nullptr) {
    return;
  }

  for (char **p = environ; *p != nullptr; ++p) {
    const std::string entry(*p);
    const size_t eq = entry.find('=');
    if (eq == std::string::npos || eq == 0) {
      continue;
    }
    vars_.emplace(entry.substr(0, eq), entry.substr(eq + 1));
  }
#endif
}

void Environment::Set(const std::string &name, const std::string &value) {
  vars_[name] = value;
}

std::string Environment::Get(const std::string &name) const {
  auto it = vars_.find(name);
  if (it != vars_.end()) {
    return it->second;
  }
  return {};
}

Environment Environment::WithOverrides(
    const std::unordered_map<std::string, std::string> &overrides) const {
  Environment derived = *this;
  for (const auto &[name, value] : overrides) {
    derived.Set(name, value);
  }
  return derived;
}

std::vector<std::string> Environment::ToEnvStrings() const {
  std::vector<std::string> out;
  out.reserve(vars_.size());

  for (const auto &[name, value] : vars_) {
    out.push_back(name + "=" + value);
  }

  // POSIX exec expects environment often sorted, but does not require it.
  std::sort(out.begin(), out.end());
  return out;
}

#ifdef _WIN32
std::wstring Environment::ToWindowsEnvironmentBlock() const {
  // Windows expects environment block sorted case-insensitively by name.
  std::vector<std::wstring> entries;
  entries.reserve(vars_.size());

  for (const auto &[name, value] : vars_) {
    entries.push_back(Utf8ToWide(name) + L"=" + Utf8ToWide(value));
  }

  std::sort(entries.begin(), entries.end(),
            [](const std::wstring &a, const std::wstring &b) {
              // Compare only by name part.
              const size_t aeq = a.find(L'=');
              const size_t beq = b.find(L'=');
              const std::wstring an = a.substr(0, aeq);
              const std::wstring bn = b.substr(0, beq);
              const int cmp =
                  CompareStringOrdinal(an.c_str(), -1, bn.c_str(), -1, TRUE);
              return cmp == CSTR_LESS_THAN;
            });

  std::wstring block;
  for (const auto &e : entries) {
    block.append(e);
    block.push_back(L'\0');
  }
  block.push_back(L'\0');
  return block;
}
#endif

} // namespace cppshell
