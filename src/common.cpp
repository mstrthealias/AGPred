#include "common.h"


std::ostream& operator<< (std::ostream& out, const std::vector<real_t>& v)
{
    out << '[';  // (std::is_floating_point<T>::value ? '[' : '(');
    if (!v.empty()) {
        const auto vSize = v.size();
        for (std::vector<real_t>::size_type i = 0; i < vSize; ++i)
            std::cout << v[i] << (i == vSize - 1 ? "" : ", ");
    }
    out << ']';  // (std::is_floating_point<T>::value ? ']' : ')');
    return out;
}

std::ostream& operator<< (std::ostream& out, const std::vector<int>& s)
{
    out << '(';
    std::copy(s.cbegin(), s.cend(), std::ostream_iterator<int>(out, ", "));
    out << "\b\b)";
    return out;
}

std::ostream& operator<< (std::ostream& out, const xt::svector<size_t>& s)
{
    out << '(';
    std::copy(s.cbegin(), s.cend(), std::ostream_iterator<size_t>(out, ", "));
    out << "\b\b)";
    return out;
}

std::ostream& operator<< (std::ostream& out, const std::vector<std::vector<size_t>>& v)
{
	out << '[';
	if (!v.empty()) {
		const auto vSize = v.size();
		for (std::vector<std::vector<size_t>>::size_type i = 0; i < vSize; ++i)
			std::cout << v[i] << (i == vSize - 1 ? "" : ", ");
	}
	out << ']';
	return out;
}

std::ostream& operator<< (std::ostream& out, const Quote& quote)
{
	out << '{' << std::endl;
	out << '\t' << "timestamp: " << quote.timestamp << std::endl;
	out << '\t' << "ask: " << quote.ask << std::endl;
	out << '\t' << "ask_size: " << quote.ask_size << std::endl;
	out << '\t' << "bid: " << quote.bid << std::endl;
	out << '\t' << "bid_size: " << quote.bid_size << std::endl;
	out << '}' << std::endl;
	return out;
}

std::ostream& operator<< (std::ostream& out, const Trade& trade)
{
	out << '{' << std::endl;
	out << '\t' << "timestamp: " << trade.timestamp << std::endl;
	out << '\t' << "price: " << trade.price << std::endl;
	out << '\t' << "size: " << trade.size << std::endl;
	out << '}' << std::endl;
	return out;
}

std::ostream& operator<< (std::ostream& out, const Bar& bar)
{
	out << '{' << std::endl;
	out << '\t' << "timestamp: " << bar.timestamp << std::endl;
	out << '\t' << "open: " << bar.open << std::endl;
	out << '\t' << "high: " << bar.high << std::endl;
	out << '\t' << "low: " << bar.low << std::endl;
	out << '\t' << "close: " << bar.close << std::endl;
	out << '\t' << "volume: " << bar.volume << std::endl;
	out << '\t' << "ask_high: " << bar.ask_high << std::endl;
	out << '\t' << "ask_low: " << bar.ask_low << std::endl;
	out << '\t' << "ask: " << bar.ask << std::endl;
	out << '\t' << "ask_size: " << bar.ask_size << std::endl;
	out << '\t' << "bid_high: " << bar.bid_high << std::endl;
	out << '\t' << "bid_low: " << bar.bid_low << std::endl;
	out << '\t' << "bid: " << bar.bid << std::endl;
	out << '\t' << "bid_size: " << bar.bid_size << std::endl;
	out << '}' << std::endl;
	return out;
}

std::ostream& operator<< (std::ostream& out, const BarFullRef& bar)
{
	out << '{' << std::endl;
	out << '\t' << "timestamp: " << bar.timestamp << std::endl;
	out << '\t' << "open: " << bar.open << std::endl;
	out << '\t' << "high: " << bar.high << std::endl;
	out << '\t' << "low: " << bar.low << std::endl;
	out << '\t' << "close: " << bar.close << std::endl;
	out << '\t' << "volume: " << bar.volume << std::endl;
	out << '\t' << "ask_high: " << bar.ask_high << std::endl;
	out << '\t' << "ask_low: " << bar.ask_low << std::endl;
	out << '\t' << "ask: " << bar.ask << std::endl;
	out << '\t' << "ask_size: " << bar.ask_size << std::endl;
	out << '\t' << "bid_high: " << bar.bid_high << std::endl;
	out << '\t' << "bid_low: " << bar.bid_low << std::endl;
	out << '\t' << "bid: " << bar.bid << std::endl;
	out << '\t' << "bid_size: " << bar.bid_size << std::endl;
	out << '}' << std::endl;
	return out;
}

std::ostream& operator<< (std::ostream& out, const BarRef& bar)
{
	out << '{' << std::endl;
	out << '\t' << "timestamp: " << bar.timestamp << std::endl;
	out << '\t' << "open: " << bar.open << std::endl;
	out << '\t' << "high: " << bar.high << std::endl;
	out << '\t' << "low: " << bar.low << std::endl;
	out << '\t' << "close: " << bar.close << std::endl;
	out << '\t' << "volume: " << bar.volume << std::endl;
	out << '}' << std::endl;
	return out;
}
