#include "consolidate.h"

#include <pybind11/pybind11.h>            // Pybind11 import to define Python bindings
#include <pybind11/iostream.h>
#define FORCE_IMPORT_ARRAY
#include <xtensor-python/pyarray.hpp>     // Numpy bindings

#include "preprocess.h"


namespace py = pybind11;


xt::pyarray<double> py_process_trades_json(const char* json_str, const int interval, const double& start_ts)
{
	return process_trades_json(json_str, interval, start_ts);
}

xt::pyarray<double> py_preprocess_single(const char* symbol, const xt::pyarray<double>& a_orig, const int timeframe, const int interval, const bool ext_hours)
{
	const bool training = timeframe == interval;

	xt::xarray<double> a_step1 = process_step1_single(symbol, a_orig, training, timeframe, interval, ext_hours);

	xt::xarray<double> o_results = process_step2_single(symbol, a_orig, training, timeframe, interval, ext_hours);

	// TODO outputs ...
	// TODO skip creating outputs if !trainig?
	xt::xarray<double> o_outputs = xt::zeros<double>({ static_cast<int>(ColPos::_OUTPUT_NUM_COLS), static_cast<int>(a_step1.shape().at(1)) });  // TODO timestamp and/or close in outputs?

	process_step3_single(o_results, o_outputs, symbol, a_step1, training, timeframe, interval, ext_hours);

	return o_results;
}



PYBIND11_MODULE(agen_xt_api, m)
{
	xt::import_numpy();
	m.doc() = "Alphagen XTensor python bindings";

	m.def("process_trades_json", py_process_trades_json, "Convert Trades History into interval aggregates");

	m.def("process_trades_json_debug", [](const char* json_str, const int interval, const double& start_ts) -> auto
	{
		py::scoped_ostream_redirect redir(std::cout, py::module::import("sys").attr("stdout"));
		return py_process_trades_json(json_str, interval, start_ts);
	});

	m.def("preprocess_single", py_preprocess_single, "Preprocess a single timestamp/interval");

	m.def("preprocess_single_debug", [](const char* symbol, const xt::pyarray<double>& a_orig, const int timeframe, const int interval, const bool ext_hours) -> auto
	{
		py::scoped_ostream_redirect redir(std::cout, py::module::import("sys").attr("stdout"));
		return py_preprocess_single(symbol, a_orig, timeframe, interval, ext_hours);
	});
}

