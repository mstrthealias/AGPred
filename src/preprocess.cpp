#include "preprocess.h"

#include <tuple>

#include <xtensor/xview.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xset_operation.hpp>
#include <xtensor/xvectorize.hpp>
#include <xtensor/xnpy.hpp>
#include <xtensor/xmasked_view.hpp>
#include <xtensor/xmath.hpp>

#include <ta_libc.h>

#include "common.h"
#include "defs.h"
#include "util.h"


typedef int(*TA_CDL_Func_Lookback)(
	void);

typedef int(*TA_CDL_OptPen_Func_Lookback)(
	double optInPenetration);

typedef TA_RetCode(*TA_CDL_Func)(
	TA_Integer  startIdx,
	TA_Integer  endIdx,
	const real_t inOpen[],
	const real_t inHigh[],
	const real_t inLow[],
	const real_t inClose[],
	TA_Integer* outBegIdx,
	TA_Integer* outNbElement,
	TA_Integer outInteger[]);

typedef TA_RetCode(*TA_CDL_OptPen_Func)(
	TA_Integer  startIdx,
	TA_Integer  endIdx,
	const real_t inOpen[],
	const real_t inHigh[],
	const real_t inLow[],
	const real_t inClose[],
    double optInPenetration,
	TA_Integer* outBegIdx,
	TA_Integer* outNbElement,
	TA_Integer outInteger[]);

#ifdef AGPRED_DOUBLE_P
const std::vector<std::tuple<TA_CDL_Func, TA_CDL_Func_Lookback>> CANDLE_FUNCTIONS1 = {
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL2CROWS, &TA_CDL2CROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL3BLACKCROWS, &TA_CDL3BLACKCROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL3INSIDE, &TA_CDL3INSIDE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL3LINESTRIKE, &TA_CDL3LINESTRIKE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL3OUTSIDE, &TA_CDL3OUTSIDE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL3STARSINSOUTH, &TA_CDL3STARSINSOUTH_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDL3WHITESOLDIERS, &TA_CDL3WHITESOLDIERS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLADVANCEBLOCK, &TA_CDLADVANCEBLOCK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLBELTHOLD, &TA_CDLBELTHOLD_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLBREAKAWAY, &TA_CDLBREAKAWAY_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLCLOSINGMARUBOZU, &TA_CDLCLOSINGMARUBOZU_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLCONCEALBABYSWALL, &TA_CDLCONCEALBABYSWALL_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLCOUNTERATTACK, &TA_CDLCOUNTERATTACK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLDOJI, &TA_CDLDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLDOJISTAR, &TA_CDLDOJISTAR_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLDRAGONFLYDOJI, &TA_CDLDRAGONFLYDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLENGULFING, &TA_CDLENGULFING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLGAPSIDESIDEWHITE, &TA_CDLGAPSIDESIDEWHITE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLGRAVESTONEDOJI, &TA_CDLGRAVESTONEDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHAMMER, &TA_CDLHAMMER_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHANGINGMAN, &TA_CDLHANGINGMAN_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHARAMI, &TA_CDLHARAMI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHARAMICROSS, &TA_CDLHARAMICROSS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHIGHWAVE, &TA_CDLHIGHWAVE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHIKKAKE, &TA_CDLHIKKAKE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHIKKAKEMOD, &TA_CDLHIKKAKEMOD_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLHOMINGPIGEON, &TA_CDLHOMINGPIGEON_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLIDENTICAL3CROWS, &TA_CDLIDENTICAL3CROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLINNECK, &TA_CDLINNECK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLINVERTEDHAMMER, &TA_CDLINVERTEDHAMMER_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLKICKING, &TA_CDLKICKING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLKICKINGBYLENGTH, &TA_CDLKICKINGBYLENGTH_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLLADDERBOTTOM, &TA_CDLLADDERBOTTOM_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLLONGLEGGEDDOJI, &TA_CDLLONGLEGGEDDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLLONGLINE, &TA_CDLLONGLINE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLMARUBOZU, &TA_CDLMARUBOZU_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLMATCHINGLOW, &TA_CDLMATCHINGLOW_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLONNECK, &TA_CDLONNECK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLPIERCING, &TA_CDLPIERCING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLRICKSHAWMAN, &TA_CDLRICKSHAWMAN_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLRISEFALL3METHODS, &TA_CDLRISEFALL3METHODS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLSEPARATINGLINES, &TA_CDLSEPARATINGLINES_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLSHOOTINGSTAR, &TA_CDLSHOOTINGSTAR_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLSHORTLINE, &TA_CDLSHORTLINE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLSPINNINGTOP, &TA_CDLSPINNINGTOP_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLSTALLEDPATTERN, &TA_CDLSTALLEDPATTERN_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLSTICKSANDWICH, &TA_CDLSTICKSANDWICH_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLTAKURI, &TA_CDLTAKURI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLTASUKIGAP, &TA_CDLTASUKIGAP_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLTHRUSTING, &TA_CDLTHRUSTING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLTRISTAR, &TA_CDLTRISTAR_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLUNIQUE3RIVER, &TA_CDLUNIQUE3RIVER_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLUPSIDEGAP2CROWS, &TA_CDLUPSIDEGAP2CROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_CDLXSIDEGAP3METHODS, &TA_CDLXSIDEGAP3METHODS_Lookback),
};
const std::vector<std::tuple<const TA_CDL_OptPen_Func, const TA_CDL_OptPen_Func_Lookback>> CANDLE_FUNCTIONS2 = {
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLABANDONEDBABY, &TA_CDLABANDONEDBABY_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLDARKCLOUDCOVER, &TA_CDLDARKCLOUDCOVER_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLEVENINGDOJISTAR, &TA_CDLEVENINGDOJISTAR_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLEVENINGSTAR, &TA_CDLEVENINGSTAR_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLMATHOLD, &TA_CDLMATHOLD_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLMORNINGDOJISTAR, &TA_CDLMORNINGDOJISTAR_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_CDLMORNINGSTAR, &TA_CDLMORNINGSTAR_Lookback),
};
#else
const std::vector<std::tuple<TA_CDL_Func, TA_CDL_Func_Lookback>> CANDLE_FUNCTIONS1 = {
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL2CROWS, &TA_CDL2CROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL3BLACKCROWS, &TA_CDL3BLACKCROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL3INSIDE, &TA_CDL3INSIDE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL3LINESTRIKE, &TA_CDL3LINESTRIKE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL3OUTSIDE, &TA_CDL3OUTSIDE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL3STARSINSOUTH, &TA_CDL3STARSINSOUTH_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDL3WHITESOLDIERS, &TA_CDL3WHITESOLDIERS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLADVANCEBLOCK, &TA_CDLADVANCEBLOCK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLBELTHOLD, &TA_CDLBELTHOLD_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLBREAKAWAY, &TA_CDLBREAKAWAY_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLCLOSINGMARUBOZU, &TA_CDLCLOSINGMARUBOZU_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLCONCEALBABYSWALL, &TA_CDLCONCEALBABYSWALL_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLCOUNTERATTACK, &TA_CDLCOUNTERATTACK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLDOJI, &TA_CDLDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLDOJISTAR, &TA_CDLDOJISTAR_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLDRAGONFLYDOJI, &TA_CDLDRAGONFLYDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLENGULFING, &TA_CDLENGULFING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLGAPSIDESIDEWHITE, &TA_CDLGAPSIDESIDEWHITE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLGRAVESTONEDOJI, &TA_CDLGRAVESTONEDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHAMMER, &TA_CDLHAMMER_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHANGINGMAN, &TA_CDLHANGINGMAN_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHARAMI, &TA_CDLHARAMI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHARAMICROSS, &TA_CDLHARAMICROSS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHIGHWAVE, &TA_CDLHIGHWAVE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHIKKAKE, &TA_CDLHIKKAKE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHIKKAKEMOD, &TA_CDLHIKKAKEMOD_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLHOMINGPIGEON, &TA_CDLHOMINGPIGEON_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLIDENTICAL3CROWS, &TA_CDLIDENTICAL3CROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLINNECK, &TA_CDLINNECK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLINVERTEDHAMMER, &TA_CDLINVERTEDHAMMER_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLKICKING, &TA_CDLKICKING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLKICKINGBYLENGTH, &TA_CDLKICKINGBYLENGTH_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLLADDERBOTTOM, &TA_CDLLADDERBOTTOM_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLLONGLEGGEDDOJI, &TA_CDLLONGLEGGEDDOJI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLLONGLINE, &TA_CDLLONGLINE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLMARUBOZU, &TA_CDLMARUBOZU_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLMATCHINGLOW, &TA_CDLMATCHINGLOW_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLONNECK, &TA_CDLONNECK_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLPIERCING, &TA_CDLPIERCING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLRICKSHAWMAN, &TA_CDLRICKSHAWMAN_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLRISEFALL3METHODS, &TA_CDLRISEFALL3METHODS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLSEPARATINGLINES, &TA_CDLSEPARATINGLINES_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLSHOOTINGSTAR, &TA_CDLSHOOTINGSTAR_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLSHORTLINE, &TA_CDLSHORTLINE_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLSPINNINGTOP, &TA_CDLSPINNINGTOP_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLSTALLEDPATTERN, &TA_CDLSTALLEDPATTERN_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLSTICKSANDWICH, &TA_CDLSTICKSANDWICH_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLTAKURI, &TA_CDLTAKURI_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLTASUKIGAP, &TA_CDLTASUKIGAP_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLTHRUSTING, &TA_CDLTHRUSTING_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLTRISTAR, &TA_CDLTRISTAR_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLUNIQUE3RIVER, &TA_CDLUNIQUE3RIVER_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLUPSIDEGAP2CROWS, &TA_CDLUPSIDEGAP2CROWS_Lookback),
	std::make_tuple<TA_CDL_Func, TA_CDL_Func_Lookback>(&TA_S_CDLXSIDEGAP3METHODS, &TA_CDLXSIDEGAP3METHODS_Lookback),
};
const std::vector<std::tuple<const TA_CDL_OptPen_Func, const TA_CDL_OptPen_Func_Lookback>> CANDLE_FUNCTIONS2 = {
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLABANDONEDBABY, &TA_CDLABANDONEDBABY_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLDARKCLOUDCOVER, &TA_CDLDARKCLOUDCOVER_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLEVENINGDOJISTAR, &TA_CDLEVENINGDOJISTAR_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLEVENINGSTAR, &TA_CDLEVENINGSTAR_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLMATHOLD, &TA_CDLMATHOLD_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLMORNINGDOJISTAR, &TA_CDLMORNINGDOJISTAR_Lookback),
	std::make_tuple<TA_CDL_OptPen_Func, TA_CDL_OptPen_Func_Lookback>(&TA_S_CDLMORNINGSTAR, &TA_CDLMORNINGSTAR_Lookback),
};
#endif // AGPRED_DOUBLE_P

