#define NOMINMAX  // do not define min/max macro

#include <ta_libc.h>

#include "core/account_controller.h"
#include "core/data_controller.h"

#include "algos/ma3_ema9.h"
#include "algos/long_low.h"

#include "adapters/polygon_io.h"  // TODO not directly include this?
#include "sim/simulator.h"  // TODO not directly include this?

