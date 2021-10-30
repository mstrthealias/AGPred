#ifndef COMMON_H
#define COMMON_H


#include <iostream>
#include <map>
#include <vector>
#include <limits>

#include <xtensor/xarray.hpp>


using timestamp_t = size_t;


using dfs_map_t = std::map<int, xt::xarray<double>>;

using interval_map_t = std::map<const char*, const int>;


std::ostream& operator<< (std::ostream& out, const xt::svector<size_t>& s);
std::ostream& operator<< (std::ostream& out, const std::vector<int>& s);
std::ostream& operator<< (std::ostream& out, const std::vector<double>& v);
std::ostream& operator<< (std::ostream& out, const std::vector<std::vector<size_t>>& v);

/*
enum Timeframe : uint32_t
{
	MIN1 = 1,
	MIN5 = 5,
	MIN15 = 15,
	HR1 = 60,
	HR4 = 240,
	DAY = 1440,
	WEEK = 10080,
};*/


template <typename T>
bool dbl_equal(T a, T b) {
    return std::fabs(a - b) < std::numeric_limits<T>::epsilon();
}


template <size_t Size>
std::ostream& operator<< (std::ostream& out, const std::array<size_t, Size>& v) {
    out << '(';  // (std::is_floating_point<T>::value ? '[' : '(');
    if (!v.empty()) {
        const auto vSize = v.size();
        for (typename std::array<size_t, Size>::size_type i = 0; i < vSize; ++i)
            std::cout << v[i] << (i == vSize - 1 ? "" : ", ");
    }
    out << ')';  // (std::is_floating_point<T>::value ? ']' : ')');
    return out;
}



// TODO track NBBO separate from Bar??
template <typename Real, typename Size, typename Time>
struct nbbo_t
{
	const Time& timestamp;
	const Real& bid;
	const Real& ask;
	const Size& bid_size;
	const Size& ask_size;
};

typedef nbbo_t<double, uint64_t, double> NBBO;


// TODO 32bit Size??

template <typename Real, typename Volume, typename Time, typename Size>
struct bar_t
{
	Time timestamp = 0;
	Real open = 0.0;
	Real high = 0.0;
	Real low = 0.0;
	Real close = 0.0;
	Volume volume = 0;

	Real bid = 0.0;
	Real bid_high = 0.0;
	Real bid_low = 0.0;

	Real ask = 0.0;
	Real ask_high = 0.0;
	Real ask_low = 0.0;

	Size bid_size = 0;
	Size ask_size = 0;

	void zero()
	{
		memset(this, 0, sizeof *this);
	}

	[[nodiscard]] NBBO nbbo() const
	{
		return { timestamp, bid, ask, bid_size, ask_size };
	}
};

typedef bar_t<double, uint64_t, double, uint32_t> Bar;

std::ostream& operator<< (std::ostream& out, const Bar& bar);


class Timeframes
{
public:
	enum Pos : ptrdiff_t
	{
		_1min = 0,
		_5min = 1,
		_15min = 2,
		_1hr = 3,
		_4hr = 4,
		_1day = 5,
		_1wk = 6,
	};

	/*
	enum class Min : ptrdiff_t
	{
		_1min = 1,
		_5min = 5,
		_15min = 15,
		_1hr = 60,
		_4hr = 240,
		_1day = 1440,
		_1wk = 10080,
	};*/
};


enum class ColPosType
{
	In,
	Candle,
	Dep,
	Regr,
	Diff,
	TA,
	Norm
};


class ColPos
{
public:
	enum In : ptrdiff_t
	{
		timestamp = 0,
		open = 1,
		high = 2,
		low = 3,
		close = 4,
		volume = 5,