//constexpr size_t num_patterns = std::tuple_size_v<T> + CANDLE_FUNCTIONS2.size();


static inline void negate_row(xt::xarray<real_t>& row, ptrdiff_t idx)
{
	xt::row(row, idx) = -xt::row(row, idx);
}


void _print_ta_groups()
{
	TA_StringTable* table;
	TA_RetCode retCode;
	int i;
	retCode = TA_GroupTableAlloc(&table);
	if (retCode == TA_SUCCESS)
	{
		for (i = 0; i < table->size; i++)
			printf("%s\n", table->string[i]);
		TA_GroupTableFree(table);
	}
}

void _print_ta_functions(const char* group)
{
	TA_StringTable* table;
	TA_RetCode retCode;
	int i;
	retCode = TA_FuncTableAlloc(group, &table);
	if (retCode == TA_SUCCESS)
	{
		for (i = 0; i < table->size; i++)
			printf("%s\n", table->string[i]);
		TA_FuncTableFree(table);
	}
}

void _iter_ta_functions(const char* group)
{
	TA_StringTable* table;
	const TA_FuncHandle* handle;
	const TA_FuncInfo* theInfo;
	TA_RetCode retCode;

	retCode = TA_FuncTableAlloc(group, &table);
	if (retCode == TA_SUCCESS)
	{
		for (int i = 0; i < table->size; i++)
		{
			retCode = TA_GetFuncHandle(table->string[i], &handle);

			if (retCode == TA_SUCCESS)
			{
				
				/*
				retCode = TA_GetFuncInfo(handle, &theInfo);
				if (retCode == TA_SUCCESS)
					printf("%s: Nb Input = %d\n", table->string[i], theInfo->nbInput);
				*/
			}
		}
		TA_FuncTableFree(table);
	}
}



real_t mk_hlc3(real_t high, real_t low, real_t close)
{
	// dfin['hlc3'] = (dfin['high'] + dfin['low'] + dfin['close']) / 3.0
	return (high + low + close) / 3.0;
}

real_t mk_range(real_t high, real_t low)
{
	// dfin['range'] = dfin['high'] - dfin['low']
	return high - low;
}

real_t mk_diff(real_t close, real_t open)
{
	// dfin['diff'] = dfin['close'] - dfin['open']
	return close - open;
}

real_t mk_range_t(real_t open, real_t high, real_t close)
{
	// dfin['range_t'] = dfin['high'] - dfin [["open", "close"]] .max(axis = 1)
	return high - std::max(open, close);
}

real_t mk_range_b(real_t open, real_t low, real_t close)
{
	// dfin['range_b'] = dfin [["open", "close"]] .min(axis = 1) - dfin['low']
	return std::min(open, close) - low;
}

real_t mk_range_pos(real_t range_t, real_t range_b)
{
	// dfin['range_pos'] = 0.5 - (dfin['range_t'] / (dfin['range_t'] + dfin['range_b'])).where(dfin['range_t'] + dfin['range_b'] > 0, 0.5)
	if (range_t + range_b > 0)
		return 0.5 - (range_t / (range_t + range_b));
	else
		return 0.0f;
}


real_t mk_norm_stddevs(real_t val, real_t stddev)
{
	return val / stddev;
	//dfin[k] = dfin[k] / stddev
}

real_t mk_norm_n_stddevs(real_t val, real_t regr, real_t stddev)
{
	// calculate number of stddev[1]
	if (stddev == 0)
		return 0.0f;
	else
		return (val - regr) / stddev;
	//dfin[k] = (dfin[k] - regr) / stddev
	//// normalize to price from linear regression
	//// dfin[k] = dfin[k] - regr
}

real_t mk_norm_scaled_stddevs1(real_t val, real_t stddev)
{
	return pow((100.0 * val / stddev), 2) / 1000.0;
	//dfin[k] = ((100.0 * dfin[k] / stddev) ** 2) / 1000.0
}

real_t mk_norm_scaled_stddevs2(real_t val, real_t stddev)
{
	return pow((100.0 * val / stddev), 3) / 1000.0;
	//dfin[k] = ((100.0 * dfin[k] / stddev) ** 3) / 1000.0
}

real_t mk_norm_scaled_stddevs3(real_t val, real_t stddev)
{
	return pow((100.0 * val / stddev), 2) / 100.0;
	//dfin[k] = ((100.0 * dfin[k] / stddev) ** 2) / 100.0
}


real_t mk_volume_buy(real_t high, real_t low, real_t close, real_t volume)
{
	// Buy Volume = iff( (high==low), 0, volume*(close-low)/(high-low))
	if (dbl_equal(high, low))  // TODO compare directly? o.0
		return 0.0;
	else
		return volume * (close - low) / (high - low);
	//dfin['volume_buy'] = (dfin['volume'] * (dfin['close'] - dfin['low']) / (dfin['high'] - dfin['low'])).where(dfin['high'] != dfin['low'], 0.0)
}

real_t mk_volume_sell(real_t high, real_t low, real_t volume, real_t volume_buy)
{
	// Sell Volume = iff( (high==low), 0, volume*(high-close)/(high-low))
	if (dbl_equal(high, low))  // TODO compare directly? o.0
		return 0.0;
	else
		return volume - volume_buy;
	//dfin['volume_sell'] = (dfin['volume'] - dfin['volume_buy']).where(dfin['high'] != dfin['low'], 0.0)
	// w/o using volume_buy:
	// //dfin['volume_sell'] = (dfin['volume'] * (dfin['high'] - dfin['close']) / (dfin['high'] - dfin['low'])).where(dfin['high'] != dfin['low'], 0.0)
}

real_t mk_volume_percent_buy(real_t volume_buy, real_t volume_sum)
{
	// Buy Volume Percent = BV / (BV + SV)
	if (volume_sum > 0)
		return volume_buy / volume_sum;
	else
		return 0.0;
	//dfin['volume_percent_buy'] = (dfin['volume_buy'] / vol_sum).where(vol_sum > 0, 0.0)
}

real_t mk_volume_percent_sell(real_t volume_sell, real_t volume_sum)
{
	// Sell Volume Percent = SV / (BV + SV)
	if (volume_sum > 0)
		return volume_sell / volume_sum;  // convert to negative...
	else
		return 0.0;
	//dfin['volume_percent_sell'] = (dfin['volume_sell'] / vol_sum).where(vol_sum > 0, 0.0)
}

real_t mk_rsi_indicator(real_t rsi)
{
	return (((rsi < 30) * -(rsi - 50.0)) + ((rsi > 70) * -(rsi - 50.0))) / 100.0;
	//rsi_oversold = rsi < 30
	//rsi_overbought = rsi > 70
	//rsi_shifted = -(rsi - 50.0)
	//dfin['rsi_indicator'] = ((rsi_oversold * rsi_shifted) + (rsi_overbought * rsi_shifted)) / 100.0
}

real_t mk_adx_indicator(real_t adx, real_t adx_pos, real_t adx_neg)
{
	//// determine ADX trend singles
	//adx_strong_trend = (adx > 20)
	//adx_strength = ((adx - 20) / 10.0)
	return ((adx > 20) * (adx_pos > adx_neg) * ((adx - 20) / 10.0)) + ((adx > 20) * (adx_neg > adx_pos) * -((adx - 20) / 10.0));
	//// // ADX above 20 and DI+ above DI-: That's an uptrend.
	//// dfin["adx_uptrend"] = adx_strong_trend * (indicator_adx.adx_pos() > indicator_adx.adx_neg())
	//// // ADX above 20 and DI- above DI+: That's a downtrend.
	//// dfin["adx_downtrend"] = adx_strong_trend * (indicator_adx.adx_neg() > indicator_adx.adx_pos())
	//dfin['adx_indicator'] = (adx_strong_trend * (indicator_adx.adx_pos() > indicator_adx.adx_neg()) * adx_strength) + (adx_strong_trend * (indicator_adx.adx_neg() > indicator_adx.adx_pos()) * -adx_strength)
}

real_t mk_adi_pre_cumsum(real_t high, real_t low, real_t close, real_t volume)
{
	if (dbl_equal(high, low))
		return 0.0;
	else
		return volume * ((close - low) - (high - close)) / (high - low);
	//clv = ((self._close - self._low) - (self._high - self._close)) / (
	//    self._high - self._low
	//)
	//clv = clv.fillna(0.0)  # float division by zero
	//adi = clv * self._volume
	//// self._adi = adi.cumsum()  // Note: performed after running this vectorized fn
}

/*real_t mk_adi_obv_ratio(real_t obv, real_t adi)
{
	if (dbl_equal(adi, 0.0))
		return 0.0f;
	else
		return obv / (adi * 10.0);
	//dfin['adi_obv_ratio'] = dfin['obv'] / (adi * 10.0)
}*/


/*real_t mk_xfrm_ln_div_10(real_t val)
{
	return std::log(val + 1) / 10.0;
	//dfin['close_ln'] = (close + 1).apply(np.log) / 10.0
	////dfin['close_ln'] = close.apply(np.log) / 10.0
}*/

