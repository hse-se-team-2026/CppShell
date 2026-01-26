# CppShell
[![CI](https://github.com/hse-se-team-2026/CppShell/actions/workflows/ci.yml/badge.svg)](https://github.com/hse-se-team-2026/CppShell/actions/workflows/ci.yml)
[![CodeQL](https://github.com/hse-se-team-2026/CppShell/actions/workflows/codeql.yml/badge.svg)](https://github.com/hse-se-team-2026/CppShell/actions/workflows/codeql.yml)
Интерпретатор командной строки (shell) на C++.
Выполнено командой студентов Высшей школы экономики в Санкт-Петербурге программы Прикладная математика и информатика в рамках курса SE.

## Возможности
- Встроенные команды: `cat`, `echo`, `wc`, `pwd`, `exit`
- Поддержка переменных окружения и подстановок `$NAME`
- Одинарные и двойные кавычки
- Pipeline через `|`
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
Если используется CMake:
```
cmake -S . -B build
cmake --build build
```

Если используется Makefile:
```
make
```

## Запуск
```
./bin/cppshell
```

## Примеры
```
echo "Hello, world!"
FILE=example.txt
cat $FILE
cat example.txt | wc
echo 123 | wc
```