		// TODO here?
		// TODO this order?
		ask_size = 6,
		bid_size = 7,
		ask = 8,
		ask_high = 9,
		ask_low = 10,
		bid = 11,
		bid_high = 12,
		bid_low = 13,
		// TODO fe. Dep starts at 14?
	};
	enum Dep : ptrdiff_t
	{
		hlc3 = 6,
		range = 7,
		atr = 8,
		past_range = 9,
	};
	enum Regr : ptrdiff_t
	{
		regr = 10,
		stddev = 11,
	};
	enum Diff : ptrdiff_t
	{
		diff = 12,
		range_t = 13,
		range_b = 14,
		range_pos = 15,
	};
	enum TA : ptrdiff_t
	{
		trend5 = 16,
		trend9 = 17,
		trend20 = 18,
		trend50 = 19,
		volume_buy = 20,
		volume_sell = 21,
		volume_percent_buy = 22,
		volume_percent_sell = 23,
		obv = 24,
		obv_trend20 = 25,
		obv_trend50 = 26,
		adi_trend20 = 27,
		adi_trend50 = 28,
		adi_obv_ratio = 29,
		rsi = 30,
		rsi_indicator = 31,
		rsi_trend20 = 32,
		rsi_trend50 = 33,
		ppo = 34,
		ppo_diff = 35,
		adx = 36,
		adx_indicator = 37,
	};
	enum Norm : ptrdiff_t
	{
		close_ln = 38,
		close_next_norm = 39,
	};
	enum Candle : ptrdiff_t
	{
		// 61 cols
		_2CROWS = 40,
		_3BLACKCROWS,
		_3INSIDE,
		_3LINESTRIKE,
		_3OUTSIDE,
		_3STARSINSOUTH,
		_3WHITESOLDIERS,
		_ADVANCEBLOCK,
		_BELTHOLD,
		_BREAKAWAY,
		_CLOSINGMARUBOZU,
		_CONCEALBABYSWALL,
		_COUNTERATTACK,
		_DOJI,
		_DOJISTAR,
		_DRAGONFLYDOJI,
		_ENGULFING,
		_GAPSIDESIDEWHITE,
		_GRAVESTONEDOJI,
		_HAMMER,
		_HANGINGMAN,
		_HARAMI,
		_HARAMICROSS,
		_HIGHWAVE,
		_HIKKAKE,
		_HIKKAKEMOD,
		_HOMINGPIGEON,
		_IDENTICAL3CROWS,
		_INNECK,
		_INVERTEDHAMMER,
		_KICKING,
		_KICKINGBYLENGTH,
		_LADDERBOTTOM,
		_LONGLEGGEDDOJI,
		_LONGLINE,
		_MARUBOZU,
		_MATCHINGLOW,
		_ONNECK,
		_PIERCING,
		_RICKSHAWMAN,
		_RISEFALL3METHODS,
		_SEPARATINGLINES,
		_SHOOTINGSTAR,
		_SHORTLINE,
		_SPINNINGTOP,
		_STALLEDPATTERN,
		_STICKSANDWICH,
		_TAKURI,
		_TASUKIGAP,
		_THRUSTING,
		_TRISTAR,
		_UNIQUE3RIVER,
		_UPSIDEGAP2CROWS,
		_XSIDEGAP3METHODS,
		_ABANDONEDBABY,
		_DARKCLOUDCOVER,
		_EVENINGDOJISTAR,
		_EVENINGSTAR,
		_MATHOLD,
		_MORNINGDOJISTAR,
		_MORNINGSTAR,  // 100
	};
};


/*
class ColPos
{
public:
	enum In : ptrdiff_t
	{
		timestamp = 0,
		open = 0,
		high = 1,
		low = 2,
		close = 3,
		volume = 4,
	};
	enum Dep : ptrdiff_t
	{
		hlc3 = 0,
		range = 1,
		atr = 2,
		past_range = 3,
	};
	enum Regr : ptrdiff_t
	{
		regr = 0,
		stddev = 1,
	};
	enum Diff : ptrdiff_t
	{
		diff = 0,
		range_t = 1,
		range_b = 2,
		range_pos = 3,
	};
	enum TA : ptrdiff_t
	{
		trend5 = 0,
		trend9 = 1,
		trend20 = 2,
		trend50 = 3,
		volume_buy = 4,
		volume_sell = 5,
		volume_percent_buy = 6,
		volume_percent_sell = 7,
		obv = 8,
		obv_trend20 = 9,
		obv_trend50 = 10,
		adi_trend20 = 11,
		adi_trend50 = 12,
		adi_obv_ratio = 13,
		rsi = 14,
		rsi_indicator = 15,
		rsi_trend20 = 16,
		rsi_trend50 = 17,
		ppo = 18,
		ppo_diff = 19,
		adx = 20,
		adx_indicator = 21,
	};
	enum Norm : ptrdiff_t
	{
		close_ln = 0,
		close_next_norm = 1,
	};
	enum Candle : ptrdiff_t
	{
		// 61 cols
		_2CROWS,
		_3BLACKCROWS,
		_3INSIDE,
		_3LINESTRIKE,
		_3OUTSIDE,
		_3STARSINSOUTH,
		_3WHITESOLDIERS,
		_ADVANCEBLOCK,
		_BELTHOLD,
		_BREAKAWAY,
		_CLOSINGMARUBOZU,
		_CONCEALBABYSWALL,
		_COUNTERATTACK,
		_DOJI,
		_DOJISTAR,
		_DRAGONFLYDOJI,
		_ENGULFING,
		_GAPSIDESIDEWHITE,
		_GRAVESTONEDOJI,
		_HAMMER,
		_HANGINGMAN,
		_HARAMI,
		_HARAMICROSS,
		_HIGHWAVE,
		_HIKKAKE,
		_HIKKAKEMOD,
		_HOMINGPIGEON,
		_IDENTICAL3CROWS,
		_INNECK,
		_INVERTEDHAMMER,
		_KICKING,
		_KICKINGBYLENGTH,
		_LADDERBOTTOM,
		_LONGLEGGEDDOJI,
		_LONGLINE,
		_MARUBOZU,
		_MATCHINGLOW,
		_ONNECK,
		_PIERCING,
		_RICKSHAWMAN,
		_RISEFALL3METHODS,
		_SEPARATINGLINES,
		_SHOOTINGSTAR,
		_SHORTLINE,
		_SPINNINGTOP,
		_STALLEDPATTERN,
		_STICKSANDWICH,
		_TAKURI,
		_TASUKIGAP,
		_THRUSTING,
		_TRISTAR,
		_UNIQUE3RIVER,
		_UPSIDEGAP2CROWS,
		_XSIDEGAP3METHODS,
		_ABANDONEDBABY,
		_DARKCLOUDCOVER,
		_EVENINGDOJISTAR,
		_EVENINGSTAR,
		_MATHOLD,
		_MORNINGDOJISTAR,
		_MORNINGSTAR,
	};
};
*/


#endif // COMMON_H