real_t mk_xfrm_ln(real_t val)
{
	if (val <= 0)
		return 0.0f;
	else
		return std::log(val);
	//dfin['close_ln'] = (close + 1).apply(np.log) / 10.0
	////dfin['close_ln'] = close.apply(np.log) / 10.0
}

real_t mk_scale_div_100(real_t val)
{
	return val / 100.0;
	//dfin[k] = dfin[k] / 100.0
}

real_t mk_scale_div_10(real_t val)
{
	return val / 10.0;
	//dfin[k] = dfin[k] / 10.0
}

real_t mk_scale_rsi(real_t rsi)
{
	// scale rsi to - 1 - 1
	return (rsi / 50.0) - 1.0;
	//dfin[k] = (dfin[k] / 50.0) - 1.0
}

real_t mk_scale_vol(real_t hlc3_ln, real_t v, real_t adj_vol_divider)
{
	return pow(hlc3_ln * v / 100.0, 0.5) / adj_vol_divider;
	//((dfin['volume'] * close_ln) ** (1/2)) / 1000.0
	////dfin['volume'] = ((dfin['volume'] * close / 10000.0) ** 2) / 10.0
}

/*real_t mk_scale_obv(real_t obv)
{
	return obv / 1000000000.0;
	//(dfin['obv'] / 1000000000.0)
	////dfin['obv'] = (dfin['obv'] * close / 10000000.0)
}*/

real_t mk_scale_obv_trend(real_t obv_trend, real_t adj_vol_divider)
{
	return (obv_trend / 10000.0) / adj_vol_divider;
	//(dfin['obv_trend20'] / 10000000.0)
	////dfin['obv_trend20'] = (dfin['obv_trend20'] * close / 100000.0)
}


real_t mk_norm_10x_n_stddevs(real_t hlc3, real_t regr, real_t stddev)
{
	return 10.0 * (hlc3 - regr) / stddev;
	//hlc3_norm = 10.0 * (dfin['hlc3'] - dfin['regr']) / dfin['stddev']
}


real_t mk_min5(real_t a, real_t b, real_t c, real_t d, real_t e)
{
	return std::min({ a, b, c, d, e });
}

real_t mk_max5(real_t a, real_t b, real_t c, real_t d, real_t e)
{
	return std::max({ a, b, c, d, e });
}

real_t mk_min7(real_t a, real_t b, real_t c, real_t d, real_t e, real_t f, real_t g)
{
	return std::min({ a, b, c, d, e, f, g });
}

real_t mk_max7(real_t a, real_t b, real_t c, real_t d, real_t e, real_t f, real_t g)
{
	return std::max({ a, b, c, d, e, f, g });
}

real_t mk_subtract(real_t b, real_t a)
{
	return b - a;
}

real_t mk_sum(real_t b, real_t a)
{
	return b + a;
}

real_t mk_multiply(real_t b, real_t a)
{
	return b * a;
}

real_t mk_divide_by_160(real_t b)
{
	return b / 160.0;
}

real_t mk_vec_equals(real_t b, real_t a) {
	return static_cast<real_t>(dbl_equal(b, a));
}

real_t mk_vec_greater_sum2(real_t a, real_t b, real_t c) {
	return a > b + c;
}

real_t mk_vec_less_sub2(real_t a, real_t b, real_t c) {
	return a < b - c;
}


const auto vec_range = xt::vectorize(mk_range);
const auto vec_hlc3 = xt::vectorize(mk_hlc3);
const auto vec_diff = xt::vectorize(mk_diff);
const auto vec_range_t = xt::vectorize(mk_range_t);
const auto vec_range_b = xt::vectorize(mk_range_b);
const auto vec_range_pos = xt::vectorize(mk_range_pos);

const auto vec_norm_stddevs = xt::vectorize(mk_norm_stddevs);
const auto vec_norm_n_stddevs = xt::vectorize(mk_norm_n_stddevs);
const auto vec_norm_scaled_stddevs1 = xt::vectorize(mk_norm_scaled_stddevs1);
const auto vec_norm_scaled_stddevs2 = xt::vectorize(mk_norm_scaled_stddevs2);
const auto vec_norm_scaled_stddevs3 = xt::vectorize(mk_norm_scaled_stddevs3);

const auto vec_volume_buy = xt::vectorize(mk_volume_buy);
const auto vec_volume_sell = xt::vectorize(mk_volume_sell);
const auto vec_volume_percent_buy = xt::vectorize(mk_volume_percent_buy);
const auto vec_volume_percent_sell = xt::vectorize(mk_volume_percent_sell);
const auto vec_rsi_indicator = xt::vectorize(mk_rsi_indicator);
const auto vec_adx_indicator = xt::vectorize(mk_adx_indicator);
const auto vec_adi_pre_cumsum = xt::vectorize(mk_adi_pre_cumsum);
//const auto vec_adi_obv_ratio = xt::vectorize(mk_adi_obv_ratio);

//const auto vec_xfrm_ln_div_10 = xt::vectorize(mk_xfrm_ln_div_10);
const auto vec_xfrm_ln = xt::vectorize(mk_xfrm_ln);
const auto vec_scale_div_100 = xt::vectorize(mk_scale_div_100);
const auto vec_scale_div_10 = xt::vectorize(mk_scale_div_10);
const auto vec_scale_rsi = xt::vectorize(mk_scale_rsi);
const auto vec_scale_vol = xt::vectorize(mk_scale_vol);
//const auto vec_scale_obv = xt::vectorize(mk_scale_obv);
const auto vec_scale_obv_trend = xt::vectorize(mk_scale_obv_trend);

const auto vec_norm_10x_n_stddevs = xt::vectorize(mk_norm_10x_n_stddevs);

const auto vec_min5 = xt::vectorize(mk_min5);
const auto vec_max5 = xt::vectorize(mk_max5);
const auto vec_min7 = xt::vectorize(mk_min7);
const auto vec_max7 = xt::vectorize(mk_max7);
const auto vec_subtract = xt::vectorize(mk_subtract);
const auto vec_sum = xt::vectorize(mk_sum);
const auto vec_multiply = xt::vectorize(mk_multiply);
const auto vec_mk_divide_by_160 = xt::vectorize(mk_divide_by_160);
const auto vec_equals = xt::vectorize(mk_vec_equals);
const auto vec_greater_sum2 = xt::vectorize(mk_vec_greater_sum2);
const auto vec_less_sub2 = xt::vectorize(mk_vec_less_sub2);


// removes NANs and sorts the input dataset
xt::xarray<real_t> process_step1_single(const char* symbol, const xt::xarray<real_t>& a_orig, const bool training, const int timeframe, const int interval, const bool ext_hours)
{

	const size_t df_len_orig = a_orig.shape().at(1);

	//const auto a_orig_close = xt::xarray<real_t>(xt::row(a_orig, ColPos::In::close));

	// copy to new array with no nans
	//cleanup_pre_step1()
	xt::xarray<real_t> a_new = _xt_nonans(a_orig, ColPos::In::close);

	// reverse order if needed (a_new must be ASC order)
    const bool do_sort = _xt_check_2d_sort(a_new, ColPos::In::timestamp);
    if (do_sort)
        _xt_2d_sort(a_new, ColPos::In::timestamp);

	//// TODO actually sort intervals dataset...
	////dfinterval = dfs[interval].sort_index(ascending = True)
	if (do_sort && DEBUG_PRINT_DATA)
		std::cout << "process_step1_single() interval=" << interval << " sorted" << std::endl;

	return a_new;
}

// removes NANs and sorts the input dataset
void process_step1_single_2a(xt::xarray<timestamp_us_t>& ts_orig, xt::xarray<real_t>& a_orig, const char* symbol, const bool training, const int timeframe, const int interval, const bool ext_hours)
{

	const size_t df_len_orig = a_orig.shape().at(1);

	//const auto a_orig_close = xt::xarray<real_t>(xt::row(a_orig, ColPos::In::close));

	// copy to new array with no nans
	//cleanup_pre_step1()
	_xt_nonans_2a(ts_orig, a_orig, ColPos::In::close);

	// reverse order if needed (a_new must be ASC order)
	const bool do_sort = _xt_check_2d_sort_2a(ts_orig, ColPos::In::timestamp);
	if (do_sort)
		_xt_2d_sort_2a(ts_orig, a_orig, ColPos::In::timestamp);

	//// TODO actually sort intervals dataset...
	////dfinterval = dfs[interval].sort_index(ascending = True)
	if (do_sort && DEBUG_PRINT_DATA)
		std::cout << "process_step1_single_2a() interval=" << interval << " sorted" << std::endl;

}

void _process_step2_single(xt::xarray<real_t>& o_results, const char* symbol, const xt::xarray<real_t>& a_step1, const bool training, const int timeframe, const int interval, const bool ext_hours)
{
    xt::row(o_results, ColPos::In::open) = xt::row(a_step1, ColPos::In::open);
    xt::row(o_results, ColPos::In::high) = xt::row(a_step1, ColPos::In::high);
    xt::row(o_results, ColPos::In::low) = xt::row(a_step1, ColPos::In::low);
    xt::row(o_results, ColPos::In::close) = xt::row(a_step1, ColPos::In::close);
    xt::row(o_results, ColPos::In::volume) = xt::row(a_step1, ColPos::In::volume);

    // bid/ask if input has these columns
	const bool has_bid_ask = interval <= 15 && a_step1.shape().at(0) > static_cast<int>(ColPos::In::volume) + 1;
	if (has_bid_ask)
	{
		xt::row(o_results, ColPos::In::ask) = xt::row(a_step1, ColPos::In::ask);
		xt::row(o_results, ColPos::In::bid) = xt::row(a_step1, ColPos::In::bid);
		xt::row(o_results, ColPos::In::ask_high) = xt::row(a_step1, ColPos::In::ask_high);
		xt::row(o_results, ColPos::In::ask_low) = xt::row(a_step1, ColPos::In::ask_low);
		xt::row(o_results, ColPos::In::bid_high) = xt::row(a_step1, ColPos::In::bid_high);
		xt::row(o_results, ColPos::In::bid_low) = xt::row(a_step1, ColPos::In::bid_low);
	}
	else {
		// otherwise set zero
		const auto zeros = xt::zeros<real_t>(xt::row(a_step1, ColPos::In::open).shape());
		xt::row(o_results, ColPos::In::ask) = zeros;
		xt::row(o_results, ColPos::In::bid) = zeros;
		xt::row(o_results, ColPos::In::ask_high) = zeros;
		xt::row(o_results, ColPos::In::ask_low) = zeros;
		xt::row(o_results, ColPos::In::bid_high) = zeros;
		xt::row(o_results, ColPos::In::bid_low) = zeros;
	}

    // Track step1 run time for each interval
    //now = time.time()

    apply_candles(symbol, o_results, a_step1);  // +61 cols

    apply_step2(symbol, o_results, timeframe, interval, training, ext_hours, has_bid_ask);

}

