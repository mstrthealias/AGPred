#ifndef UTIL_H
#define UTIL_H


#include <xtensor/xarray.hpp>

#include "common.h"


bool _xt_check_2d_sort(const xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index, const bool force = false);
bool _xt_check_2d_sort_2a(const xt::xarray<timestamp_us_t>& ts_vals, const ptrdiff_t& sort_index, const bool force = false);

void _xt_2d_sort(xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index);
void _xt_2d_sort_2a(xt::xarray<timestamp_us_t>& ts_vals, xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index);

bool _xt_2d_sort(xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index, const bool force);

xt::xarray<real_t> _xt_nonans(const xt::xarray<real_t>& a_in, const ptrdiff_t& index);
void _xt_nonans_2a(xt::xarray<timestamp_us_t>& ts_in, xt::xarray<real_t>& a_in, const ptrdiff_t& index);


#endif // UTIL_H

