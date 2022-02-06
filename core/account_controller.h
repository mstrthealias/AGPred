#ifndef ACCOUNT_CONTROLLER_H
#define ACCOUNT_CONTROLLER_H

#include <map>
#include <array>
#include <vector>
#include <queue>
#include <nlohmann/json.hpp>

#include "core.h"
#include "position.h"
#include "strategy.h"
#include "account_adapter.h"
#include "ring_buffers.h"


using json = nlohmann::json;


namespace agpred {
	
	void onUpdateLog(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed);

	
	template <size_t Algos, size_t Entries, size_t Exits>
	class AccountController {
	private:
		AccountAdapter& adapter_;

		const AGMode mode_;
		
		const std::array<AlgoBase* const, Algos> algos_;
		const std::array<EntryBase* const, Entries> entries_;
		const std::array<ExitBase* const, Exits> exits_;
		
		std::map<id_t, Position> positions_;

		std::map<Symbol, std::map<id_t, PendingEntry>, _compare_symbol> symbol_pending_entries_;  // TODO remove?
		std::map<id_t, PendingEntry> pending_entries_;  // TODO remove or ^?
		std::map<id_t, PendingExit> pending_exits_;

		double total_pl = 0.0;
		size_t total_trades = 0;

	public:
		AccountController(AccountAdapter& adapter, const AGMode mode, const std::array<AlgoBase* const, Algos>& algos, const std::array<EntryBase* const, Entries>& entries, const std::array<ExitBase* const, Exits>& exits)
			: adapter_(adapter), mode_(mode), algos_(algos), entries_(entries), exits_(exits)
		{
			adapter.setCallbacks(
				[AccountPtr = this](id_t order_id, const Symbol& symbol, OrderStatus status, size_t filled, size_t remaining, double avg_price)
				{
					AccountPtr->onOrderStatus(order_id, symbol, status, filled, remaining, avg_price);
				}
			);
		}

		void onSnapshot(const Symbol& symbol, const Snapshot& snapshot)
		{
			// TODO call exits with snapshot
			for (auto& p : positions_)
			{
				Position& position = p.second;
			}
		}
		