xt::xarray<real_t> process_step2_single(const char* symbol, const xt::xarray<real_t>& a_step1, const bool training, const int timeframe, const int interval, const bool ext_hours)
{
	static_assert(NUM_COLUMNS >= ColPos::NUM_COLS);

	// allocate the results array
	xt::xarray<real_t> o_results = xt::zeros<real_t>({ NUM_COLUMNS, static_cast<int>(a_step1.shape().at(1)) });

	// copy input rows to o_results
	xt::row(o_results, ColPos::In::timestamp) = xt::row(a_step1, ColPos::In::timestamp);

    _process_step2_single(o_results, symbol, a_step1, training, timeframe, interval, ext_hours);

	return o_results;
}

xt::xarray<real_t> process_step2_single_2a(const char* symbol, const xt::xarray<timestamp_us_t>& ts_step1, const xt::xarray<real_t>& a_step1, const bool training, const int timeframe, const int interval, const bool ext_hours)
{

	// allocate the results array
	xt::xarray<real_t> o_results = xt::zeros<real_t>({ NUM_COLUMNS, static_cast<int>(a_step1.shape().at(1)) });

	// copy input rows to o_results
	xt::row(o_results, ColPos::In::timestamp) = xt::row(ts_step1, ColPos::In::timestamp);  // TODO subtract 37 years?

	_process_step2_single(o_results, symbol, a_step1, training, timeframe, interval, ext_hours);

	return o_results;
}


void process_step3_single(xt::xarray<real_t>& o_results, xt::xarray<double>& o_outputs, const char* symbol, const xt::xarray<real_t>& a_step1, const bool training, const int timeframe, const int interval, const bool ext_hours, const bool was_sorted)
{

	if (training && a_step1.shape().at(0) > static_cast<int>(ColPos::In::volume) + 1) {
		// TODO fe. null|
		// TODO verify o_outputs size? or not empty?
		if (!o_outputs.size())
			throw std::logic_error("o_outputs must be initialized.");
		do_outputs(o_outputs, symbol, o_results, a_step1, timeframe, interval);
	}

	// create a copy, removing initial nan values
	o_results = do_cleanup_initial(o_results, interval);

	if (training) {
		// copy outputs with same rows removed...
		o_outputs = do_cleanup_initial(o_outputs, interval);
	}

	/*
	// TODO restore original order?
	if (was_sorted)
	{
		// restore order
		_xt_2d_sort(o_results, 0, true);
	}*/

}
void process_step3_single_2a(xt::xarray<real_t>& o_results, xt::xarray<double>& o_outputs, xt::xarray<timestamp_us_t>& ts_step1, const char* symbol, const xt::xarray<real_t>& a_step1, const bool training, const int timeframe, const int interval, const bool ext_hours, const bool was_sorted)
{

	// TODO o_outputs double to preserve ts

	if (training && a_step1.shape().at(0) > static_cast<int>(ColPos::In::volume) + 1) {
		// TODO fe. null|
		// TODO verify o_outputs size? or not empty?
		if (!o_outputs.size())
			throw std::logic_error("o_outputs must be initialized.");
		do_outputs(o_outputs, symbol, o_results, a_step1, timeframe, interval);
		xt::row(o_outputs, ColPos::Output::ts) = xt::row(ts_step1, ColPos::In::timestamp) / xt::xarray<double>(SEC_TO_US);  // copy the accurate TS 
	}

	// create a copy, removing initial nan values
	do_cleanup_initial_2a(ts_step1, o_results, interval);

	if (training) {
		// copy outputs with same rows removed...
		o_outputs = do_cleanup_initial(o_outputs, interval);
	}

	/*
	// TODO restore original order?
	if (was_sorted)
	{
		// restore order
		_xt_2d_sort(o_results, 0, true);
	}*/

}


void process_step1to3(const char* symbol, dfs_map_t& dfs, const int timeframe, const bool ext_hours, const interval_map_t& interval_map)
{
	// loop interval_map
	for (const auto& i : interval_map)
	{
		const char* interval_str = i.first;
		const int &interval = i.second;

		auto& a_orig = dfs.at(interval);

		// TODO training
		const bool training = timeframe == interval;

		auto a_step1 = process_step1_single(symbol, a_orig, training, timeframe, interval, ext_hours);
		auto o_results = process_step2_single(symbol, a_step1, training, timeframe, interval, ext_hours);

		// TODO outputs ...
		// TODO skip creating outputs if !trainig?
		xt::xarray<double> o_outputs = xt::zeros<double>({ static_cast<int>(ColPos::_OUTPUT_NUM_COLS), static_cast<int>(a_step1.shape().at(1))});  // TODO timestamp and/or close in outputs?

		process_step3_single(o_results, o_outputs, symbol, a_step1, training, timeframe, interval, ext_hours);

		dfs[interval] = o_results;
		
		write_dataset(symbol, o_results);
		// TODO print run-time time and size...
	}

}

void process_step1to3_2a(const char* symbol, dfs_ts_map_t& dfs_ts, dfs_map_t& dfs, const int timeframe, const bool ext_hours, const interval_map_t& interval_map)
{
	// loop interval_map
	for (const auto& i : interval_map)
	{
		const char* interval_str = i.first;
		const int &interval = i.second;

		auto& a_raw = dfs.at(interval);
		auto& ts_raw = dfs_ts.at(interval);

		// TODO training
		const bool training = timeframe == interval;

		process_step1_single_2a(ts_raw, a_raw, symbol, training, timeframe, interval, ext_hours);
		auto o_results = process_step2_single_2a(symbol, ts_raw, a_raw, training, timeframe, interval, ext_hours);

		// TODO outputs ...
		// TODO skip creating outputs if !trainig?
		xt::xarray<double> o_outputs = xt::zeros<double>({ static_cast<int>(ColPos::_OUTPUT_NUM_COLS), static_cast<int>(a_raw.shape().at(1))});  // TODO timestamp and/or close in outputs?

		process_step3_single_2a(o_results, o_outputs, ts_raw, symbol, a_raw, training, timeframe, interval, ext_hours);

		dfs[interval] = o_results;
		dfs_ts[interval] = ts_raw;

		write_dataset_2a(symbol, o_results, ts_raw);
		// TODO print run-time time and size...
	}

}


void write_dataset(const char* symbol, const xt::xarray<real_t>& o_results)
{
    const auto& colsMap = COLS_FORMAT1;  // COLS_FORMAT_YAR;  //  COLS_FORMAT1;

    const size_t n_len = o_results.shape().at(1);
    xt::xarray<real_t> allCols = xt::zeros<real_t>({ static_cast<int>(colsMap.size()), static_cast<int>(o_results.shape().at(1)) });
    int pos = 0;

    for (std::tuple<ColPosType, ptrdiff_t> tpl : colsMap)
        xt::row(allCols, pos++) = xt::row(o_results, std::get<1>(tpl));

    std::string npy_file = "pyfiles/_tmp.AAPL.s0." + std::to_string(TIMEFRAME) + ".npy";
    xt::dump_npy(npy_file, allCols);
}

void write_dataset_2a(const char* symbol, const xt::xarray<real_t>& o_results, const xt::xarray<timestamp_us_t>& ts_raw)
{
	constexpr auto& colsMap = COLS_FORMAT1;  // COLS_FORMAT_YAR;  //  COLS_FORMAT1;

	const size_t n_len = o_results.shape().at(1);
	xt::xarray<double> allCols = xt::zeros<double>({ static_cast<int>(colsMap.size()), static_cast<int>(o_results.shape().at(1)) });
	int pos = 0;

	for (std::tuple<ColPosType, ptrdiff_t> tpl : colsMap) {
		if (std::get<1>(tpl) == ColPos::In::timestamp)
			xt::row(allCols, pos++) = xt::row(ts_raw, std::get<1>(tpl));
		else
			xt::row(allCols, pos++) = xt::row(o_results, std::get<1>(tpl));
	}

	std::string npy_file = "pyfiles/_tmp.AAPL.s0." + std::to_string(TIMEFRAME) + ".npy";
	xt::dump_npy(npy_file, allCols);
}


