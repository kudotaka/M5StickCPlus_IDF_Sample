#pragma once

#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
#include "sht3x.h"
#endif

#if CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT
#include "sk6812.h"
#endif

#if ( CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
#include "tm1637.h"

#ifdef CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT
#define DIGIT_COUNT 4
#elif CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT
#define DIGIT_COUNT 6
#else
#define DIGIT_COUNT 1
#endif

#endif
