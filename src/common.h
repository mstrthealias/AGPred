#ifndef COMMON_H
#define COMMON_H


#include <iostream>
#include <map>
#include <vector>
#include <limits>

#include <xtensor/xarray.hpp>


constexpr bool DEBUG_PRINT_DATA = false;
constexpr bool DEBUG_PRINT_PROCESSED_DATA = false;
constexpr bool DEBUG_PRINT_REQUESTS = true;
constexpr bool DEBUG_ORDERS = false;


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



enum class TradeCondition : uint32_t
{
	//Invalid = -1,  // using PLACEHOLDER instead
	Regular_Sale = 0,
	Acquisition = 1,
	Average_Price_Trade = 2,
	Automatic_Execution = 3,
	Bunched_Trade = 4,
	Bunched_Sold_Trade = 5,
	CAP_Election = 6,
	Cash_Sale = 7,
	Closing_Prints = 8,
	Cross_Trade = 9,
	Derivatively_Priced = 10,
	Distribution = 11,
	Form_T_Extended_Hours = 12,
	Extended_Hours_Sold_Out_Of_Sequence = 13,
	Intermarket_Sweep = 14,
	Market_Center_Official_Close = 15,
	Market_Center_Official_Open = 16,
	Market_Center_Opening_Trade = 17,
	Market_Center_Reopening_Trade = 18,
	Market_Center_Closing_Trade = 19,
	Next_Day = 20,
	Price_Variation_Trade = 21,
	Prior_Reference_Price = 22,
	Rule_155_Trade_AMEX = 23,
	Rule_127_NYSE_Only = 24,
	Opening_Prints = 25,
	Stopped_Stock_Regular_Trade = 27,
	ReOpening_Prints = 28,
	Seller = 29,
	Sold_Last = 30,
	Sold_Last_and_Stopped_Stock = 31,
	Sold_Out_Of_Sequence = 32,
	Sold_Out_of_Sequence_and_Stopped_Stock = 33,
	Split_Trade = 34,
	Stock_Option = 35,
	Yellow_Flag_Regular_Trade = 36,
	Odd_Lot_Trade = 37,
	Corrected_Consolidated_Close_per_listing_market = 38,
	Held = 40,
	Trade_Thru_Exempt = 41,
	PLACEHOLDER = 49,  // PLACEHOLDER use for trade_condition_t[] placeholders
	Contingent_Trade = 52,
	Qualified_Contingent_Trade = 53,
	Opening_Reopening_Trade_Detail = 55,
	Short_Sale_Restriction_Activated = 57,
	Short_Sale_Restriction_Continued = 58,
	Short_Sale_Restriction_Deactivated = 59,
	Short_Sale_Restriction_In_Effect = 60,
	Financial_Status_Bankrupt = 62,
	Financial_Status_Deficient = 63,
	Financial_Status_Delinquent = 64,
	Financial_Status_Bankrupt_and_Deficient = 65,
	Financial_Status_Bankrupt_and_Delinquent = 66,
	Financial_Status_Deficient_and_Delinquent = 67,
	Financial_Status_Deficient_Delinquent_and_Bankrupt = 68,
	Financial_Status_Liquidation = 69,
	Financial_Status_Creations_Suspended = 70,
	Financial_Status_Redemptions_Suspended = 71,
};
constexpr uint32_t TRADE_PLACEHOLDER = static_cast<uint32_t>(TradeCondition::PLACEHOLDER);
constexpr uint32_t MAX_VALID_TRADE_CONDITION = 71;


enum class TradeConditionType : uint16_t
{
	sale_condition,
	quote_condition,
	sip_generated_flag,
	financial_status_indicator,
	short_sale_restriction_indicator,
	settlement_condition,
	market_condition,
	trade_thru_exempt,
	PLACEHOLDER,  // PLACEHOLDER use for trade_condition_t[] placeholders
};


enum class QuoteCondition : int32_t
{
	Invalid = -1,
	Regular = 0,
	RegularTwoSidedOpen = 1,
	RegularOneSidedOpen = 2,
	SlowAsk = 3,
	SlowBid = 4,
	SlowBidAsk = 5,
	SlowDueLRPBid = 6,
	SlowDueLRPAsk = 7,
	SlowDueNYSELRP = 8,
	SlowDueSetSlowListBidAsk = 9,
	ManualAskAutomatedBid = 10,
	ManualBidAutomatedAsk = 11,
	ManualBidAndAsk = 12,
	Opening = 13,
	Closing = 14,
	Closed = 15,
	Resume = 16,
	FastTrading = 17,
	TradingRangeIndication = 18,
	MarketMakerQuotesClosed = 19,
	NonFirm = 20,
	NewsDissemination = 21,
	OrderInflux = 22,
	OrderImbalance = 23,
	DueToRelatedSecurityNewsDissemination = 23,
	DueToRelatedSecurityNewsPending = 25,
	AdditionalInformation = 26,
	NewsPending = 27,
	AdditionalInformationDueToRelatedSecurity = 28,
	DueToRelatedSecurity = 29,
	InViewOfCommon = 30,
	EquipmentChangeover = 31,
	NoOpenNoResponse = 32,
	SubPennyTrading = 33,
	AutomatedBidNoOfferNoBid = 34,
	LULDPriceBand = 35,
	MarketWideCircuitBreakerLevel1 = 36,
	MarketWideCircuitBreakerLevel2 = 37,
	MarketWideCircuitBreakerLevel3 = 38,
	RepublishedLULDPriceBand = 39,
	OnDemandAuction = 40,
	CashOnlySettlement = 41,
	NextDaySettlement = 42,
	LULDTradingPause = 43,
	SLowDuelRPBidAsk = 71,
	Cancel = 80,
	Corrected_Price = 81,
	SIPGenerated = 82,
	Unknown = 83,
	Crossed_Market = 84,
	Locked_Market = 85,
	Depth_On_Offer_Side = 86,
	Depth_On_Bid_Side = 87,
	Depth_On_Bid_And_Offer = 88,
	Pre_Opening_Indication = 89,
	Syndicate_Bid = 90,
	Pre_Syndicate_Bid = 91,
	Penalty_Bid = 92,
};
constexpr int32_t QUOTE_PLACEHOLDER = static_cast<int32_t>(QuoteCondition::Invalid);
constexpr int32_t MAX_VALID_QUOTE_CONDITION = 92;


