.PHONY: all build clean run-samples

all: build build_wasm

build:
	@echo "Building the URL safe encoder..."
	@cd cpp && mkdir -p build && cd build && cmake .. && make

build_wasm:
	@echo "Building the URL safe encoder for WebAssembly..."
	@cd cpp && ./build_wasm.sh

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf cpp/build

run-samples:
	@echo "Running sample JSON strings through encoder/decoder..."
	@./run_samples.sh