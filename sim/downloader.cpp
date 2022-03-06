#include "downloader.h"

#include <fstream>

#include <xtensor/xnpy.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>


using namespace agpred;


constexpr bool DEBUG_DOWNLOADER = false;

constexpr bool STAGED_INCLUDE_TIMESTAMPS = false;
constexpr size_t STAGED_SIZE = 500;
constexpr size_t STAGED_DELAY_OFFSET = 8;
constexpr size_t STAGED_COL_ADJ = STAGED_INCLUDE_TIMESTAMPS ? 0 : 1;


using shape_staged_processed_t = xt::xshape<STAGED_SIZE, NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS - STAGED_COL_ADJ>;  // TODO move NUM_INTERVALS to end?
using shape_staged_trades_t = xt::xshape<STAGED_SIZE, NUM_TRADES, NUM_TRADE_COLUMNS - STAGED_COL_ADJ>;  // (stages, timesteps, cols)
using shape_staged_quotes_t = xt::xshape<STAGED_SIZE, NUM_QUOTES, NUM_QUOTE_COLUMNS - STAGED_COL_ADJ>;  // (stages, timesteps, cols)
using shape_staged_outputs_t = xt::xshape<STAGED_SIZE, ColPos::_OUTPUT_NUM_COLS - STAGED_COL_ADJ>;

using xtensor_staged_processed = xt::xtensor_fixed<real_t, shape_staged_processed_t>;
using xtensor_staged_trades = xt::xtensor_fixed<real_t, shape_staged_trades_t>;
using xtensor_staged_quotes = xt::xtensor_fixed<real_t, shape_staged_quotes_t>;
using xtensor_staged_outputs = xt::xtensor_fixed<real_t, shape_staged_outputs_t>;

xtensor_quotes cur_quotes;
xtensor_trades cur_trades;

xtensor_staged_processed staged_proccessed = xt::zeros<real_t>(shape_staged_processed_t());
xtensor_staged_trades staged_trades = xt::zeros<real_t>(shape_staged_trades_t());
xtensor_staged_quotes staged_quotes = xt::zeros<real_t>(shape_staged_quotes_t());
xtensor_staged_outputs staged_outputs = xt::zeros<real_t>(shape_staged_outputs_t());


void agpred::Downloader::onSimComplete(const Symbol& symbol) 
{
	if (cur_pos_ > STAGED_DELAY_OFFSET)
	{
		do_flush(symbol);
	}
}

void agpred::Downloader::onUpdate(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades, const xtensor_outputs_interval& outputs)
{
	//(NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS)

	auto nan_sum = xt::sum(xt::isnan(data))(0) + xt::sum(xt::isnan(data_processed))(0) + xt::sum(xt::isnan(outputs))(0);
	if (nan_sum > 0) {
		std::cout << "SKIP onUpdate() " << nan_sum << " nan value(s) found" << std::endl;
		return;
	}

	//if (DEBUG_PRINT_DATA)
	//std::cout << "onUpdate(" << symbol.symbol << ") data_processed.shape:" << std::endl << data_processed.shape() << std::endl;

	/* // TODO looks like this can be removed...
	// TODO if the first and last timestamp in processed data is 0, skip this entry
	{
		//std::cout << "data_processed.shape() " << data_processed.shape() << std::endl;  // (7, 17, 101)
		auto proc_first_ts = data_processed(0, 0, 0);
		if (proc_first_ts <= 0.0)
			return;  // TODO throw? TODO assert?

		auto proc_last_ts = data_processed(0, NUM_TIMESTEMPS - 1, 0);
		if (proc_last_ts <= 0.0)
			return;  // TODO throw? TODO assert?
	}*/

	if (static_cast<int64_t>(cur_pos_) - static_cast<int64_t>(STAGED_DELAY_OFFSET) >= 0) {
		//// insert the valid output into staged_outputs[cur_pos_ - STAGED_DELAY_OFFSET]
		//assert(static_cast<int64_t>(cur_pos_) - static_cast<int64_t>(STAGED_DELAY_OFFSET) >= 0);  // must have shifted cur_pos_?

		// the last 8 rowsof outputs are invalid...
		xt::xarray<real_t> outputs_last_valid = xt::view(outputs, outputs.shape().at(0) - (STAGED_DELAY_OFFSET + 1), xt::all());  // TODO minus 1?

		auto stage_outputs = xt::view(staged_outputs, cur_pos_ - STAGED_DELAY_OFFSET, xt::all());
		if constexpr (STAGED_INCLUDE_TIMESTAMPS) {
			std::copy(outputs_last_valid.cbegin(), outputs_last_valid.cend(), stage_outputs.begin());
		}
		else {
			auto tmp_outputs_last_valid = xt::view(outputs_last_valid, xt::range(1, NUM_COLUMNS));
			std::copy(tmp_outputs_last_valid.cbegin(), tmp_outputs_last_valid.cend(), stage_outputs.begin());
		}
	}

	// copy quotes into staging array
	{
		// note: most recent quotes at bottom (order asc)
		quotes.toTensor(cur_quotes);

		auto stage_quotes = xt::view(staged_quotes, cur_pos_, xt::all(), xt::all());
		if constexpr (STAGED_INCLUDE_TIMESTAMPS) {
			std::copy(cur_quotes.cbegin(), cur_quotes.cend(), stage_quotes.begin());
		}
		else {
			auto tmp_cur_quotes = xt::view(cur_quotes, xt::all(), xt::range(1, NUM_QUOTE_COLUMNS));
			std::copy(tmp_cur_quotes.cbegin(), tmp_cur_quotes.cend(), stage_quotes.begin());
		}
	}

	// copy trades into staging array
	{
		// note: most recent trades at bottom (order asc)
		trades.toTensor(cur_trades);

		auto stage_trades = xt::view(staged_trades, cur_pos_, xt::all(), xt::all());
		if constexpr (STAGED_INCLUDE_TIMESTAMPS) {
			std::copy(cur_trades.cbegin(), cur_trades.cend(), stage_trades.begin());
		}
		else {
			auto tmp_cur_trades = xt::view(cur_trades, xt::all(), xt::range(1, NUM_TRADE_COLUMNS));
			std::copy(tmp_cur_trades.cbegin(), tmp_cur_trades.cend(), stage_trades.begin());
		}
	}

	// copy processed data into staging array
	{
		auto stage_processed = xt::view(staged_proccessed, cur_pos_, xt::all(), xt::all(), xt::all());
		if constexpr (STAGED_INCLUDE_TIMESTAMPS) {
			std::copy(data_processed.cbegin(), data_processed.cend(), stage_processed.begin());
		}
		else {
			// create view without timestamps, copy from it...
			auto tmp_data_processed = xt::view(data_processed, xt::all(), xt::all(), xt::range(1, NUM_COLUMNS));
			std::copy(tmp_data_processed.cbegin(), tmp_data_processed.cend(), stage_processed.begin());
		}
	}


	if constexpr (DEBUG_DOWNLOADER)
	{
		if (static_cast<int64_t>(cur_pos_) - static_cast<int64_t>(STAGED_DELAY_OFFSET) >= 0) {
			//auto stage_proc_at_valid_output = xt::view(staged_proccessed, cur_pos_ - STAGED_DELAY_OFFSET, xt::all(), xt::all(), xt::all());
			//std::cout << "stage_proc_at_valid_output.shape() " << xt::xarray<real_t>(stage_proc_at_valid_output).shape() << std::endl;
			//std::cout << "stage_proc_avo_1min_last TS: " << static_cast<timestamp_t>(stage_proc_at_valid_output(0, NUM_TIMESTEMPS - 1, 0)) << std::endl;

			// Note outputs copied into this row: (cur_pos_ - STAGED_DELAY_OFFSET)
			//auto stage_outputs = xt::view(staged_outputs, cur_pos_ - STAGED_DELAY_OFFSET, xt::all());
			//std::cout << "stage_outputs.shape() " << xt::xarray<real_t>(stage_outputs).shape() << std::endl;
			//std::cout << "stage_outputs TS: " << static_cast<timestamp_t>(stage_outputs(0)) << std::endl;
		}
	}


	if (++cur_pos_ >= STAGED_SIZE) {
		do_flush(symbol);
	}

}