void apply_candles(const char* symbol, xt::xarray<real_t>& o_results, const xt::xarray<real_t>& a_in)
{
	//_iter_ta_functions("Pattern Recognition");

	const size_t n_len = o_results.shape().at(1);
	
	// Note: apparently need to copy from views, in order for TA_ functions to work properly
	const auto r_open = xt::xarray<real_t>(xt::row(a_in, ColPos::In::open));
	const auto r_high = xt::xarray<real_t>(xt::row(a_in, ColPos::In::high));
	const auto r_low = xt::xarray<real_t>(xt::row(a_in, ColPos::In::low));
	const auto r_close = xt::xarray<real_t>(xt::row(a_in, ColPos::In::close));
	const auto& rows_open = r_open.data();
	const auto& rows_high = r_high.data();
	const auto& rows_low = r_low.data();
	const auto& rows_close = r_close.data();
	const auto& o_shape = r_close.shape();

	TA_RetCode retCode;
	int outBegIdx = 0;
	int outNBElement = 0;

	// TODO real_t[] or std::vector ???
	//real_t col_data[n_len + 1];
	std::vector<int> candle_data(n_len + 1);
	int pos = static_cast<int>(ColPos::Candle::_2CROWS);
	for (auto &func : CANDLE_FUNCTIONS1)
	{
		/* For example:
		 * TA_RetCode TA_CDL2CROWS(int    startIdx,
							 int    endIdx,
							 const real_t inOpen[],
							 const real_t inHigh[],
							 const real_t inLow[],
							 const real_t inClose[],
							 int          *outBegIdx,
							 int          *outNBElement,
							 int           outInteger[] );
		 */
		std::fill(candle_data.begin(), candle_data.end(), 0);
		retCode = std::get<0>(func)(
			0,
			n_len,
			rows_open,
			rows_high,
			rows_low,
			rows_close,
			&outBegIdx,
			&outNBElement,
			candle_data.data() + std::get<1>(func)()
		);
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA-Lib CDL func error: " << retCode << std::endl;
			continue;
		}
		std::copy(candle_data.cbegin(), std::prev(candle_data.cend()), xt::row(o_results, pos++).begin());
		xt::row(o_results, pos - 1) = vec_mk_divide_by_160(xt::row(o_results, pos - 1));
	}

	for (auto& func : CANDLE_FUNCTIONS2)
	{
		/* For example:
		 * TA_RetCode TA_CDLMORNINGDOJISTAR(int    startIdx,
							 int    endIdx,
							 const real_t inOpen[],
							 const real_t inHigh[],
							 const real_t inLow[],
							 const real_t inClose[],
                             double        optInPenetration, // From 0 to TA_REAL_MAX
							 int          *outBegIdx,
							 int          *outNBElement,
							 int           outInteger[] );
		 */
		std::fill(candle_data.begin(), candle_data.end(), 0);
		retCode = std::get<0>(func)(
			0,
			n_len,
			rows_open,
			rows_high,
			rows_low,
			rows_close,
			0.0,  // TODO optInPenetration  (or does 0 make it use default?!)
			&outBegIdx,
			&outNBElement,
			candle_data.data() + std::get<1>(func)(0.0)
		);
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA-Lib CDL func error: " << retCode << std::endl;
			continue;
		}
		std::copy(candle_data.cbegin(), std::prev(candle_data.cend()), xt::row(o_results, pos++).begin());
		xt::row(o_results, pos - 1) = vec_mk_divide_by_160(xt::row(o_results, pos - 1));
	}
}

void apply_step2(const char* symbol, xt::xarray<real_t>& o_results, const unsigned int timeframe, const unsigned int interval, const bool training, const bool ext_hours, const bool has_bid_ask)
{
	//if (training && interval <= 30) {  // TODO !0
	//	a_in = do_fill(a_in, ext_hours);
	//	a_in = do_filter_pre(a_in);  // TODO
	//}

	do_dep_columns(o_results, training);  // +4 cols
	add_regr(o_results, interval);  // +2 cols
	
	do_ta(o_results);  // +22 cols
	do_diffs(o_results);  // +4 cols
	do_norm(o_results, interval, has_bid_ask);  // +2 cols, updates 21

	if (strcmp(symbol, "QQQ") != 0 && strcmp(symbol, "SPY") != 0) {  // not in{ 'QQQ', 'SPY' }
		// TODO
		// auto cols_market = do_market_inputs(a_in);
	}

	// drop auto - filled timestamps
	////dfin = dfin.reindex(orig_index, axis = 'index', method = None)

	//if (training && interval == 1) {
	//	a_in = do_filter_post(a_in, training, ext_hours);
	//}

	// TODO
	//a_results = do_cleanup_final(a_results);
}

xt::xarray<real_t> do_fill(const xt::xarray<real_t>& a_in, const bool ext_hours)
{
	const int n_cols = a_in.shape().at(0);
	xt::xarray<real_t> results = xt::zeros<real_t>({ n_cols, static_cast<int>(a_in.shape().at(1)) });
	return results;
}

xt::xarray<real_t> do_filter_pre(const xt::xarray<real_t>& a_in)
{
	const int n_cols = a_in.shape().at(0);
	xt::xarray<real_t> results = xt::zeros<real_t>({ n_cols, static_cast<int>(a_in.shape().at(1)) });
	/*
	expr_suffix = '[0-9]0' if interval == 0 else '00'
	if ext_hours:
		// limit to market hours + extended hours
		expr_parts = [
			'0[4-9]:[0-9]{2}:' + expr_suffix,
				'1[0-9]:[0-9]{2}:' + expr_suffix,
				'20:00:' + expr_suffix,
		]
	else:
		// limit to market hours
		expr_parts = [
			'09:[3-5][0-9]:' + expr_suffix,
				'1[0-4]:[0-9]{2}:' + expr_suffix,
				'15:[0-9]{2}:' + expr_suffix,
				'16:00:' + expr_suffix,
		]
	dfin = dfin.filter(regex = ' (' + '|'.join(expr_parts) + ')', axis = 0)
	*/
	return results;
}

xt::xarray<real_t> do_filter_post(const xt::xarray<real_t>& a_in, const bool training, const bool ext_hours)
{
	const int n_cols = a_in.shape().at(0);
	xt::xarray<real_t> results = xt::zeros<real_t>({ n_cols, static_cast<int>(a_in.shape().at(1)) });
	/*
	if (training && interval == 1) {
		if (ext_hours) {
			// limit to market hours + extended hours(excl.first 2hrs / last 1 hr)
			dfin = dfin.filter(regex = ' (0[6-9]:[0-9]{2}:00|1[0-9]:[0-9]{2}:00)', axis = 0)
		}
		else {
			// limit to market hours (excl. first/last 21 minutes)
			// dfin = dfin.filter(regex = ' (09:5[0-9]:00|1[0-4]:[0-9]{2}:00|15:[0-3][0-9]:00)', axis = 0)
			// limit to market hours(excl.first / last 5 minutes)
			dfin = dfin.filter(regex = ' (09:3[6-9]:00|09:[4-5][0-9]:00|1[0-4]:[0-9]{2}:00|15:[0-4][0-9]:00|15:5[0-5]:00)', axis = 0)
		}
	}
	*/
	return results;
}

