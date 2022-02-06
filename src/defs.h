#ifndef DEFS_H
#define DEFS_H

#include <map>
#include <vector>

#include "common.h"


// TODO move into core.h?
constexpr int NUM_COLUMNS = 101;
constexpr int NUM_TIMESTEMPS = 17;  // 73;  //33  //27
constexpr int NUM_INTERVALS = 7;

static const int TIMEFRAME = 10080;

[[clang::no_destroy]]
static const std::map<const char*, const int> INTERVAL_MAP = {
	// //{"10sec", 0},
	{"1min", 1},
	{"5min", 5},
	{"15min", 15},
	//{"30min", 30},
	{"1hr", 60},
	//{"2hr", 120},
	{"4hr", 240},
	{"D", 1440},
	{"W", 10080},
};

/*
// TODO use 
static const size_t POS_1MIN = 0;
static const size_t POS_5MIN = 1;
static const size_t POS_15MIN = 2;
static const size_t POS_1HR = 3;
static const size_t POS_4HR = 4;
static const size_t POS_DAY = 5;
static const size_t POS_WK = 6;
*/


static const std::map<const int, const int> INPUTS_PER_CHUNK_MAP = {
	{5, 10240},
	{15, 10240},
	{60, 4864},
	{240, 1408},
	{1440, 640},
	{10080, 320},
};
static const int INPUTS_PER_CHUNK = INPUTS_PER_CHUNK_MAP.at(5);
static const int SHUFFLE_CHUNK_SIZE = 1;

static const int LEN_DEPTH = NUM_TIMESTEMPS;
static const int CHECK_FUTURE_CNT = 7;
static const int CHECK_PAST_CNT = 5;

constexpr int INTERVAL_REGR_WINDOW = 31;
[[clang::no_destroy]]
static const std::map<const int, const int> INTERVAL_REGR_WINDOWS = {
	{0, 195},
	{1, 195},
	{5, 95},
	{15, 90},
	{30, 90},
	{60, 90},
	{120, 45},
	{240, 45},
	{1440, 45},
	{10080, 30},
};

[[clang::no_destroy]]
static const std::array<std::tuple<unsigned int, unsigned int, unsigned int, Timeframes::Pos>, NUM_INTERVALS> INTERVAL_INITIAL_DOWNLOADS = {
	std::make_tuple<unsigned int, unsigned int, unsigned int>(1, 345600, 350, Timeframes::Pos::_1min),  // 1min, 86400*4, 345600, limit=350;  Note: 345600 incase last day is monday and friday is holiday
	std::make_tuple<unsigned int, unsigned int, unsigned int>(5, 432000, 1750, Timeframes::Pos::_5min),  // 5min, 86400*5, 432000, limit=1750
	std::make_tuple<unsigned int, unsigned int, unsigned int>(15, 950400, 5000, Timeframes::Pos::_15min),  // 15min, 86400*11, 950400, limit=5000
	std::make_tuple<unsigned int, unsigned int, unsigned int>(60, 2851200, 50000, Timeframes::Pos::_1hr),  // 1hr, 86400*33, 2851200, limit=50000
	std::make_tuple<unsigned int, unsigned int, unsigned int>(240, 9072000, 50000, Timeframes::Pos::_4hr),  // 4hr, 86400*105, 9072000, limit=50000
	std::make_tuple<unsigned int, unsigned int, unsigned int>(1440, 21600000, 250, Timeframes::Pos::_1day),  // 1day, 86400*250, 21600000, limit=250
	std::make_tuple<unsigned int, unsigned int, unsigned int>(10080, 151200000, 900, Timeframes::Pos::_1wk),  // 1wk, 86400*1750, 151200000, limit=900
};
/*
// Note: interval download requirements:
[[clang::no_destroy]]
static const std::map<const int, const int> INTERVAL_REQUIRED_QUANTITIES = {
	//{0, 255},
	{1, 255},
	{5, 155},
	{15, 175},
	{30, 175},
	{60, 255},
	{120, 205},
	{240, 255},
	{1440, 95},
	{10080, 95},
};
*/

#endif // DEFS_H