// TODO symbol o.0
void agpred::Downloader::do_flush(const Symbol& symbol)
{
	std::cout << "FLUSH: " << std::to_string(cur_pos_) << " stages to disk..." << std::endl;

	// export the first (STAGED_SIZE - STAGED_DELAY_OFFSET) entries
	auto staged_size = std::min(cur_pos_, STAGED_SIZE);
	auto last_stage = staged_size - STAGED_DELAY_OFFSET;
	{
		std::string file = "E:/_data/_v3/" + file_prefix_ + "." + std::to_string(cur_stage_) + ".exports" + (STAGED_INCLUDE_TIMESTAMPS ? ".ts" : "") + ".npy";

		std::ofstream fout(file, std::ios::binary);
		if (!fout.is_open())
			throw std::runtime_error("Unable to open file " + file);

		auto export_processed = xt::view(staged_proccessed, xt::range(0, last_stage), xt::all(), xt::all(), xt::all());
		auto export_trades = xt::view(staged_trades, xt::range(0, last_stage), xt::all(), xt::all());
		auto export_quotes = xt::view(staged_quotes, xt::range(0, last_stage), xt::all(), xt::all());
		auto export_outputs = xt::view(staged_outputs, xt::range(0, last_stage), xt::all());

		xt::detail::dump_npy_stream(fout, export_processed);
		xt::detail::dump_npy_stream(fout, export_trades);
		xt::detail::dump_npy_stream(fout, export_quotes);
		xt::detail::dump_npy_stream(fout, export_outputs);
	}

	// shift the not-exported records to the front
	{
		auto adtl_processed = xt::view(staged_proccessed, xt::range(last_stage, staged_size), xt::all(), xt::all(), xt::all());
		std::copy(adtl_processed.cbegin(), adtl_processed.cend(), staged_proccessed.begin());
	}
	{
		auto adtl_trades = xt::view(staged_trades, xt::range(last_stage, staged_size), xt::all(), xt::all());
		std::copy(adtl_trades.cbegin(), adtl_trades.cend(), staged_trades.begin());
	}
	{
		auto adtl_quotes = xt::view(staged_quotes, xt::range(last_stage, staged_size), xt::all(), xt::all());
		std::copy(adtl_quotes.cbegin(), adtl_quotes.cend(), staged_quotes.begin());
	}
	{
		auto adtl_outputs = xt::view(staged_outputs, xt::range(last_stage, staged_size), xt::all());  // TODO can skip this since it will be overwritten...
		std::copy(adtl_outputs.cbegin(), adtl_outputs.cend(), staged_outputs.begin());
	}

	////std::cout << "adtl_processed" << adtl_processed << std::endl;
	//std::cout << "rem_processed" << xt::view(staged_proccessed, xt::range(0, STAGED_DELAY_OFFSET), xt::all(), xt::all(), xt::all()) << std::endl;
	//std::cout << "rem_processed.shape" << xt::view(staged_proccessed, xt::range(0, STAGED_DELAY_OFFSET), xt::all(), xt::all(), xt::all()).shape() << std::endl;

	// reset cur_pos_ to the next incomplete position
	cur_pos_ = STAGED_DELAY_OFFSET;
	++cur_stage_;

}