TA_RetCode do_dep_columns(xt::xarray<real_t>& o_results, const bool training)
{
	const size_t n_len = o_results.shape().at(1);

	// Note: apparently need to copy from views, in order for TA_ functions to work properly
	const auto r_high = xt::xarray<real_t>(xt::row(o_results, ColPos::In::high));
	const auto r_low = xt::xarray<real_t>(xt::row(o_results, ColPos::In::low));
	const auto r_close = xt::xarray<real_t>(xt::row(o_results, ColPos::In::close));

	// add hlc3 / related dependent columns now
	xt::row(o_results, ColPos::Dep::hlc3) = vec_hlc3(r_high, r_low, r_close);  //:hlc3

	// other various columns...
	xt::row(o_results, ColPos::Dep::range) = vec_range(r_high, r_low);  //:range

	// Average True Range(ATR)
	{
		std::vector<double> atr(n_len + 1);
		int outBegIdx = 0;
		int outNBElement = 0;
		std::fill(atr.begin(), atr.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		TA_RetCode retCode = TA_ATR(0, n_len, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, atr.data() + TA_ATR_Lookback(14));
#else
		TA_RetCode retCode = TA_S_ATR(0, n_len, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, atr.data() + TA_ATR_Lookback(14));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "ATR error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(atr.cbegin(), std::prev(atr.cend()), xt::row(o_results, ColPos::Dep::atr).begin());  //:atr
	}

	// past_range
	{
		assert(CHECK_PAST_CNT == 5);
		xt::xarray<real_t> windowCols = xt::zeros<real_t>({ CHECK_PAST_CNT, static_cast<int>(n_len) });
		for (ptrdiff_t i = 0; i < CHECK_PAST_CNT; i++)
		{
			if (i)
				xt::row(windowCols, i) = xt::roll(r_low, i);
			else
				xt::row(windowCols, i) = r_low;
		}
		xt::xarray<real_t> rolling5_low = vec_min5(xt::row(windowCols, 0), xt::row(windowCols, 1), xt::row(windowCols, 2), xt::row(windowCols, 3), xt::row(windowCols, 4));

		for (ptrdiff_t i = 0; i < CHECK_PAST_CNT; i++)
		{
			if (i)
				xt::row(windowCols, i) = xt::roll(r_high, i);
			else
				xt::row(windowCols, i) = r_high;
		}
		xt::xarray<real_t> rolling5_high = vec_max5(xt::row(windowCols, 0), xt::row(windowCols, 1), xt::row(windowCols, 2), xt::row(windowCols, 3), xt::row(windowCols, 4));

		xt::row(o_results, ColPos::Dep::past_range) = vec_subtract(rolling5_high, rolling5_low);  //:past_range
	}
	return TA_SUCCESS;
}

TA_RetCode add_regr(xt::xarray<real_t>& o_results, const unsigned int interval)
{
	const int n_rows = static_cast<int>(o_results.shape().at(1));

	// copy rows for TALib
	const auto r_close = xt::xarray<real_t>(xt::row(o_results, ColPos::In::close));

	const int regrPeriod = INTERVAL_REGR_WINDOW;  // INTERVAL_REGR_WINDOWS.at(static_cast<const int>(interval));

	int outBegIdx = 0;
	int outNBElement = 0;
	TA_RetCode retCode;
	std::vector<double> taResult(static_cast<std::vector<real_t>::size_type>(n_rows) + 1);

	// Linear regression
	{
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG(0, n_rows, r_close.data(), regrPeriod, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_Lookback(regrPeriod));
#else
		retCode = TA_S_LINEARREG(0, n_rows, r_close.data(), regrPeriod, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_Lookback(regrPeriod));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "LINEARREG error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::Regr::regr).begin());  //:regr
	}

	// Standard Deviation
	{
		const real_t numDeviations = 1;
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_STDDEV(0, n_rows, r_close.data(), regrPeriod, numDeviations, &outBegIdx, &outNBElement, taResult.data() + TA_STDDEV_Lookback(regrPeriod, numDeviations));
#else
		retCode = TA_S_STDDEV(0, n_rows, r_close.data(), regrPeriod, numDeviations, &outBegIdx, &outNBElement, taResult.data() + TA_STDDEV_Lookback(regrPeriod, numDeviations));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "STDDEV error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::Regr::stddev).begin());  //:stddev
	}
	return TA_SUCCESS;
}

TA_RetCode do_ta(xt::xarray<real_t>& o_results)
{
	/*
	 * trend5, trend9, trend20, trend50, volume_percent_buy, volume_percent_sell
	 * obv_trend20, obv_trend50, adi_trend20, adi_trend50
	 * rsi, rsi_indicator, rsi_trend20, rsi_trend50, ppo, ppo_diff, adx, adx_indicator
	 */ 

	const int n_rows = static_cast<int>(o_results.shape().at(1));

	// copy rows for TALib
	const auto r_high = xt::xarray<real_t>(xt::row(o_results, ColPos::In::high));
	const auto r_low = xt::xarray<real_t>(xt::row(o_results, ColPos::In::low));
	const auto r_close = xt::xarray<real_t>(xt::row(o_results, ColPos::In::close));
	const auto r_volume = xt::xarray<real_t>(xt::row(o_results, ColPos::In::volume));
	// const auto& r_hlc3 = xt::row(o_results, ColPos::Dep::hlc3);
	//const auto& r_regr = xt::row(o_results, ColPos::Regr::regr);
	//const auto& r_stddev = xt::row(o_results, ColPos::Regr::stddev);

	int outBegIdx = 0;
	int outNBElement = 0;
	TA_RetCode retCode;
	std::vector<double> taResult(static_cast<std::vector<real_t>::size_type>(n_rows) + 1);
	xt::xarray<real_t> taTmp1 = xt::zeros<real_t>(r_high.shape());
	xt::xarray<real_t> taTmp2 = xt::zeros<real_t>(r_high.shape());

	// Simple trends: trend5, trend9, trend20, trend50 // TODO slope of # of std. devs ??
	{
		//close_norm = 10.0 * (dfin['close'] - dfin['regr']) / dfin['stddev'];
		//taTmp1 = vec_norm_10x_n_stddevs(r_close, r_regr, r_stddev);  //close_norm
		taTmp1 = r_close;
		
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 5, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(5));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 5, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(5));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::trend5).begin());  //:trend5

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 9, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(9));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 9, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(9));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::trend9).begin());  //:trend9

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::trend20).begin());  //:trend20

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::trend50).begin());  //:trend50
	}

	// Split volume into volume_percent_buy, volume_percent_sell
	{
		taTmp1 = vec_volume_buy(r_high, r_low, r_close, r_volume);  //volume_buy
		taTmp2 = vec_volume_sell(r_high, r_low, r_volume, taTmp1);  //volume_sell
		const auto taTmp3 = vec_sum(taTmp1, taTmp2);
		xt::row(o_results, ColPos::TA::volume_percent_buy) = vec_volume_percent_buy(taTmp1, taTmp3);  //:volume_percent_buy
		xt::row(o_results, ColPos::TA::volume_percent_sell) = vec_volume_percent_sell(taTmp2, taTmp3);  //:volume_percent_sell
	}


	/* add technical indicators(ATR, OBV, ADI, RSI, ADX, PPO) : */

	// Average True Range (ATR)
	// Note: applied in do_dep_columns
	
	// On Balance Volume Indicator (OBV) trends
	{
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_OBV(0, n_rows, r_close.data(), r_volume.data(), &outBegIdx, &outNBElement, taResult.data() + TA_OBV_Lookback());
#else
		retCode = TA_S_OBV(0, n_rows, r_close.data(), r_volume.data(), &outBegIdx, &outNBElement, taResult.data() + TA_OBV_Lookback());
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "OBV error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), taTmp1.begin());  // obv
		
		// OBV trend (20 and 50 period)
		
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::obv_trend20).begin());  //:obv_trend20

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::obv_trend50).begin());  //:obv_trend50
	}

	// Accumulation/Distribution Index (ADI)
	{
		// Note: instead of having OBV and ADI, use OBV and ratio of OBV/ADI  // (adi_obv_ratio instead of adi)
		taTmp1 = vec_adi_pre_cumsum(r_high, r_low, r_close, r_volume);
		taTmp2 = xt::cumsum(taTmp1);  // adi

		// ADI trend (20 and 50 period)

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp2.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp2.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::adi_trend20).begin());  //:adi_trend20

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp2.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp2.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::adi_trend50).begin());  //:adi_trend50

		//xt::row(o_results, ColPos::TA::adi_obv_ratio) = vec_adi_obv_ratio(xt::row(o_results, ColPos::TA::obv), taTmp2);  // adi_obv_ratio
	}

	// Relative Strength Index (RSI)
	{
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_RSI(0, n_rows, r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_RSI_Lookback(14));
#else
		retCode = TA_S_RSI(0, n_rows, r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_RSI_Lookback(14));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "RSI error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::rsi).begin());  //:rsi
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), taTmp1.begin());  // rsi
		xt::row(o_results, ColPos::TA::rsi_indicator) = vec_rsi_indicator(taTmp1);  //:rsi_indicator
		
		// RSI trend (20 and 50 period)

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 20, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(20));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::rsi_trend20).begin());  //:rsi_trend20

		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#else
		retCode = TA_S_LINEARREG_SLOPE(0, n_rows, taTmp1.data(), 50, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_SLOPE_Lookback(50));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "TA_LINEARREG_SLOPE error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::rsi_trend50).begin());  //:rsi_trend50
	}

	// Note: using PPO instead of MACD, as it's the same thing except PPO uses percentages
	// Percentage Price Oscillator (PPO)
	{
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_PPO(0, n_rows, r_close.data(), 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, taResult.data() + TA_PPO_Lookback(12, 26, TA_MAType_EMA));
#else
		retCode = TA_S_PPO(0, n_rows, r_close.data(), 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, taResult.data() + TA_PPO_Lookback(12, 26, TA_MAType_EMA));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "PPO error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::ppo).begin());  //:ppo
		xt::row(o_results, ColPos::TA::ppo) = vec_cleanup_float_errs(xt::nan_to_num(xt::row(o_results, ColPos::TA::ppo)));
		
		// 'ppo_diff'
		// PPO SIGNAL = _ema(self._ppo, self._window_sign, self._fillna)
		// PPO HIST = self._ppo - self._ppo_signal
		// copy PPO for EMA
		auto r_tmp = xt::xarray<real_t>(xt::nan_to_num(xt::row(o_results, ColPos::TA::ppo)));
		auto windowSign = 9;
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_EMA(0, n_rows, r_tmp.data(), windowSign, &outBegIdx, &outNBElement, taResult.data() + TA_EMA_Lookback(windowSign));
#else
		retCode = TA_S_EMA(0, n_rows, r_tmp.data(), windowSign, &outBegIdx, &outNBElement, taResult.data() + TA_EMA_Lookback(windowSign));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "EMA error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), r_tmp.begin());

		xt::row(o_results, ColPos::TA::ppo_diff) = vec_subtract(xt::row(o_results, ColPos::TA::ppo), r_tmp);  //:ppo_diff
	}

	// Average Directional Movement Index (ADX)
	{
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_ADX(0, n_rows, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_ADX_Lookback(14));
#else
		retCode = TA_S_ADX(0, n_rows, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_ADX_Lookback(14));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "ADX error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), xt::row(o_results, ColPos::TA::adx).begin());  //:adx

		// ADX trend signals (@see vec_adx_indicator)
		
		// adx_pos
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_PLUS_DI(0, n_rows, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_PLUS_DI_Lookback(14));
#else
		retCode = TA_S_PLUS_DI(0, n_rows, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_PLUS_DI_Lookback(14));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "PLUS_DI error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), taTmp1.begin());

		// adx_neg
		std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		retCode = TA_MINUS_DI(0, n_rows, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_MINUS_DI_Lookback(14));
#else
		retCode = TA_S_MINUS_DI(0, n_rows, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, taResult.data() + TA_MINUS_DI_Lookback(14));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "MINUS_DI error: " << retCode << std::endl;
			return retCode;
		}
		std::copy(taResult.cbegin(), std::prev(taResult.cend()), taTmp2.begin());

		xt::row(o_results, ColPos::TA::adx_indicator) = vec_adx_indicator(xt::row(o_results, ColPos::TA::adx), taTmp1, taTmp2);  //:adx_indicator
	}
	return TA_SUCCESS;
}

void do_diffs(xt::xarray<real_t>& o_results)
{
	const auto& r_open = xt::row(o_results, ColPos::In::open);
	const auto& r_high = xt::row(o_results, ColPos::In::high);
	const auto& r_low = xt::row(o_results, ColPos::In::low);
	const auto& r_close = xt::row(o_results, ColPos::In::close);
	
	xt::row(o_results, ColPos::Diff::diff) = vec_diff(r_close, r_open);  //:diff
	// xt::row(results, 1) = vec_range(r_high, r_low);  //:range  // dep column ...
	xt::row(o_results, ColPos::Diff::range_t) = vec_range_t(r_open, r_high, r_close);  //:range_t
	xt::row(o_results, ColPos::Diff::range_b) = vec_range_b(r_open, r_low, r_close);  //:range_b
	// dfin['range_inner'] = [abs(r.close - r.open) for j, r in dfin [['close', 'open']] .iterrows()]
	// dfin['direction'] = [(1 if r.close - r.open > 0 else 0) for j, r in dfin [['close', 'open']] .iterrows()]
	xt::row(o_results, ColPos::Diff::range_pos) = vec_range_pos(xt::row(o_results, ColPos::Diff::range_t), xt::row(o_results, ColPos::Diff::range_b));  //:range_pos
}


