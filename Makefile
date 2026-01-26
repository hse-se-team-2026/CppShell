all: build

build:
	cmake -S . -B build
	cmake --build build

run: build
	./bin/cppshell

clean:
	cmake --build build --target clean || true
	rm -rf build

