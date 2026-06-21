.PHONY: native-debug
native-debug:
	cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
		-S . -B ./build/native/DEBUG \
		-DCMAKE_BUILD_TYPE=DEBUG \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build ./build/native/DEBUG --parallel $$(nproc)

.PHONY: native-release
native-release:
	cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
		-S . -B ./build/native/RELEASE \
		-DCMAKE_BUILD_TYPE=RELEASE \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build ./build/native/RELEASE --parallel $$(nproc)

.PHONY: wasm-debug
wasm-debug:
	emcmake cmake \
		-S . -B ./build/wasm/DEBUG \
		-DCMAKE_BUILD_TYPE=DEBUG \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build ./build/wasm/DEBUG --parallel $$(nproc)
	cp ./assets/* ./build/wasm/DEBUG/
	npm pack ./build/wasm/DEBUG \
		--cache ./build/.npm \
		--pack-destination ./build/wasm/DEBUG

.PHONY: wasm-release
wasm-release:
	emcmake cmake \
		-S . -B ./build/wasm/RELEASE \
		-DCMAKE_BUILD_TYPE=RELEASE \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build ./build/wasm/RELEASE --parallel $$(nproc)
	cp ./assets/* ./build/wasm/RELEASE/
	npm pack ./build/wasm/RELEASE \
		--cache ./build/.npm \
		--pack-destination ./build/wasm/RELEASE

.PHONY: test
test: native-debug
	cd ./build/native/DEBUG && ctest
