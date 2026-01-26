# CppShell
Интерпретатор командной строки (shell) на C++.

## Возможности
- Встроенные команды: `cat`, `echo`, `wc`, `pwd`, `exit`
- Поддержка переменных окружения и подстановок `$NAME`
- Одинарные и двойные кавычки
- Pipeline через `|`
- Запуск внешних программ

## Архитектура
Подробная архитектура и требования находятся в `docs/Architecture.md` и `docs/Requirements.md`.

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
./cppshell
```

## Примеры
```
echo "Hello, world!"
FILE=example.txt
cat $FILE
cat example.txt | wc
echo 123 | wc
```
