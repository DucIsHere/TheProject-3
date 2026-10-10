#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include ".../density/Modifier.h"
#include ".../cell/Cell.h"
#include ".../include/math/Interpolation.h"
#include ".../include/math/extern/InterporlationFast.h"
#include ".../include/math/NoiseUtil.h"

typedef struct Glacial Glacial;

struct Glacial {
    float accumulationRate;
    float meltRate;
    float viscosity;
    float pluckingProbability;
    float pluckingRate;

    float erodeSpeed;
    float depositSpeed;
    float
};