		void onUpdate(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
		{
			onUpdateLog(symbol, snapshot, data, data_processed);

			// TODO loop positions, only perform exit logic when a position exists...

			// handle algo exits
			{
				// loop exits to track unique algos
				std::map<size_t, const AlgoBase*> algo_map;
				for (const ExitBase* exit : exits_)
				{
					// TODO using dynamic_cast to determine sub-class; maybe track the exits separately...
					const AlgoExitBase* algo_exit = dynamic_cast<const AlgoExitBase*>(exit);
					if (algo_exit != nullptr)
					{
						algo_map[algo_exit->algo_.id_] = &(algo_exit->algo_);
					}
				}

				// loop unique algos, map result by algo id
				std::map<size_t, bool> algo_result_map;
				for (const auto& pair : algo_map)
				{
					algo_result_map[pair.first] = pair.second->operator()(snapshot, data, data_processed);
				}

				// loop exits, check if algo result was true
				for (const ExitBase* exit : exits_)
				{

					// TODO move PositionType:type into ExitBase?

					const AlgoExitBase* algo_exit = dynamic_cast<const AlgoExitBase*>(exit);
					if (algo_exit != nullptr && algo_result_map[algo_exit->algo_.id_])
					{
						// algo result true, exit position...
						ExitData exit_data = (*algo_exit)(symbol, snapshot);

						// loop all entries for this symbol that do not have a position/have not been filled
						if (symbol_pending_entries_.count(symbol))
						{
							for (auto& p : symbol_pending_entries_.at(symbol))
							{
								if (!positions_.count(p.first))
									cancelPendingPosition(symbol, p.first);
							}
						}

						// TODO loop/exit all positions on this symbol?

						for (auto& p : positions_)
						{
							Position& position = p.second;
							if (!position.exiting() && position.type() == exit_data.type && position.symbol() == symbol)
								exitPosition(position, exit_data);
						}

					}
				}
			}


			// handle algo entries
			{
				// loop entries to track unique algos
				std::map<size_t, const AlgoBase*> algo_map;
				for (const auto entry : entries_)
				{
					algo_map[entry->algo_.id_] = &(entry->algo_);
				}

				// loop unique algos, map result by algo id
				std::map<size_t, bool> algo_result_map;
				for (const auto& pair : algo_map)
				{
					algo_result_map[pair.first] = pair.second->operator()(snapshot, data, data_processed);
				}

				// loop entries, check if algo result was true
				for (const auto entry : entries_)
				{
					if (algo_result_map[entry->algo_.id_])
					{
						// algo result true, do entry...
						EntryData entry_data = (*entry)(symbol, snapshot);
						enterPosition(symbol, entry_data);
					}
				}
			}

		}

		/**
		 * @param order_id A unique order ID, set by client when creating the order (fe. next_order_id)
		 */
		void onOrderStatus(id_t order_id, const Symbol& symbol, OrderStatus status, size_t filled, size_t remaining, double avg_price)
		{
			// TODO
			bool is_entry = false;
			bool is_exit = false;
			bool is_long = false;

			if (pending_exits_.count(order_id))
			{
				is_exit = true;
				const PendingExit& exit = pending_exits_.at(order_id);
				is_long = exit.exit_data.type == PositionType::LONG;
				onExitFill(symbol, exit, filled, avg_price);
			}

			if (pending_entries_.count(order_id))
			{
				is_entry = true;
				const PendingEntry& entry = pending_entries_.at(order_id);
				is_long = entry.entry_data.type == PositionType::LONG;
				onEntryFill(symbol, entry, filled, avg_price);
			}

			if (!is_exit && !is_entry)
			{
				std::cout << "> onOrderStatus(" << symbol.symbol << ", " << order_id << ") TODO canceled order fill" << std::endl;
				// TODO receive order status for order that we canceled
				//throw std::logic_error("invalid order_id");
				// TODO if it filled anything, immediately create opposite order?
			}
			else
			{
				const bool is_filled = status == OrderStatus::FILLED;
				std::cout << "> onOrderStatus(" << symbol.symbol << ", " << order_id << ", "
					<< (is_exit ? (is_filled ? "Closed" : "Partial Close") : (is_filled ? "Filled" : "Partial Fill"))
					<< (is_long ? " LONG " : " SHORT ")
					<< filled << " @ $" << avg_price << ")" << std::endl;
			}

		}

		double getProfitLoss() const
		{
			return total_pl;
		}

		size_t getNumTrades() const
		{
			return total_trades;
		}
		
		Position& getPosition(const id_t& position_id)
		{
			return positions_.at(position_id);
		}
		
		bool hasPosition(const id_t& position_id) const
		{
			return static_cast<bool>(positions_.count(position_id));
		}


	private:

		void enterPosition(const Symbol& symbol, const EntryData& entry_data)
		{
			if (!entry_data.size)
				throw std::logic_error("invalid size");

			const bool is_long = entry_data.type == PositionType::LONG;  // position is long, not the exit...
			const OrderType order_type = is_long ? OrderType::BUY : OrderType::SELL;
			const PendingEntry entry = PendingEntry::fromEntryData(entry_data);

			std::cout << "enterPosition(" << symbol.symbol << ", " << (is_long ? "LONG" : "SHORT") << ", " << entry.order_id << ") " << (is_long ? "BUY " : "SELL ") << entry_data.size << " @ $" << entry_data.limit_price << std::endl;

			pending_entries_.emplace(std::pair<id_t, PendingEntry>(entry.order_id, entry));
			if (!symbol_pending_entries_.count(symbol))
				symbol_pending_entries_.emplace(std::pair<Symbol, std::map<id_t, PendingEntry>>(symbol, {}));
			symbol_pending_entries_.at(symbol).emplace(std::pair<id_t, PendingEntry>(entry.order_id, entry));
			
			if (entry_data.limit_price > 0)
				adapter_.newOrder(entry.order_id, symbol, order_type, entry_data.size, entry_data.limit_price);
			else
				adapter_.newOrder(entry.order_id, symbol, order_type, entry_data.size);
		}

		void exitPosition(Position& position, const ExitData& exit_data)
		{
			if (position.exiting())
				throw std::logic_error("position already exiting");
			position.setExiting();

			const bool is_long = exit_data.type == PositionType::LONG;  // position is long, not the exit...
			const PendingExit exit = PendingExit::fromExitData(position.id(), exit_data);

			std::cout << "exitPosition(" << position.symbol().symbol << ", " << (is_long ? "LONG" : "SHORT") << ", " << position.id() << ", " << exit.order_id << ") " << (is_long ? "SELL " : "BUY ") << position.filled_size() << std::endl;  //" @ $" << exit_data.limit_price << std::endl;

			pending_exits_.emplace(std::pair<id_t, PendingExit>(exit.order_id, exit));
			
			// TODO check pending_entries_ for pending entry that should be canceled/removed
			if (pending_entries_.count(position.id()))
			{
				// cancel unfilled order...
				cancelPendingPosition(position.symbol(), position.id());
			}

			// order_type is opposite of position type...
			const OrderType order_type = is_long ? OrderType::SELL : OrderType::BUY;

			if (exit_data.limit_price > 0)
				adapter_.newOrder(exit.order_id, position.symbol(), order_type, position.filled_size(), exit_data.limit_price);
			else
				adapter_.newOrder(exit.order_id, position.symbol(), order_type, position.filled_size());
		}

		void cancelPendingPosition(const Symbol& symbol, const id_t& order_id)
		{
			// cancel the order...
			adapter_.cancelOrder(order_id);

			// note: position.id() == order_id == pending_entry.order_id
			//const PendingEntry& pending_entry = pending_entries_.at(order_id);
			pending_entries_.erase(order_id);
			symbol_pending_entries_.at(symbol).erase(order_id);

			std::cout << "cancelPendingPosition(" << symbol.symbol << ", " << order_id << ") " << order_id << std::endl;
		}

		/**
		 * TODO num_shares is total # of filled shares for order_id
		 * TODO price is average of all fills for order_id
		 * Note: handling as num_shares = total # of filled shares, and price is average of all filled shares...
		 */
		void onEntryFill(const Symbol& symbol, const PendingEntry& pending_entry, const size_t& num_shares, const double& price)
		{
			// TODO

			// TODO improve these, instead of getting the position after create it; maybe use std::map
			if (!hasPosition(pending_entry.order_id))
			{
				// create new position/add position to the positions array
				positions_.emplace(std::pair<id_t, Position>(pending_entry.order_id, Position( pending_entry.order_id, symbol, pending_entry.entry_data )));
			}

			// have a partially or fully filled entry
			Position& position = getPosition(pending_entry.order_id);

			position.onFill(num_shares, price);

			if (position.isFilled())
			{
				// position is filled, remove from pending_entries_
				pending_entries_.erase(pending_entry.order_id);
				symbol_pending_entries_.at(symbol).erase(pending_entry.order_id);
			}
			//else {}  // have a partially filled entry
		}

		void onExitFill(const Symbol& symbol, const PendingExit& pending_exit, const size_t& num_shares, const double& price)
		{
			if (!hasPosition(pending_exit.position_id))
				throw std::logic_error("position for exit does not exist");

			Position& position = getPosition(pending_exit.position_id);

			if (static_cast<int64_t>(position.num_shares_filled()) - static_cast<int64_t>(num_shares) < 0)
				throw std::logic_error("too many exit shares filled...");

			position.onExitFill(num_shares, price);

			if (position.isClosed())
			{
				// position is closed, remove from positions_ AND pending_exits_
				if (!pending_exits_.erase(pending_exit.order_id))
					throw std::logic_error("invalid pending exit");
				if (!positions_.erase(pending_exit.position_id))
					throw std::logic_error("invalid position");

				const double pl = position.getClosedPL(price);
				total_pl += pl;
				total_trades++;
				std::cout << "  onExitFill(" << symbol.symbol << ") closed PL $" << pl << std::endl;
			}
		}


	};
}

#endif // ACCOUNT_CONTROLLER_H