// quote_t
template <typename Real, typename Size, typename Time>
struct quote_t
{
	Time timestamp;
	Real bid;
	Real ask;
	Size bid_size;
	Size ask_size;
	// TODO codes here instead? (see QuoteData)
};

// quote_ref_t
template <typename Real, typename Size, typename Time>
struct quote_ref_t
{
	const Time& timestamp;
	const Real& bid;
	const Real& ask;
	const Size& bid_size;
	const Size& ask_size;
};



// trade_t
template <typename Real, typename Size, typename Time>
struct trade_t
{
	Time timestamp;
	Real price;
	Size size;
	// TODO const ref??
	/*const Time& timestamp;
	const Real& price;
	const Size& size;*/
	// TODO codes here instead? (see TradeData)
};

// trade_ref_t
template <typename Real, typename Size, typename Time>
struct trade_ref_t
{
	const Time& timestamp;
	const Real& price;
	const Size& size;
};


typedef quote_ref_t<double, uint32_t, double> NBBO;
typedef trade_t<double, uint32_t, timestamp_t> Trade;
typedef quote_t<double, uint32_t, timestamp_t> Quote;


std::ostream& operator<< (std::ostream& out, const Quote& quote);
std::ostream& operator<< (std::ostream& out, const Trade& trade);


// TODO 32bit Size??

template <typename Real, typename Volume, typename Time, typename Size>
struct bar_t
{
	using time_type = Time;
	using real_type = Real;
	using volume_type = Volume;
	using size_type = Size;

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
	static constexpr ptrdiff_t NUM_COLS = 101;

	static constexpr ptrdiff_t _IN_NUM_COLS = 6;
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

	static constexpr ptrdiff_t _DEP_NUM_COLS = 4;
	enum Dep : ptrdiff_t
	{
		// 4 cols
		hlc3 = _IN_NUM_COLS,
		range,
		atr,
		past_range,
	};

	static constexpr ptrdiff_t _REGR_NUM_COLS = 2;
	enum Regr : ptrdiff_t
	{
		// 2 cols
		regr = _IN_NUM_COLS + _DEP_NUM_COLS,
		stddev,
	};

	static constexpr ptrdiff_t _DIFF_NUM_COLS = 4;
	enum Diff : ptrdiff_t
	{
		// 4 cols
		diff = _IN_NUM_COLS + _DEP_NUM_COLS + _REGR_NUM_COLS,
		range_t = 13,
		range_b = 14,
		range_pos = 15,
	};

	static constexpr ptrdiff_t _TA_NUM_COLS = 22;
	enum TA : ptrdiff_t
	{
		// 22 cols
		trend5 = _IN_NUM_COLS + _DEP_NUM_COLS + _REGR_NUM_COLS + _DIFF_NUM_COLS,
		trend9,
		trend20,
		trend50,
		volume_buy,
		volume_sell,
		volume_percent_buy,
		volume_percent_sell,
		obv,
		obv_trend20,
		obv_trend50,
		adi_trend20,
		adi_trend50,
		adi_obv_ratio,
		rsi,
		rsi_indicator,
		rsi_trend20,
		rsi_trend50,
		ppo,
		ppo_diff,
		adx,
		adx_indicator,
	};

	static constexpr ptrdiff_t _NORM_NUM_COLS = 2;
	enum Norm : ptrdiff_t
	{
		// 2 cols
		close_ln = _IN_NUM_COLS + _DEP_NUM_COLS + _REGR_NUM_COLS + _DIFF_NUM_COLS + _TA_NUM_COLS,
		close_next_norm,
	};

	static constexpr ptrdiff_t _CANDLE_NUM_COLS = 61;
	enum Candle : ptrdiff_t
	{
		// 61 cols
		_2CROWS = _IN_NUM_COLS + _DEP_NUM_COLS + _REGR_NUM_COLS + _DIFF_NUM_COLS + _TA_NUM_COLS + _NORM_NUM_COLS,
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

// assert ColPos configuration...
static_assert(ColPos::Candle::_MORNINGSTAR == ColPos::NUM_COLS - 1, "Invalid ColPos configuration...");
static_assert(ColPos::_IN_NUM_COLS + ColPos::_DEP_NUM_COLS + ColPos::_REGR_NUM_COLS + ColPos::_DIFF_NUM_COLS + ColPos::_TA_NUM_COLS + ColPos::_NORM_NUM_COLS + ColPos::_CANDLE_NUM_COLS == ColPos::NUM_COLS, "Invalid ColPos configuration...");

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
