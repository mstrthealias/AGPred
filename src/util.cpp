#include "util.h"

#include <xtensor/xindex_view.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xmanipulation.hpp>
#include <xtensor/xio.hpp>


bool _xt_check_2d_sort(const xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index, const bool force)
{
	if (force)
		return true;
	const auto indexes = xt::row(a_vals, sort_index);  // TODO fe. a_vals(sort_index, indexes[1]) ?
	return !(indexes[1] > indexes[0]);  // TODO verify this check...
}
bool _xt_check_2d_sort_2a(const xt::xarray<timestamp_us_t>& ts_vals, const ptrdiff_t& sort_index, const bool force)
{
	if (force)
		return true;
	const auto indexes = xt::row(ts_vals, sort_index);  // TODO fe. a_vals(sort_index, indexes[1]) ?
	return !(indexes[1] > indexes[0]);  // TODO verify this check...
}


void _xt_2d_sort(xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index)
{
	//std::cout << "SORT" << std::endl;

	// create temporary array to hold reversed values
	const auto indexes = xt::row(a_vals, sort_index);
	xt::xarray<real_t> a_tmp = xt::zeros<real_t>(indexes.shape());

	const size_t len = a_vals.shape().at(0);
	// assert(len == 6);

	// copy each column
	for (int i = 0; i < len; i++)
	{
		// copy into temporary array
		auto row = xt::row(a_vals, i);
		std::copy(row.crbegin(), row.crend(), a_tmp.begin());
		// copy back to a_vals
		std::copy(a_tmp.cbegin(), a_tmp.cend(), row.begin());
	}
}

void _xt_2d_sort_2a(xt::xarray<timestamp_us_t>& ts_vals, xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index)
{
	//std::cout << "SORT" << std::endl;

	auto timesteps = xt::row(ts_vals, sort_index);
	// create temporary array to hold reversed values
	xt::xarray<real_t> a_tmp = xt::zeros<real_t>(timesteps.shape());

	const size_t len = a_vals.shape().at(0);

	// reverse ts_vals
	{
		// create temporary array to hold reversed values
		xt::xarray<timestamp_us_t> ts_tmp = xt::zeros<timestamp_us_t>(timesteps.shape());
		// copy into temporary array
		std::copy(timesteps.crbegin(), timesteps.crend(), ts_tmp.begin());
		// copy back to ts_vals
		std::copy(ts_tmp.cbegin(), ts_tmp.cend(), timesteps.begin());
	}

	// reverse each column in a_vals
	for (int i = 0; i < len; i++)
	{
		// copy into temporary array
		auto row = xt::row(a_vals, i);
		std::copy(row.crbegin(), row.crend(), a_tmp.begin());
		// copy back to a_vals
		std::copy(a_tmp.cbegin(), a_tmp.cend(), row.begin());
	}
}


bool _xt_2d_sort(xt::xarray<real_t>& a_vals, const ptrdiff_t& sort_index, const bool force)
{
	if (!_xt_check_2d_sort(a_vals, sort_index, force))
		return false;  // skip sort
	_xt_2d_sort(a_vals, sort_index);
	return true;
}


/**
 * Returns a copy of the array, with all rows with a NAN at {index} removed.
 */
xt::xarray<real_t> _xt_nonans(const xt::xarray<real_t>& a_in, const ptrdiff_t& index)
{
	const int n_cols = a_in.shape().at(0);
	const int n_rows = a_in.shape().at(1);

	// retrieve rows which have NAN
	const auto& remove_mask = xt::isnan(xt::row(a_in, index));
	assert(n_rows == remove_mask.size());

	auto remove_indices = xt::filter(xt::arange<real_t>(0., n_rows, 1), remove_mask);

	// copy to a new array that does not contain NANs
	xt::xarray<real_t> result = xt::xarray<real_t>(xt::transpose(xt::view(xt::transpose(a_in), xt::drop(remove_indices))));
	assert(result.shape().at(0) == n_cols);
	assert(result.shape().at(1) == n_rows - remove_indices.size());
	return result;
}

/**
 * Returns a copy of the array, with all rows with a NAN at {index} removed.
 */
void _xt_nonans_2a(xt::xarray<timestamp_us_t>& ts_in, xt::xarray<real_t>& a_in, const ptrdiff_t& index)
{
	const int n_cols = a_in.shape().at(0);
	const int n_rows = a_in.shape().at(1);

	// retrieve rows which have NAN
	const auto& remove_mask = xt::isnan(xt::row(a_in, index));
	assert(n_rows == remove_mask.size());

	auto remove_indices = xt::filter(xt::arange<real_t>(0., n_rows, 1), remove_mask);

	// copy to a new array that does not contain NANs
	a_in = xt::xarray<real_t>(xt::transpose(xt::view(xt::transpose(a_in), xt::drop(remove_indices))));
	assert(result.shape().at(0) == n_cols);
	assert(result.shape().at(1) == n_rows - remove_indices.size());

	const int n_ts_cols = ts_in.shape().at(0);
	const int n_ts_rows = ts_in.shape().at(1);

	ts_in = xt::xarray<timestamp_us_t>(xt::transpose(xt::view(xt::transpose(ts_in), xt::drop(remove_indices))));
	assert(result.shape().at(0) == n_ts_cols);
	assert(result.shape().at(1) == n_ts_rows - remove_indices.size());

}
