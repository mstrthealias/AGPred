#include "util.h"

#include <set>
#include <tuple>
#include <xtensor/xindex_view.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xmanipulation.hpp>
#include <xtensor/xio.hpp>



const std::set<std::tuple<const int, const int, const int>> HOLIDAYS = {
	std::make_tuple<>(2021, 1, 1),
	std::make_tuple<>(2021, 1, 18),
	std::make_tuple<>(2021, 2, 15),
	std::make_tuple<>(2021, 5, 31),
	std::make_tuple<>(2021, 7, 4),
	std::make_tuple<>(2021, 9, 6),
	std::make_tuple<>(2021, 10, 11),
	std::make_tuple<>(2021, 11, 11),
	std::make_tuple<>(2021, 11, 25),
	std::make_tuple<>(2021, 12, 24),
	std::make_tuple<>(2021, 12, 25),
	std::make_tuple<>(2022, 1, 1),
	std::make_tuple<>(2022, 1, 17),
	std::make_tuple<>(2022, 2, 21),

	// these days fail to retreive data from the API:
	std::make_tuple<>(2022, 3, 2),
};


/*
* zellersAlgorithm:
*   0=Saturday, 1=Sunday, ..., 6=Friday
*/
int day_of_week(int day, int month, int year)
{
	int mon;
	if (month > 2)
		mon = month; //for march to december month code is same as month
	else {
		mon = (12 + month); //for Jan and Feb, month code will be 13 and 14
		year--; //decrease year for month Jan and Feb
	}
	int y = year % 100; //last two digit
	int c = year / 100; //first two digit
	int w = (day + floor((13 * (mon + 1)) / 5) + y + floor(y / 4) + floor(c / 4) + (5 * c));
	return w % 7;
}


bool is_trading_day(int day, int month, int year)
{
	int dow = day_of_week(day, month, year);
	if (dow <= 1)
		return false;
	// true of not a holiday...
	return HOLIDAYS.find(std::make_tuple<>(year, month, day)) == HOLIDAYS.end();
}

bool is_trading_day(std::chrono::system_clock::time_point tp)
{
	using namespace std::chrono;
	time_t tt = system_clock::to_time_t(tp);
	tm local_tm;
	localtime_s(&local_tm, &tt);

	return is_trading_day(local_tm.tm_mday, local_tm.tm_mon + 1, local_tm.tm_year + 1900);
}

constexpr float F_MAX = std::numeric_limits<float>::max();
constexpr float F_MIN = std::numeric_limits<float>::min();

real_t cleanup_float_errs(real_t val)
{
	if (val < -1e13 || val > 1e13)
		return 0.0f;
	//if (val >= F_MAX || val <= F_MIN)  // Note: this does not function correctly (fe. removes -1.51231)
	//	return 0.0f;
	else
		return val;
}


std::chrono::system_clock::time_point to_time_point(int year, int mon, int day, int hour, int min, int is_dst)
{
	using namespace std::chrono;
	std::tm tm{};  // zero initialise
	tm.tm_year = year - 1900;
	tm.tm_mon = mon - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_isdst = is_dst ? -1 : 1;
	std::time_t tt = std::mktime(&tm);
	return system_clock::from_time_t(tt);
}



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
