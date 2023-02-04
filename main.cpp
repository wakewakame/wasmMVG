#include <iostream>
#include <chrono>
#include <string.h>
#include "examples/main_SfMInit_ImageListing.h"
#include "examples/main_ComputeFeatures.h"
#include "examples/main_ComputeMatches.h"
#include "examples/main_GeometricFilter.h"
#include "examples/main_SfM.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT extern "C"
#endif

EXPORT int main(int argc, char **argv) {
	const std::string path_root{argv[1]};

	const std::string path_images = path_root + "/images";
	const std::string path_matches = path_root + "/matches";
	char* opts1[9] = {
		strdup("./main_SfMInit_ImageListing"),
		strdup("-i"), strdup(path_images.c_str()),
		strdup("-o"), strdup(path_matches.c_str()),
		strdup("-c"), strdup("3"),
		strdup("-f"), strdup("3398")
	};
	main_SfMInit_ImageListing(9, opts1);

	const std::chrono::system_clock::time_point time0 = std::chrono::system_clock::now();

	const std::string path_matches_sfm_data = path_matches + "/sfm_data.json";
	char* opts2[9] = {
		strdup("./main_ComputeFeatures"),
		strdup("-i"), strdup(path_matches_sfm_data.c_str()),
		strdup("-o"), strdup(path_matches.c_str()),
		strdup("-m"), strdup("AKAZE_FLOAT"),
		strdup("-f"), strdup("1")
	};
	main_ComputeFeatures(9, opts2);

	const std::chrono::system_clock::time_point time1 = std::chrono::system_clock::now();

	const std::string path_matches_matches_putative = path_matches + "/matches.putative.bin";
	char* opts3[9] = {
		strdup("./main_ComputeMatches"),
		strdup("-i"), strdup(path_matches_sfm_data.c_str()),
		strdup("-o"), strdup(path_matches_matches_putative.c_str()),
		strdup("-f"), strdup("1"),
		strdup("-n"), strdup("ANNL2")
	};
	main_ComputeMatches(9, opts3);

	const std::chrono::system_clock::time_point time2 = std::chrono::system_clock::now();

	const std::string path_matches_matches_f = path_matches + "/matches.f.bin";
	char* opts4[9] = {
		strdup("./main_GeometricFilter"),
		strdup("-i"), strdup(path_matches_sfm_data.c_str()),
		strdup("-m"), strdup(path_matches_matches_putative.c_str()),
		strdup("-g"), strdup("f"),
		strdup("-o"), strdup(path_matches_matches_f.c_str())
	};
	main_GeometricFilter(9, opts4);

	const std::chrono::system_clock::time_point time3 = std::chrono::system_clock::now();

	const std::string path_sparse_model = path_root + "/sparse_model";
	char* opts5[9] = {
		strdup("./main_SfM"),
		strdup("-s"), strdup("INCREMENTAL"),
		strdup("-i"), strdup(path_matches_sfm_data.c_str()),
		strdup("-m"), strdup(path_matches.c_str()),
		strdup("-o"), strdup(path_sparse_model.c_str())
	};
	main_SfM(9, opts5);

	const std::chrono::system_clock::time_point time4 = std::chrono::system_clock::now();

	std::cout
		<< "ComputeFeatures: " << std::chrono::duration_cast<std::chrono::seconds>(time1 - time0).count() << " s\n"
		<< "ComputeMatches : " << std::chrono::duration_cast<std::chrono::seconds>(time2 - time1).count() << " s\n"
		<< "GeometricFilter: " << std::chrono::duration_cast<std::chrono::seconds>(time3 - time2).count() << " s\n"
		<< "SfM            : " << std::chrono::duration_cast<std::chrono::seconds>(time4 - time3).count() << " s\n"
		<< "========================\n"
		<< "Total          : " << std::chrono::duration_cast<std::chrono::seconds>(time4 - time0).count() << " s"
		<< std::endl;

	return 0;
}
