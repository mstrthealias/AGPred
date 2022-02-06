#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <ta_defs.h>

#include "common.h"


// void prepare_market_inputs();

xt::xarray<double> process_step1_single(const char* symbol, const xt::xarray<double>& a_orig, const bool training, const int timeframe, const int interval, const bool ext_hours);

void process_step1(const char* symbol, dfs_map_t& dfs, const int timeframe, const bool ext_hours, const interval_map_t& interval_map);

void apply_candles(const char* symbol, xt::xarray<double>& o_results, const xt::xarray<double>& a_in);

void apply_step2(const char* symbol, xt::xarray<double>& o_results, const unsigned int timeframe, const unsigned int interval, const bool training, const bool ext_hours);

void write_dataset(const char* symbol, xt::xarray<double>& o_results);

xt::xarray<double> do_fill(const xt::xarray<double>& a_in, const bool ext_hours);

xt::xarray<double> do_filter_pre(const xt::xarray<double>& a_in);

xt::xarray<double> do_filter_post(const xt::xarray<double>& a_in, const bool training, const bool ext_hours);

TA_RetCode do_dep_columns(xt::xarray<double>& o_results, const bool training);

TA_RetCode add_regr(xt::xarray<double>& o_results, const unsigned int interval);

TA_RetCode do_ta(xt::xarray<double>& o_results);

void do_diffs(xt::xarray<double>& o_results);

void apply_regr(xt::xarray<double>& o_results, const unsigned int interval);

void do_norm(xt::xarray<double>& o_results, const unsigned int interval);

xt::xarray<double> do_cleanup_initial(const xt::xarray<double>& a_in, const unsigned int interval);

xt::xarray<double> do_cleanup_final(xt::xarray<double>& a_in);

xt::xarray<double> do_market_inputs(const xt::xarray<double>& a_in);

void do_outputs(const char* symbol, xt::xarray<double>& o_outputs, const xt::xarray<double>& o_results, const unsigned int timeframe, const unsigned int interval);


static const std::array<std::tuple<ColPosType, ptrdiff_t>, 98> COLS_FORMAT1 = {
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::timestamp),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::open),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::high),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::low),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::close),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::volume),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_2CROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3BLACKCROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3INSIDE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3LINESTRIKE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3OUTSIDE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3STARSINSOUTH),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3WHITESOLDIERS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ABANDONEDBABY),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ADVANCEBLOCK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_BELTHOLD),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_BREAKAWAY),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_CLOSINGMARUBOZU),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_CONCEALBABYSWALL),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_COUNTERATTACK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DARKCLOUDCOVER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DOJISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DRAGONFLYDOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ENGULFING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_EVENINGDOJISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_EVENINGSTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_GAPSIDESIDEWHITE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_GRAVESTONEDOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HAMMER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HANGINGMAN),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HARAMI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HARAMICROSS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HIGHWAVE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HIKKAKE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HIKKAKEMOD),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HOMINGPIGEON),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_IDENTICAL3CROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_INNECK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_INVERTEDHAMMER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_KICKING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_KICKINGBYLENGTH),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_LADDERBOTTOM),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_LONGLEGGEDDOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_LONGLINE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MARUBOZU),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MATCHINGLOW),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MATHOLD),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MORNINGDOJISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MORNINGSTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ONNECK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_PIERCING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_RICKSHAWMAN),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_RISEFALL3METHODS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SEPARATINGLINES),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SHOOTINGSTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SHORTLINE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SPINNINGTOP),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_STALLEDPATTERN),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_STICKSANDWICH),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_TAKURI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_TASUKIGAP),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_THRUSTING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_TRISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_UNIQUE3RIVER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_UPSIDEGAP2CROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_XSIDEGAP3METHODS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::hlc3),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::range),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::atr),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::past_range),

	//std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Regr, ColPos::Regr::regr),
	//std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Regr, ColPos::Regr::stddev),

	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend5),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend9),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_buy),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_sell),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_percent_buy),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_percent_sell),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::obv),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::obv_trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::obv_trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adi_trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adi_trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adi_obv_ratio),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi_indicator),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi_trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi_trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::ppo),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::ppo_diff),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adx),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adx_indicator),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::diff),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::range_t),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::range_b),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::range_pos),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Norm, ColPos::Norm::close_ln),
};

static const std::array<std::tuple<ColPosType, ptrdiff_t>, 100> COLS_FORMAT_YAR = {
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::timestamp),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::open),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::high),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::low),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::close),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::In, ColPos::In::volume),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::hlc3),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Regr, ColPos::Regr::regr),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Regr, ColPos::Regr::stddev),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_buy),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_sell),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_percent_buy),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::volume_percent_sell),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::range),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::diff),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::range_t),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::range_b),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Diff, ColPos::Diff::range_pos),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Norm, ColPos::Norm::close_ln),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::past_range),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Dep, ColPos::Dep::atr),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend5),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend9),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_2CROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3BLACKCROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3INSIDE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3LINESTRIKE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3OUTSIDE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3STARSINSOUTH),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_3WHITESOLDIERS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ABANDONEDBABY),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ADVANCEBLOCK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_BELTHOLD),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_BREAKAWAY),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_CLOSINGMARUBOZU),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_CONCEALBABYSWALL),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_COUNTERATTACK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DARKCLOUDCOVER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DOJISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_DRAGONFLYDOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ENGULFING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_EVENINGDOJISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_EVENINGSTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_GAPSIDESIDEWHITE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_GRAVESTONEDOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HAMMER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HANGINGMAN),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HARAMI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HARAMICROSS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HIGHWAVE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HIKKAKE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HIKKAKEMOD),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_HOMINGPIGEON),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_IDENTICAL3CROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_INNECK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_INVERTEDHAMMER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_KICKING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_KICKINGBYLENGTH),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_LADDERBOTTOM),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_LONGLEGGEDDOJI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_LONGLINE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MARUBOZU),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MATCHINGLOW),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MATHOLD),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MORNINGDOJISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_MORNINGSTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_ONNECK),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_PIERCING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_RICKSHAWMAN),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_RISEFALL3METHODS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SEPARATINGLINES),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SHOOTINGSTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SHORTLINE),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_SPINNINGTOP),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_STALLEDPATTERN),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_STICKSANDWICH),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_TAKURI),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_TASUKIGAP),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_THRUSTING),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_TRISTAR),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_UNIQUE3RIVER),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_UPSIDEGAP2CROWS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::Candle, ColPos::Candle::_XSIDEGAP3METHODS),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::obv),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::obv_trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::obv_trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adi_trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adi_trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adi_obv_ratio),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi_indicator),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi_trend20),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::rsi_trend50),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::ppo),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::ppo_diff),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adx),
	std::make_tuple<ColPosType, ptrdiff_t>(ColPosType::TA, ColPos::TA::adx_indicator),
};

#endif // PREPROCESS_H
