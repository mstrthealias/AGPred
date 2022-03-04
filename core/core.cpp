#include "core.h"


using namespace agpred;


id_t PendingOrder::next_order_id = 1;
std::map<std::string, Symbol> Symbol::symbol_cache = {};


// TODO
const Symbol& Symbol::get_symbol(const std::string& ticker)
{
	if (symbol_cache.find(ticker) == symbol_cache.end())
	{
		// TODO statically implement list of all Symbol to consider ?
		// TODO use REST to download market/symbol data?
		symbol_cache.emplace(ticker, Symbol{ ticker.c_str(), Market::NASDAQ });
	}
	return symbol_cache[ticker];
}

bool Symbol::operator== (const Symbol& b) const
{
	return symbol == b.symbol;
}

bool Symbol::operator!= (const Symbol& b) const
{
	return !(*this == b);
}


/*std::ostream& agpred::operator<< (std::ostream& out, const QuoteData& quote)
{
	out << '{' << std::endl;
	out << '\t' << "quote: " << quote.quote << std::endl;
	out << '\t' << "cond: " << static_cast<int32_t>(quote.cond[0]) << ", " << static_cast<int32_t>(quote.cond[1]) << ", " << static_cast<int32_t>(quote.cond[2]) << std::endl;
	out << '}' << std::endl;
	return out;
}

std::ostream& agpred::operator<< (std::ostream& out, const TradeData& trade)
{
	out << '{' << std::endl;
	out << '\t' << "trade: " << trade.trade << std::endl;
	out << '\t' << "cond: " << static_cast<uint32_t>(trade.cond[0]) << ", " << static_cast<uint32_t>(trade.cond[1]) << ", " << static_cast<uint32_t>(trade.cond[2]) << std::endl;
	out << '}' << std::endl;
	return out;
}*/