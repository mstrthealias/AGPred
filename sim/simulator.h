#ifndef SIMULATOR_H
#define SIMULATOR_H


#include <map>
#include <algorithm>

#include "simulated_account_adapter.h"
#include "../src/common.h"
#include "../core/core.h"


namespace agpred {

	struct SymbolSimMarket
	{
		double price;
		double bid;
		double ask;
		size_t bid_size;
		size_t ask_size;
	};

	struct WaitingOrder
	{
		//const Symbol& symbol;
		id_t order_id;
		//std::string symbol;
		OrderType order_type;
		size_t size;
		double limit_price;

		size_t filled_size;
		double filled_avg;
		size_t remaining_size;
	};


	class Simulator final : public SimulatedAccountAdapter {
	public:
		Simulator()
		{
		}

		void initSymbol(const Symbol& symbol)
		{
			symbols_market_.emplace(std::pair<std::string, SymbolSimMarket>(symbol.symbol, { 0.0, 0.0, 0.0, 0, 0 }));
		}

		void onSnapshot(const Symbol& symbol, const Snapshot& snapshot)
		{
			SymbolSimMarket& market = symbols_market_[symbol.symbol];
			
			market.price = snapshot.price;
			market.bid = snapshot.nbbo.bid;
			market.ask = snapshot.nbbo.ask;
			market.bid_size = snapshot.nbbo.bid_size * 100;
			market.ask_size = snapshot.nbbo.ask_size * 100;

			// attempt to fill pending limit orders
			if (!pending_limit_orders_.empty())
			{

				// TODO 
				// TODO only do this for symbol pending orders
				// TODO

				//std::cout << "sim.onSnapshot(" << symbol.symbol << ") price=" << market.price << ", ask=" << market.ask << std::endl;

				// fill any limit orders within price
				for (WaitingOrder& order : pending_limit_orders_)
				{
					if (order.order_type == OrderType::BUY && market.ask <= order.limit_price)
						simFillOrder(symbol, order, false);
					else if (order.order_type == OrderType::SELL && market.bid >= order.limit_price)
						simFillOrder(symbol, order, false);
				}

				// remove limit orders that were filled
				pending_limit_orders_.erase(
					std::remove_if(
						pending_limit_orders_.begin(),
						pending_limit_orders_.end(),
						[](const WaitingOrder& o)
						{
							return o.filled_size >= o.size;
						}
					),
					pending_limit_orders_.end()
				);
			}

			// attempt to fill pending market orders
			if (!pending_market_orders_.empty())
			{

				// TODO 
				// TODO only do this for symbol pending orders
				// TODO 

				// attempt to fill all market orders
				for (WaitingOrder& order : pending_market_orders_)
				{
					simFillOrder(symbol, order, true);
				}

				// remove market orders that were filled
				pending_market_orders_.erase(
					std::remove_if(
						pending_market_orders_.begin(),
						pending_market_orders_.end(),
						[](const WaitingOrder& o)
						{
							return o.filled_size >= o.size;
						}
					),
					pending_market_orders_.end()
				);
			}
		}

		void onOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size) override
		{
			// MARKET order
			pending_market_orders_.emplace_back(WaitingOrder{
				order_id,
				//symbol.symbol,
				order_type,
				size,
				0.0,
				0,   //filled_size
				0.0, //filled_avg
				size //remaining_size
			});
		}

		void onOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size, const double& limit_price) override
		{
			// LIMIT order
			pending_limit_orders_.emplace_back(WaitingOrder{
				order_id,
				//symbol.symbol,
				order_type,
				size,
				limit_price,
				0,   //filled_size
				0.0, //filled_avg
				size //remaining_size
			});
		}

		void cancelOrder(const id_t& order_id) override
		{
			// TODO anything else needed here?
			// TODO implement and verify logic mentioned at 'TODO canceled order fill

			if (!pending_limit_orders_.empty())
			{
				// remove limit orders matching this id...
				pending_limit_orders_.erase(
					std::remove_if(
						pending_limit_orders_.begin(),
						pending_limit_orders_.end(),
						[order_id](const WaitingOrder& o)
						{
							return o.order_id == order_id;
						}
					),
					pending_limit_orders_.end()
				);
			}
			if (!pending_market_orders_.empty())
			{
				// remove market orders matching this id...
				pending_market_orders_.erase(
					std::remove_if(
						pending_market_orders_.begin(),
						pending_market_orders_.end(),
						[order_id](const WaitingOrder& o)
						{
							return o.order_id == order_id;
						}
					),
					pending_market_orders_.end()
				);
			}
		}

	private:

		void simFillOrder(const Symbol& symbol, WaitingOrder& order, const bool is_market)
		{
			const SymbolSimMarket& market = symbols_market_[symbol.symbol];

			//std::cout << "sim.simFillOrder(" << symbol.symbol << ") price=" << market.price << ", ask=" << market.ask << std::endl;

			// TODO handle order
			size_t size;
			double avg;
			if (is_market)
			{
				if (order.order_type == OrderType::BUY)
				{
					// fill as many shares as possible at ask
					size = std::min(market.ask_size, order.remaining_size);
					avg = market.ask;
				}
				else
				{
					// fill as many shares as possible at bid
					size = std::min(market.bid_size, order.remaining_size);
					avg = market.bid;
				}
			}
			else
			{
				if (order.order_type == OrderType::BUY && market.ask <= order.limit_price)
				{
					size = std::min(market.ask_size, order.remaining_size);
					avg = market.ask;
				}
				else if (order.order_type == OrderType::SELL && market.bid >= order.limit_price)
				{
					size = std::min(market.bid_size, order.remaining_size);
					avg = market.bid;
				}
				else
				{
					size = 0;
					avg = 0.0;
				}
			}
			if (!size)
			{
				// no fill, return...
				return;
			}

			// calculate average fill price/remaining size
			order.filled_avg = ((static_cast<double>(order.filled_size) * order.filled_avg) + (static_cast<double>(size) * avg)) / static_cast<double>(order.filled_size + size);
			order.filled_size += size;
			assert(order.size >= order.filled_size);
			order.remaining_size = order.size - order.filled_size;

			const OrderStatus status = (order.filled_size < order.size ? OrderStatus::PARTIAL : OrderStatus::FILLED);

			// TODO need to track this
			
			on_order_status_(order.order_id, symbol, status, order.filled_size, order.remaining_size, order.filled_avg);
		}

	private:
		//std::map<Symbol, SymbolSimMarket, _compare_symbol> symbols_market_;  // TODO index by char*?
		std::map<std::string, SymbolSimMarket> symbols_market_;  // TODO index by char*?

		// TODO std::map?
		std::vector<WaitingOrder> pending_limit_orders_;
		std::vector<WaitingOrder> pending_market_orders_;
	};


}

#endif // SIMULATOR_H
