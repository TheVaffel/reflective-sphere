#pragma once

#include "lodepng.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>

typedef struct PNGImage {
	unsigned int width, height;
	std::vector<unsigned char> pixels;
} PNGImage;

PNGImage loadPNGFile(std::string fileName);

double getTimeDeltaSeconds();

float _random();