constexpr std::array<ColPos::In, 4> COLS_REGR_IN_N_STDDEVS = { ColPos::In::open, ColPos::In::high, ColPos::In::low, ColPos::In::close };
constexpr std::array<ColPos::Dep, 1> COLS_REGR_DEP_N_STDDEVS = { ColPos::Dep::hlc3 };
constexpr std::array<ColPos::Dep, 3> COLS_REGR_DEP_NORM_STDDEV = { ColPos::Dep::past_range, ColPos::Dep::range, ColPos::Dep::atr };
constexpr std::array<ColPos::Diff, 3> COLS_REGR_DIFF_NORM_STDDEV = { ColPos::Diff::diff, ColPos::Diff::range_t, ColPos::Diff::range_b };

void apply_regr(xt::xarray<real_t>& o_results, const unsigned int interval)
{
	//;open, ;high, ;low, ;close
	{
		// normalize with vec_norm_n_stddevs and ColPos::Regr::regr/ColPos::Regr::stddev
		const auto& r_regr = xt::row(o_results, ColPos::Regr::regr);
		const auto& r_stddev = xt::row(o_results, ColPos::Regr::stddev);
		
		for (const ColPos::In k : COLS_REGR_IN_N_STDDEVS)
			xt::row(o_results, k) = vec_norm_n_stddevs(xt::row(o_results, k), r_regr, r_stddev);
	}

	// TODO 'bid_high', 'bid_low', 'ask_high', 'ask_low', 'bid', 'ask'
	// TODO Note: 'bid_high', 'bid_low', 'ask_high', 'ask_low'

	/* //;close_next_norm  // TODO remove?
	{
		// TODO remove?
		// close_next_norm should be the shifted close value
		xt::row(o_results, ColPos::Norm::close_next_norm) = xt::roll(xt::row(o_results, ColPos::In::close), 1);  //:close_next_norm // TODO roll direction? TODO verify compile warning...
	}*/
	
	//;hlc3, ;atr, ;past_range, ;range, ;diff, ;range_t, ;range_b
	{
		const int n_rows = static_cast<int>(o_results.shape().at(1));

		constexpr int regrPeriod = INTERVAL_REGR_WINDOW;  // INTERVAL_REGR_WINDOWS.at(interval);

		// copy rows for TALib
		const auto r_hlc3 = xt::xarray<real_t>(xt::row(o_results, ColPos::Dep::hlc3));

		// first calculate hlc3regr and hlc3stddev:
		xt::xarray<real_t> hlc3regr = xt::zeros<real_t>(r_hlc3.shape());
		xt::xarray<real_t> hlc3stddev = xt::zeros<real_t>(r_hlc3.shape());
		
		int outBegIdx = 0;
		int outNBElement = 0;
		TA_RetCode retCode;
		bool hlc3ok = true;
		std::vector<double> taResult(n_rows + 1);

		// Linear regression (hlc3)
		{
			std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
			retCode = TA_LINEARREG(0, n_rows, r_hlc3.data(), regrPeriod, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_Lookback(regrPeriod));
#else
			retCode = TA_S_LINEARREG(0, n_rows, r_hlc3.data(), regrPeriod, &outBegIdx, &outNBElement, taResult.data() + TA_LINEARREG_Lookback(regrPeriod));
#endif
			if (retCode != TA_SUCCESS)
			{
				hlc3ok = false;
				std::cout << "LINEARREG error: " << retCode << std::endl;
			}
			else
			{
				std::copy(taResult.cbegin(), std::prev(taResult.cend()), hlc3regr.begin());
			}
		}

		// Standard Deviation (hlc3)
		{
			const real_t numDeviations = 1;
			std::fill(taResult.begin(), taResult.end(), NAN);
#ifdef AGPRED_DOUBLE_P
			retCode = TA_STDDEV(0, n_rows, r_hlc3.data(), regrPeriod, numDeviations, &outBegIdx, &outNBElement, taResult.data() + TA_STDDEV_Lookback(regrPeriod, numDeviations));
#else
			retCode = TA_S_STDDEV(0, n_rows, r_hlc3.data(), regrPeriod, numDeviations, &outBegIdx, &outNBElement, taResult.data() + TA_STDDEV_Lookback(regrPeriod, numDeviations));
#endif
			if (retCode != TA_SUCCESS)
			{
				hlc3ok = false;
				std::cout << "STDDEV error: " << retCode << std::endl;
			}
			else
			{
				std::copy(taResult.cbegin(), std::prev(taResult.cend()), hlc3stddev.begin());
			}
		}
		if (!hlc3ok)
		{
			std::cout << "Unable to normalize hlc3." << std::endl;
		}
		else
		{
			//;hlc3
			for (const ColPos::Dep k : COLS_REGR_DEP_N_STDDEVS)
				xt::row(o_results, k) = vec_norm_n_stddevs(xt::row(o_results, k), hlc3regr, hlc3stddev);

			//;atr, ;past_range, ;range
			for (const ColPos::Dep k : COLS_REGR_DEP_NORM_STDDEV)
				xt::row(o_results, k) = vec_norm_stddevs(xt::row(o_results, k), hlc3stddev);

			/* transform diff / range features */
			//;diff, ;range_t, ;range_b
			for (const ColPos::Diff k : COLS_REGR_DIFF_NORM_STDDEV)
				xt::row(o_results, k) = vec_norm_stddevs(xt::row(o_results, k), hlc3stddev);
		}
	}

}


constexpr std::array<ColPos::TA, 1> COLS_NORM_TA_SCALE_DIV_100 = { ColPos::TA::adx };
constexpr std::array<ColPos::TA, 2> COLS_NORM_TA_SCALE_DIV_10 = { ColPos::TA::ppo, ColPos::TA::ppo_diff };
constexpr std::array<ColPos::TA, 4> COLS_NORM_TA_SCALE_AS_VOL = { ColPos::TA::obv_trend20, ColPos::TA::obv_trend50, ColPos::TA::adi_trend20, ColPos::TA::adi_trend50 };

void do_norm(xt::xarray<real_t>& o_results, const unsigned int interval, const bool has_bid_ask)
{
	// Normalize volume related fields across timeperiods  // TODO don't do this???
	real_t adj_vol_divider =
		interval < 1440 ? interval
		: interval == 1440 ? 390
		: 1950;

	/* Apply normalization that relies on actual close price */

	const auto& r_close = xt::row(o_results, ColPos::In::close);
	const auto& r_hlc3 = xt::row(o_results, ColPos::Dep::hlc3);

	// keep close, with ln applied, allows model to handle price differences
	xt::row(o_results, ColPos::Norm::close_ln) = vec_xfrm_ln(r_close);  //:close_ln
	if (has_bid_ask)
	{
		xt::row(o_results, ColPos::In::ask) = vec_xfrm_ln(xt::row(o_results, ColPos::In::ask));  //:ask
		xt::row(o_results, ColPos::In::bid) = vec_xfrm_ln(xt::row(o_results, ColPos::In::bid));  //:bid
		xt::row(o_results, ColPos::In::ask_high) = vec_xfrm_ln(xt::row(o_results, ColPos::In::ask_high));  //:ask_high
		xt::row(o_results, ColPos::In::ask_low) = vec_xfrm_ln(xt::row(o_results, ColPos::In::ask_low));  //:ask_low
		xt::row(o_results, ColPos::In::bid_high) = vec_xfrm_ln(xt::row(o_results, ColPos::In::bid_high));  //:bid_high
		xt::row(o_results, ColPos::In::bid_low) = vec_xfrm_ln(xt::row(o_results, ColPos::In::bid_low));  //:bid_low
	}

	// calculate hlc3_ln, for volume normalization
	const auto hlc3_ln = vec_xfrm_ln(r_hlc3);
	
	// add volume(normalized with asset price), squared to increase focus on large volume candles
	//;volume, ;volume_buy, ;volume_sell
	xt::row(o_results, ColPos::In::volume) = vec_scale_vol(hlc3_ln, xt::row(o_results, ColPos::In::volume), adj_vol_divider);
	//xt::row(o_results, ColPos::TA::volume_buy) = vec_scale_vol(r_close_ln, xt::row(o_results, ColPos::TA::volume_buy), adj_vol_divider);
	//xt::row(o_results, ColPos::TA::volume_sell) = vec_scale_vol(r_close_ln, xt::row(o_results, ColPos::TA::volume_sell), adj_vol_divider);
	//negate_row(o_results, ColPos::TA::volume_sell);  // volume_sell = -volume_sell

	// normalize OBV/ADI similar as volume
	//;obv_trend20, ;obv_trend50, ;adi_trend20, ;adi_trend50
	for (const ColPos::TA k : COLS_NORM_TA_SCALE_AS_VOL)
		xt::row(o_results, k) = vec_scale_obv_trend(xt::row(o_results, k), adj_vol_divider);

	// TODO bid/ask/etc.
	// for bids / asks, use difference between open and bids / asks
	// for k in['bid_high', 'bid_low']:
	//     dfin[k] = dfin[k] - dfin['open']
	// for k in['ask_high', 'ask_low']:
	//     dfin[k] = dfin['open'] - dfin[k]

	/* Apply regression */

	apply_regr(o_results, interval);  // ~+0 cols, updates 11  // TODO //:close_next_norm  // ~+1 cols, updates 11

	/* Normalize remaining fields */
	// scale rsi to - 1 - 1
	//;rsi
	xt::row(o_results, ColPos::TA::rsi) = vec_scale_rsi(xt::row(o_results, ColPos::TA::rsi));

	//;adx,
	for (const ColPos::TA k : COLS_NORM_TA_SCALE_DIV_100)
		xt::row(o_results, k) = vec_scale_div_100(xt::row(o_results, k));

	//;ppo, ;ppo_diff
	for (const ColPos::TA k : COLS_NORM_TA_SCALE_DIV_10)
		xt::row(o_results, k) = vec_scale_div_10(xt::row(o_results, k));
	
	//// log normalize these inputs
	////cols2 = []
}

