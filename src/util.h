#ifndef UTIL_H
#define UTIL_H


#include <xtensor/xarray.hpp>

//#include "common.h"


bool _xt_2d_sort(xt::xarray<double>& a_vals, const int sort_index, const bool force = false);

xt::xarray<double> _xt_nonans(const xt::xarray<double>& a_in, const int index);


#endif // UTIL_H

