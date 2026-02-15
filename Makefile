all: build

format:
	clang-format-20 -i $(shell find . -path './test-data' -prune -or \( -iname '*.c' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.hpp' \) -print)

build: format
	cmake -S . -B build
	cmake --build build

# Better two times
test: build
	ctest --test-dir build --verbose
	./bin/cppshell_tests
	./tests/integration_tests.sh

run: build
	./bin/cppshell

clean:
	cmake --build build --target clean || true
	rm -rf build