xt::xarray<real_t> do_cleanup_initial(const xt::xarray<real_t>& a_in, const unsigned int interval)
{
	const int n_cols = a_in.shape().at(0);
	const int window = INTERVAL_REGR_WINDOW;  // INTERVAL_REGR_WINDOWS.at(static_cast<const int>(interval));
	const auto trim_first = window + 50;
	auto a_view = xt::view(a_in, xt::all(), xt::range(trim_first, a_in.shape().at(1)));
	return xt::xarray<real_t>(a_view);
	/*
    // before applying market inputs, trim oldest incomplete nan/data
    window = mk_window(interval)
    dfin = dfin.iloc[max(window, 50):]  // trim oldest 'window' rows (contains incomplete averages)
	*/
}
void do_cleanup_initial_2a(xt::xarray<timestamp_us_t>& ts_in, xt::xarray<real_t>& a_in, const unsigned int interval)
{
	const int n_cols = a_in.shape().at(0);
	const int window = INTERVAL_REGR_WINDOW;  // INTERVAL_REGR_WINDOWS.at(static_cast<const int>(interval));
	const auto trim_first = window + 50;
	{
		auto ts_view = xt::view(ts_in, xt::all(), xt::range(trim_first, ts_in.shape().at(1)));
		ts_in = xt::xarray<timestamp_us_t>(ts_view);
	}
	{
		auto a_view = xt::view(a_in, xt::all(), xt::range(trim_first, a_in.shape().at(1)));
		a_in = xt::xarray<real_t>(a_view);
	}
	/*
	// before applying market inputs, trim oldest incomplete nan/data
	window = mk_window(interval)
	dfin = dfin.iloc[max(window, 50):]  // trim oldest 'window' rows (contains incomplete averages)
	*/
}

xt::xarray<real_t> do_cleanup_final(xt::xarray<real_t>& a_in)
{
	const int n_cols = a_in.shape().at(0);
	xt::xarray<real_t> results = xt::zeros<real_t>({ n_cols, static_cast<int>(a_in.shape().at(1)) });
	/*
	// remove nan / null values(since may exist after resorting index)
	dfin = dfin[dfin.open.notnull()]

	dfin = dfin.iloc[44:]  // trim oldest 44 rows(was 33, contains empty averages)

	if not for_eval:
		dfin = dfin.iloc[CHECK_PAST_CNT:]
		// dfin = dfin.iloc[(CHECK_PAST_CNT + PAST_CNT) : ]  // note: v2 not using PAST_CNT
		dfin = dfin.iloc[:-CHECK_FUTURE_CNT]  // if training, trim last CHECK_FUTURE_CNT rows too
	*/
	return results;
}

xt::xarray<real_t> do_market_inputs(const xt::xarray<real_t>& a_in)
{
	xt::xarray<real_t> results = xt::zeros<real_t>({ 11, static_cast<int>(a_in.shape().at(1)) });
	return results;
}

void do_outputs(xt::xarray<double>& o_outputs, const char* symbol, const xt::xarray<real_t>& o_results, const xt::xarray<real_t>& a_step1, const unsigned int timeframe, const unsigned int interval)
{
	assert(CHECK_FUTURE_CNT == 7);

	const size_t n_len = o_results.shape().at(1);

	const auto r_ask_low = xt::row(a_step1, ColPos::In::ask_low);
	const auto r_ask_high = xt::row(a_step1, ColPos::In::ask_high);
	const auto r_bid_low = xt::row(a_step1, ColPos::In::bid_low);
	const auto r_bid_high = xt::row(a_step1, ColPos::In::bid_high);
	const auto r_low = xt::row(a_step1, ColPos::In::low);
	const auto r_high = xt::row(a_step1, ColPos::In::high);

	xt::xarray<real_t> windowCols = xt::zeros<real_t>({ CHECK_FUTURE_CNT, static_cast<int>(n_len) });

	// calculate max(future_bid_high) [with current row]
	for (ptrdiff_t i = 0; i < CHECK_FUTURE_CNT; i++)
	{
		if (i)
			xt::row(windowCols, i) = xt::roll(r_bid_high, -i);
		else
			xt::row(windowCols, i) = r_bid_high;
	}
	xt::xarray<real_t> rolling7_bid_high = vec_max7(xt::row(windowCols, 0), xt::row(windowCols, 1), xt::row(windowCols, 2), xt::row(windowCols, 3), xt::row(windowCols, 4), xt::row(windowCols, 5), xt::row(windowCols, 6));

	// calculate min(future_ask_low) [with current row]
	windowCols = xt::zeros<real_t>({ CHECK_FUTURE_CNT, static_cast<int>(n_len) });  // TODO zero this?
	for (ptrdiff_t i = 0; i < CHECK_FUTURE_CNT; i++)
	{
		if (i)
			xt::row(windowCols, i) = xt::roll(r_ask_low, -i);
		else
			xt::row(windowCols, i) = r_ask_low;
	}
	xt::xarray<real_t> rolling7_ask_low = vec_min7(xt::row(windowCols, 0), xt::row(windowCols, 1), xt::row(windowCols, 2), xt::row(windowCols, 3), xt::row(windowCols, 4), xt::row(windowCols, 5), xt::row(windowCols, 6));

	// calculate future_low [excluding current row]
	windowCols = xt::zeros<real_t>({ CHECK_FUTURE_CNT, static_cast<int>(n_len) });  // TODO zero this?
	for (ptrdiff_t i = 0; i < CHECK_FUTURE_CNT; i++)
	{
		xt::row(windowCols, i) = xt::roll(r_low, static_cast<ptrdiff_t>(-(i + 1)));
	}
	xt::xarray<real_t> future_low = vec_min7(xt::row(windowCols, 0), xt::row(windowCols, 1), xt::row(windowCols, 2), xt::row(windowCols, 3), xt::row(windowCols, 4), xt::row(windowCols, 5), xt::row(windowCols, 6));
	
	// calculate future_high [excluding current row]
	windowCols = xt::zeros<real_t>({ CHECK_FUTURE_CNT, static_cast<int>(n_len) });  // TODO zero this?
	for (ptrdiff_t i = 0; i < CHECK_FUTURE_CNT; i++)
	{
		xt::row(windowCols, i) = xt::roll(r_high, static_cast<ptrdiff_t>(-(i + 1)));
	}
	xt::xarray<real_t> future_high = vec_max7(xt::row(windowCols, 0), xt::row(windowCols, 1), xt::row(windowCols, 2), xt::row(windowCols, 3), xt::row(windowCols, 4), xt::row(windowCols, 5), xt::row(windowCols, 6));


	//x_std_dev = current_row['atr'] * 1.777
	xt::xarray<real_t> x_std_dev = vec_multiply(xt::row(o_results, ColPos::Dep::atr), 1.777);

	//h_std_dev = current_row['atr'] * 1.577
	xt::xarray<real_t> h_std_dev = vec_multiply(xt::row(o_results, ColPos::Dep::atr), 1.577);

	//o_std_dev = current_row['atr'] * 1.337
	xt::xarray<real_t> o_std_dev = vec_multiply(xt::row(o_results, ColPos::Dep::atr), 1.337);

	
	// calculate the outputs
	/*ColPos::Output::long_low;    // min(future_ask_low) == ask_low
	ColPos::Output::long1;       // future_high > x_std_dev + current_ask_high
	ColPos::Output::long2;       // future_high > h_std_dev + current_ask_high
	ColPos::Output::long3;       // future_high > o_std_dev + current_ask_high
	ColPos::Output::short_high;  // max(future_bid_high) == bid_high
	ColPos::Output::short1;      // future_low < current_bid_low - x_std_dev
	ColPos::Output::short2;      // future_low < current_bid_low - h_std_dev
	ColPos::Output::short3;      // future_low < current_bid_low - o_std_dev*/

	xt::row(o_outputs, ColPos::Output::ts) = xt::row(a_step1, ColPos::In::timestamp);

	xt::row(o_outputs, ColPos::Output::long_low) = vec_equals(rolling7_ask_low, r_ask_low);
	xt::row(o_outputs, ColPos::Output::short_high) = vec_equals(rolling7_bid_high, r_bid_high);

	xt::row(o_outputs, ColPos::Output::long1) = vec_greater_sum2(future_high, x_std_dev, r_ask_high);
	xt::row(o_outputs, ColPos::Output::long2) = vec_greater_sum2(future_high, h_std_dev, r_ask_high);
	xt::row(o_outputs, ColPos::Output::long3) = vec_greater_sum2(future_high, o_std_dev, r_ask_high);

	xt::row(o_outputs, ColPos::Output::short1) = vec_less_sub2(future_low, r_bid_low, x_std_dev);
	xt::row(o_outputs, ColPos::Output::short2) = vec_less_sub2(future_low, r_bid_low, h_std_dev);
	xt::row(o_outputs, ColPos::Output::short3) = vec_less_sub2(future_low, r_bid_low, o_std_dev);

	/*std::cout << "do_outputs() " << symbol << " long_low = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::long_low)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " short_high = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::short_high)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " long1 = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::long1)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " long2 = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::long2)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " long3 = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::long3)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " short1 = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::short1)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " short2 = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::short2)) / n_len) << "%" << std::endl;
	std::cout << "do_outputs() " << symbol << " short3 = " << (100.0 * xt::sum(xt::row(o_outputs, ColPos::Output::short3)) / n_len) << "%" << std::endl;*/
}
