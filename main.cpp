#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE

#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <cassert>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "examples/main_SfMInit_ImageListing.h"
#include "examples/main_ComputeFeatures.h"
#include "examples/main_ComputeMatches.h"
#include "examples/main_GeometricFilter.h"
#include "examples/main_SfM.h"

EXPORT int sfm() {
	char* opts1[9] = {
		"./main_SfMInit_ImageListing",
		"-i", "/home/web_user/images",
		"-o", "/home/web_user/matches",
		"-c", "3",
		"-f", "3398"
	};
	main_SfMInit_ImageListing(9, opts1);
	char* opts2[9] = {
		"./main_ComputeFeatures",
		"-i", "/home/web_user/matches/sfm_data.json",
		"-o", "/home/web_user/matches",
		"-m", "AKAZE_FLOAT",
		"-f", "1"
	};
	main_ComputeFeatures(9, opts2);
	char* opts3[9] = {
		"./main_ComputeMatches",
		"-i", "/home/web_user/matches/sfm_data.json",
		"-o", "/home/web_user/matches/matches.putative.bin",
		"-f", "1",
		"-n", "ANNL2"
	};
	main_ComputeMatches(9, opts3);
	char* opts4[9] = {
		"./main_GeometricFilter",
		"-i", "/home/web_user/matches/sfm_data.json",
		"-m", "/home/web_user/matches/matches.putative.bin",
		"-g", "f",
		"-o", "/home/web_user/matches/matches.f.bin"
	};
	main_GeometricFilter(9, opts4);
	char* opts5[9] = {
		"./main_SfM",
		"-s", "INCREMENTAL",
		"-i", "/home/web_user/matches/sfm_data.json",
		"-m", "/home/web_user/matches",
		"-o", "/home/web_user/sparse_model"
	};
	main_SfM(9, opts5);
	return 0;
}

EXPORT bool save(const char *path, const char *const uint8array, const int length) {
	assert(length >= 0);
	const std::vector<char> bytes{ uint8array, uint8array + length };
	std::ofstream outputFile(path, std::ios::out | std::ios::binary);
	if (!outputFile.good()) return false;
	outputFile.write(&bytes[0], length);
	return outputFile.good();
}

EXPORT int load(const char *path, char *const uint8array) {
	std::ifstream inputFile(path);
	if (!inputFile.good()) { return -1; }
	const std::vector<char> bytes{
		std::istreambuf_iterator<char>(inputFile),
		std::istreambuf_iterator<char>()
	};
	if (uint8array) {
		std::memcpy(uint8array, bytes.data(), bytes.size());
	}
	return bytes.size();
}

EXPORT bool ls(const char *path) {
	DIR *dir;
	struct dirent *dp;
	dir = opendir(path);
	if (dir == NULL) { return false; }
	dp = readdir(dir);
	while (dp != NULL) {
		printf("%s\n", dp->d_name);
		dp = readdir(dir);
	}
	if (dir != NULL) { closedir(dir); }
	return true;
}

EXPORT int makedir(const char *path) {
	return mkdir(path, 0755);
}
#endif
