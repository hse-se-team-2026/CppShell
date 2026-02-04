# CppShell
[![CI](https://github.com/hse-se-team-2026/CppShell/actions/workflows/ci.yml/badge.svg)](https://github.com/hse-se-team-2026/CppShell/actions/workflows/ci.yml)
[![CodeQL](https://github.com/hse-se-team-2026/CppShell/actions/workflows/codeql.yml/badge.svg)](https://github.com/hse-se-team-2026/CppShell/actions/workflows/codeql.yml)
Интерпретатор командной строки (shell) на C++.
Выполнено командой студентов Высшей школы экономики в Санкт-Петербурге программы Прикладная математика и информатика в рамках курса SE.

## Возможности
- Встроенные команды: `cat`, `echo`, `wc`, `pwd`, `exit`
- Поддержка переменных окружения (снимок окружения процесса) и присваиваний `NAME=value`
- Одинарные и двойные кавычки (строка в кавычках = один аргумент)
- Запуск внешних программ

## Требования
- C++23
- clang-format-20
- YAML-конфиг `config.yml` для расширяемости (опционально)
- clang-tidy-20

## Архитектура
Подробная архитектура и требования находятся в `docs/Architecture.md` и `docs/Requirements.md`.

## Участники
- Дмитрий Русанов — https://github.com/DimaRus05
- Усатов Павел — https://github.com/UsatovPavel

## Сборка
Проект собирается из консоли через CMake.

Linux/macOS (single-config генераторы):
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows (Visual Studio Build Tools, multi-config):
```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Примечание: можно использовать Ninja (в т.ч. Multi-Config), если он установлен.

## Запуск
Linux/macOS:
```
./bin/cppshell
```

Windows:
```
./bin/Debug/cppshell.exe
```

## Примеры
```
echo "Hello, world!"
FILE=example.txt
cat "example.txt"
wc "example.txt"
pwd
exit 0
```
