#include "wasmMVG.h"

#ifdef __EMSCRIPTEN__

EMSCRIPTEN_BINDINGS(wasm_mvg) {
	emscripten::function("getRelativePose", &getRelativePoseJs);
	emscripten::function("getPose", &getPoseJs);
	emscripten::function("triangulation", &triangulationJs);
	emscripten::function("bundleAdjustment", &bundleAdjustmentJs);
}

#else

int main() { return 0; }

#endif
