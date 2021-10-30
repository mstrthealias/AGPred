#ifndef CONSOLIDATE_H
#define CONSOLIDATE_H


#include <xtensor/xarray.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>

#include "common.h"


using json = nlohmann::json;



enum class TradeCondition : uint32_t
{
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

struct trade_condition_t
{
	TradeCondition id;
	TradeConditionType  type;

	bool updates_volume;
	bool updates_high_low;
	bool updates_open_close;
	bool updates_consolidated_high_low;
	bool updates_consolidated_open_close;
	bool legacy;

	const char* name;
};


const static std::array<const trade_condition_t, 72> conditions = {
	trade_condition_t({TradeCondition::Regular_Sale, TradeConditionType::sale_condition, true, true, true, true, true, false, "Regular Sale"}),
	{TradeCondition::Acquisition, TradeConditionType::sale_condition, true, true, true, true, true, false, "Acquisition"},
	{TradeCondition::Average_Price_Trade, TradeConditionType::sale_condition, true, false, false, false, false, false, "Average Price Trade"},
	{TradeCondition::Automatic_Execution, TradeConditionType::sale_condition, true, true, true, true, true, false, "Automatic Execution"},
	{TradeCondition::Bunched_Trade, TradeConditionType::sale_condition, true, true, true, true, true, false, "Bunched Trade"},
	{TradeCondition::Bunched_Sold_Trade, TradeConditionType::sale_condition, true, true, false, true, false, false, "Bunched Sold Trade"},
	{TradeCondition::CAP_Election, TradeConditionType::sale_condition, true, true, true, true, true, true, "CAP Election"},
	{TradeCondition::Cash_Sale, TradeConditionType::sale_condition, true, false, false, false, false, false, "Cash Sale"},
	{TradeCondition::Closing_Prints, TradeConditionType::sale_condition, true, true, true, true, true, false, "Closing Prints"},
	{TradeCondition::Cross_Trade, TradeConditionType::sale_condition, true, true, true, true, true, false, "Cross Trade"},
	{TradeCondition::Derivatively_Priced, TradeConditionType::sale_condition, true, true, false, true, false, false, "Derivatively Priced"},
	{TradeCondition::Distribution, TradeConditionType::sale_condition, true, true, true, true, true, false, "Distribution"},
	{TradeCondition::Form_T_Extended_Hours, TradeConditionType::sale_condition, true, false, false, false, false, false, "Form T/Extended Hours"},
	{TradeCondition::Extended_Hours_Sold_Out_Of_Sequence, TradeConditionType::sale_condition, true, false, false, false, false, false, "Extended Hours (Sold Out Of Sequence)"},
	{TradeCondition::Intermarket_Sweep, TradeConditionType::sale_condition, true, true, true, true, true, false, "Intermarket Sweep"},

	{TradeCondition::Market_Center_Official_Close, TradeConditionType::sale_condition, false, true, true, false, false, false, "Market Center Official Close"},
	{TradeCondition::Market_Center_Official_Open, TradeConditionType::sale_condition, false, true, false, false, false, false, "Market Center Official Open"},

	{TradeCondition::Market_Center_Opening_Trade, TradeConditionType::sale_condition, true, true, false, true, false, false, "Market Center Opening Trade"},
	{TradeCondition::Market_Center_Reopening_Trade, TradeConditionType::sale_condition, true, true, true, true, true, false, "Market Center Reopening Trade"},
	{TradeCondition::Market_Center_Closing_Trade, TradeConditionType::sale_condition, true, true, true, true, true, false, "Market Center Closing Trade"},
	{TradeCondition::Next_Day, TradeConditionType::sale_condition, true, false, false, false, false, false, "Next Day"},
	{TradeCondition::Price_Variation_Trade, TradeConditionType::sale_condition, true, false, false, false, false, false, "Price Variation Trade"},
	{TradeCondition::Prior_Reference_Price, TradeConditionType::sale_condition, true, true, false, true, false, false, "Prior Reference Price"},
	{TradeCondition::Rule_155_Trade_AMEX, TradeConditionType::sale_condition, true, true, true, true, true, false, "Rule 155 Trade (AMEX)"},
	{TradeCondition::Rule_127_NYSE_Only, TradeConditionType::sale_condition, true, true, true, true, true, false, "Rule 127 (NYSE Only)"},
	{TradeCondition::Opening_Prints, TradeConditionType::sale_condition, true, true, true, true, true, false, "Opening Prints"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::Stopped_Stock_Regular_Trade, TradeConditionType::sale_condition, true, true, true, true, true, true, "Stopped Stock (Regular Trade)"},
	{TradeCondition::ReOpening_Prints, TradeConditionType::sale_condition, true, true, true, true, true, false, "Re-Opening Prints"},
	{TradeCondition::Seller, TradeConditionType::sale_condition, true, false, false, false, false, false, "Seller"},
	{TradeCondition::Sold_Last, TradeConditionType::sale_condition, true, true, true, true, true, false, "Sold Last"},
	{TradeCondition::Sold_Last_and_Stopped_Stock, TradeConditionType::sale_condition, true, true, true, true, true, true, "Sold Last and Stopped Stock"},
	{TradeCondition::Sold_Out_Of_Sequence, TradeConditionType::sale_condition, true, true, false, true, false, false, "Sold (Out Of Sequence)"},
	{TradeCondition::Sold_Out_of_Sequence_and_Stopped_Stock, TradeConditionType::sale_condition, true, true, false, true, false, true, "Sold (Out of Sequence) and Stopped Stock"},
	{TradeCondition::Split_Trade, TradeConditionType::sale_condition, true, true, true, true, true, false, "Split Trade"},
	{TradeCondition::Stock_Option, TradeConditionType::sale_condition, true, true, true, true, true, true, "Stock Option"},
	{TradeCondition::Yellow_Flag_Regular_Trade, TradeConditionType::sale_condition, true, true, true, true, true, false, "Yellow Flag Regular Trade"},
	{TradeCondition::Odd_Lot_Trade, TradeConditionType::sale_condition, true, false, false, false, false, false, "Odd Lot Trade"},
	{TradeCondition::Corrected_Consolidated_Close_per_listing_market, TradeConditionType::sale_condition, false, false, false, true, true, false, "Corrected Consolidated Close (per listing market)"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::Held, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "Held"},
	{TradeCondition::Trade_Thru_Exempt, TradeConditionType::trade_thru_exempt, false, false, false, false, false, false, "Trade Thru Exempt"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::Contingent_Trade, TradeConditionType::sale_condition, true, false, false, false, false, false, "Contingent Trade"},
	{TradeCondition::Qualified_Contingent_Trade, TradeConditionType::sale_condition, true, false, false, false, false, false, "Qualified Contingent Trade"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::Opening_Reopening_Trade_Detail, TradeConditionType::sale_condition, true, true, true, true, true, true, "Opening Reopening Trade Detail"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::Short_Sale_Restriction_Activated, TradeConditionType::short_sale_restriction_indicator, false, false, false, false, false, false, "Short Sale Restriction Activated"},
	{TradeCondition::Short_Sale_Restriction_Continued, TradeConditionType::short_sale_restriction_indicator, false, false, false, false, false, false, "Short Sale Restriction Continued"},
	{TradeCondition::Short_Sale_Restriction_Deactivated, TradeConditionType::short_sale_restriction_indicator, false, false, false, false, false, false, "Short Sale Restriction Deactivated"},
	{TradeCondition::Short_Sale_Restriction_In_Effect, TradeConditionType::short_sale_restriction_indicator, false, false, false, false, false, false, "Short Sale Restriction In Effect"},
	{TradeCondition::PLACEHOLDER, TradeConditionType::PLACEHOLDER, false, false, false, false, false, false, "PLACEHOLDER"},
	{TradeCondition::Financial_Status_Bankrupt, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Bankrupt"},
	{TradeCondition::Financial_Status_Deficient, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Deficient"},
	{TradeCondition::Financial_Status_Delinquent, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Delinquent"},
	{TradeCondition::Financial_Status_Bankrupt_and_Deficient, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Bankrupt and Deficient"},
	{TradeCondition::Financial_Status_Bankrupt_and_Delinquent, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Bankrupt and Delinquent"},
	{TradeCondition::Financial_Status_Deficient_and_Delinquent, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Deficient and Delinquent"},
	{TradeCondition::Financial_Status_Deficient_Delinquent_and_Bankrupt, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Deficient, Delinquent, and Bankrupt"},
	{TradeCondition::Financial_Status_Liquidation, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Liquidation"},
	{TradeCondition::Financial_Status_Creations_Suspended, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Creations Suspended"},
	{TradeCondition::Financial_Status_Redemptions_Suspended, TradeConditionType::financial_status_indicator, false, false, false, false, false, false, "Financial Status - Redemptions Suspended"},
};
static const uint32_t conditions_len = conditions.size();

static const trade_condition_t& regular_trade_condition = conditions.at(0);


enum class SIPMapping : char
{
	at = '@',
	a = 'A',
	w = 'W',
	b = 'B',
	g = 'G',
	c = 'C',
	six = '6',
	x = 'X',
	four = '4',
	d = 'D',
	t = 'T',
	u = 'U',
	f = 'F',
	m = 'M',
	q = 'Q',
	n = 'N',
	h = 'H',
	p = 'P',
	k = 'K',
	o = 'O',
	one = '1',
	five = '5',
	r = 'R',
	l = 'L',
	z = 'Z',
	s = 'S',
	y = 'Y',
	i = 'I',
	nine = '9',
	v = 'V',
	seven = '7',
	e = 'E',
	eight = '8',
};


xt::xarray<double> bars_to_xt(const std::vector<Bar>& bars);


xt::xarray<double> process_trades_json(const char* json_str, const int interval, const double& start_ts);


#endif // CONSOLIDATE_H
