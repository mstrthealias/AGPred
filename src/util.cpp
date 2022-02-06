#include "util.h"

#include <xtensor/xindex_view.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xmanipulation.hpp>
#include <xtensor/xio.hpp>



bool _xt_2d_sort(xt::xarray<double>& a_vals, const int sort_index, const bool force)
{
	const auto indexes = xt::row(a_vals, sort_index);
	if (!force && indexes[1] > indexes[0])  // TODO this was >, does order need to be ASC or DESC?
		return false;  // skip sort

	//std::cout << "SORT" << std::endl;

	// create temporary array to hold reversed values
	xt::xarray<double> a_tmp = xt::zeros<double>(indexes.shape());

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

	return true;
}


/**
 * Returns a copy of the array, with all rows with a NAN at {index} removed.
 */
xt::xarray<double> _xt_nonans(const xt::xarray<double>& a_in, const int index)
{
	const int n_cols = a_in.shape().at(0);
	const int n_rows = a_in.shape().at(1);

	// retrieve rows which have NAN
	const auto& remove_mask = xt::isnan(xt::row(a_in, index));
	assert(n_rows == remove_mask.size());

	auto remove_indices = xt::filter(xt::arange<double>(0., n_rows, 1), remove_mask);

	// copy to a new array that does not contain NANs
	xt::xarray<double> result = xt::xarray<double>(xt::transpose(xt::view(xt::transpose(a_in), xt::drop(remove_indices))));
	assert(result.shape().at(0) == n_cols);
	assert(result.shape().at(1) == n_rows - remove_indices.size());
	return result;
}
