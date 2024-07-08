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
	

	using fn_symbol_positions_closed = std::function<void(const Symbol& symbol)>;


	void onUpdateLog(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed);


	/**
	 * The internal Exit implementation used for handling EntryData::stoploss
	 */
	class _SystemStopLossExit final : public SnapshotExitBase {
	public:
		explicit _SystemStopLossExit(const std::string& name) : SnapshotExitBase(name)
		{
		}
		~_SystemStopLossExit() = default;

		bool call(const Position& position, const Snapshot& snapshot) const override
		{
			const auto& stoploss = position.entry_data().stoploss;
			if (stoploss <= 0)
				return false;
			return (position.type() == PositionType::LONG && snapshot.nbbo.bid <= stoploss)
				|| (position.type() == PositionType::SHORT && snapshot.nbbo.ask >= stoploss);
		}

		ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			std::cout << "_SystemStopLossExit : ExitData CALL()" << std::endl;
			return ExitData{ position.type(), 0.0 };
		}
	};

	
	template <size_t Algos, size_t Entries, size_t Exits, size_t MaxSignals>
	class AccountController {
	public:
		static constexpr size_t max_signal_size = MaxSignals;

	private:
		AccountAdapter& adapter_;

		const AGMode mode_;
		
		const std::array<AlgoBase2<max_signal_size>* const, Algos> algos_;
		const std::array<EntryBase<max_signal_size>* const, Entries> entries_;
		const std::array<ExitBase* const, Exits> exits_;

		std::optional<AccountStatusRequest> pending_status_;

		_SystemStopLossExit internal_stoploss_exit_;
		
		std::map<id_t, Position> positions_;

		std::map<Symbol, std::map<id_t, PendingEntry>, _compare_symbol> symbol_pending_entries_;  // TODO remove?
		std::map<id_t, PendingEntry> pending_entries_;  // TODO remove or ^?
		std::map<id_t, PendingExit> pending_exits_;

		real_t total_pl = 0.0;
		size_t total_trades = 0;

		real_t max_trade_loss_ = 0.0;
		real_t max_daily_loss_ = 0.0;

		fn_symbol_positions_closed on_positions_closed_;

	public:
		AccountController(AccountAdapter& adapter, const AGMode mode, const std::array<AlgoBase2<max_signal_size>* const, Algos>& algos, const std::array<EntryBase<max_signal_size>* const, Entries>& entries, const std::array<ExitBase* const, Exits>& exits)
			: adapter_(adapter), mode_(mode), algos_(algos), entries_(entries), exits_(exits), internal_stoploss_exit_("_internal_stoploss_ exit")
		{
			adapter.setCallbacks(
				[AccountPtr = this](const timestamp_us_t& ts, id_t order_id, const Symbol& symbol, OrderStatus status, size_t filled, size_t remaining, real_t avg_price)
				{
					AccountPtr->onOrderStatus(ts, order_id, symbol, status, filled, remaining, avg_price);
				},
				[AccountPtr = this](id_t request_id, const AccountStatus& status)
				{
					AccountPtr->onAccountStatus(request_id, status);
				}
			);

			initAdapter();
		}

		size_t closePositions(const Symbol& symbol, const std::optional<PositionType>& type = std::nullopt, fn_symbol_positions_closed on_positions_closed = {})
		{
			std::cout << "closePositions(" << symbol.symbol << ")" << std::endl;

			size_t req_close_cnt = 0;

			on_positions_closed_ = on_positions_closed;

			// exit any pending entries
			if (symbol_pending_entries_.contains(symbol))
			{
				for (auto& p : symbol_pending_entries_.at(symbol))
				{
					if (!type.has_value() || type.value() == p.second.entry_data.type) {
						cancelPendingPosition(symbol, p.first);
						//++req_close_cnt;  // TODO onOrderCanceled callback?
					}
				}
			}

			// exit any filled/partially-filled positions
			for (auto& p : positions_)
			{
				Position& position = p.second;
				if (position.exiting() || position.symbol() != symbol)
					continue;
				else if (type.has_value() && type.value() != position.type())
					continue;
				// exit position...
				exitPosition(position, {
					position.type(),
					0.0  // MARKET order
				});
				++req_close_cnt;
			}

			if (!req_close_cnt && on_positions_closed)
			{
				on_positions_closed_ = {};  // always clear this callback
				on_positions_closed(symbol);
			}

			return req_close_cnt;
		}

		/*size_t closePositions()
		{
			std::cout << "closePositions()" << std::endl;

			// called when day finishes, closes any open positions...

			// TODO track symbols, loop and call closePositions()

			return 0;
		}*/

		void onSnapshot(const Symbol& symbol, const Snapshot& snapshot)
		{
			// handle snapshot exists
			{
				// loop positions, check all snapshot exists
				for (auto& p : positions_)
				{
					Position& position = p.second;
					if (position.exiting() || position.symbol() != symbol)
						continue;

					// run the system stop loss check
					if (position.hasStoploss() && internal_stoploss_exit_.call(position, snapshot))
					{
						// exit result true, exit position...
						ExitData exit_data = internal_stoploss_exit_(position, symbol, snapshot);
						if (position.type() == exit_data.type)
						{
							exitPosition(position, exit_data);
							continue;
						}
					}

					// loop snapshot exists and call to check for exit
					std::map<size_t, const AlgoBase2<max_signal_size>*> algo_map;
					for (const ExitBase* exit : exits_)
					{
						// TODO using dynamic_cast to determine sub-class; maybe track the exits separately...
						const SnapshotExitBase* snapshot_exit = dynamic_cast<const SnapshotExitBase*>(exit);
						if (snapshot_exit != nullptr && snapshot_exit->call(position, snapshot))
						{
							// exit result true, exit position...
							ExitData exit_data = snapshot_exit->operator()(position, symbol, snapshot);
							if (position.type() == exit_data.type)
								exitPosition(position, exit_data);
						}
					}
				}
			}
		}
		
		void onUpdate(const Symbol& symbol, const Snapshot& snapshot, const xtensor_ts_interval& data_ts, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
		{
			onUpdateLog(symbol, snapshot, data, data_processed);

			// TODO loop positions, only perform exit logic when a position exists...

			// loop exits and entries to track unique algos
			std::map<size_t, const AlgoBase2<max_signal_size>*> algo_map;
			// loop exits to track unique algos
			for (const ExitBase* exit : exits_)
			{
				// TODO using dynamic_cast to determine sub-class; maybe track the exits separately...
				const AlgoExitBase<max_signal_size>* algo_exit = dynamic_cast<const AlgoExitBase<max_signal_size>*>(exit);
				if (algo_exit != nullptr)
				{
					algo_map[algo_exit->algo_.id()] = &(algo_exit->algo_);
				}
			}
			// loop entries to track unique algos
			for (const auto entry : entries_)
			{
				algo_map[entry->algo_.id()] = &(entry->algo_);
			}

			// loop unique algos, run algo/map result by algo id
			std::map<size_t, std::array<bool, max_signal_size>> algo_result_map;
			for (const auto& pair : algo_map)
			{
				algo_result_map[pair.first] = pair.second->operator()(snapshot, data, data_processed, quotes, trades);
			}
			
			// handle algo exits
			{
				// loop exits, check if algo result was true
				for (const ExitBase* exit : exits_)
				{

					// TODO move PositionType:type into ExitBase?

					const AlgoExitBase<max_signal_size>* algo_exit = dynamic_cast<const AlgoExitBase<max_signal_size>*>(exit);
					if (algo_exit != nullptr && algo_result_map[algo_exit->algo_.id()][algo_exit->pos()])
					{
						// exit algo evaluated true: exit 
						
						// loop all entries for this symbol that do not have a position/have not been filled
						if (symbol_pending_entries_.contains(symbol))
						{
							for (auto& p : symbol_pending_entries_.at(symbol))
							{
								// TODO always cancel all pending entries?
								// TODO track position type on ExitBase/algo, instead of in exit_data?
								cancelPendingPosition(symbol, p.first);
								//if (!positions_.contains(p.first))
								//	cancelPendingPosition(symbol, p.first);
							}
						}

						// TODO loop/exit all positions on this symbol?

						for (auto& p : positions_)
						{
							Position& position = p.second;
							if (position.exiting() || position.symbol() != symbol)
								continue;
							// algo result true, exit position...
							ExitData exit_data = algo_exit->operator()(position, symbol, snapshot);
							if (position.type() == exit_data.type)
								exitPosition(position, exit_data);
						}
					}
				}
			}


			// handle algo entries
			{
				// loop entries, check if algo result was true
				for (const auto entry : entries_)
				{
					if (algo_result_map[entry->algo_.id()][entry->pos()])
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
		void onAccountStatus(id_t request_id, const AccountStatus& status)
		{
			if (pending_status_.has_value()) {
				if (pending_status_->request_id == request_id) {
					pending_status_.reset();
				}
				else {
					std::cout << "onAccountStatus() received invalid status update/incorrect request_id" << std::endl;
					return;
				}
			}
			else {
				std::cout << "onAccountStatus() unexpected status update received" << std::endl;
				return;
			}

			std::cout << "onAccountStatus() " << request_id << ", balance $" << status.account_balance << std::endl;

			max_daily_loss_ = status.max_daily_loss;
			max_trade_loss_ = status.max_trade_loss;
			// TODO

		}


		/**
		 * @param order_id A unique order ID, set by client when creating the order (fe. next_order_id)
		 */
		void onOrderStatus(const timestamp_us_t& ts, id_t order_id, const Symbol& symbol, OrderStatus status, size_t filled, size_t remaining, real_t avg_price)
		{
			// TODO
			bool is_entry = false;
			bool is_exit = false;
			bool is_long = false;

			if (pending_exits_.contains(order_id))
			{
				is_exit = true;
				const PendingExit& exit = pending_exits_.at(order_id);
				is_long = exit.exit_data.type == PositionType::LONG;
				onExitFill(ts, symbol, exit, filled, avg_price);
			}

			if (pending_entries_.contains(order_id))
			{
				is_entry = true;
				const PendingEntry& entry = pending_entries_.at(order_id);
				is_long = entry.entry_data.type == PositionType::LONG;
				onEntryFill(ts, symbol, entry, filled, avg_price);
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

		real_t getProfitLoss() const
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
			return positions_.contains(position_id);
		}

		size_t getSymbolPositionCnt(const Symbol& symbol) const
		{
			size_t cnt = 0;
			for (const auto& p : positions_)
			{
				const Position& position = p.second;
				if (position.symbol() == symbol)
					++cnt;
			}
			return cnt;
		}


	private:


		void initAdapter()
		{
			adapter_.initialize();
			// TODO onReadyCallback?

			if (mode_ != AGMode::DOWNLOAD_ONLY) {
				// request account status/balance
				pending_status_.emplace(AccountStatusRequest::nextRequest());
				adapter_.accountStatus(pending_status_->request_id);
			}
		}

		void entryBlocked(const Symbol& symbol, const EntryData& entry_data, const bool is_daily_loss)
		{
			const bool is_long = entry_data.type == PositionType::LONG;  // position is long, not the exit...
			if (is_daily_loss) {
				std::cout << "entryBlocked(" << symbol.symbol << ", " << (is_long ? "LONG" : "SHORT") << ") DAILY LOSS EXCEEDED " << entry_data.size << std::endl;
			}
			else {
				std::cout << "entryBlocked(" << symbol.symbol << ", " << (is_long ? "LONG" : "SHORT") << ") BLOCKED " << entry_data.size << std::endl;
			}
		}

		void enterPosition(const Symbol& symbol, const EntryData& entry_data)
		{
			if (!entry_data.size)
				throw std::logic_error("invalid size");

			const bool is_long = entry_data.type == PositionType::LONG;  // position is long, not the exit...
			const OrderType order_type = is_long ? OrderType::BUY : OrderType::SELL;
			const PendingEntry entry = PendingEntry::fromEntryData(entry_data);

			if (total_pl < -max_daily_loss_) {
				entryBlocked(symbol, entry_data, true);
				return;
			}
			if (!adapter_.allowEntry(symbol, entry_data, positions_))
			{
				entryBlocked(symbol, entry_data, false);
				return;
			}
			std::cout << "enterPosition(" << symbol.symbol << ", " << (is_long ? "LONG" : "SHORT") << ", " << entry.order_id << ") " << (is_long ? "BUY " : "SELL ") << entry_data.size << " @ $" << entry_data.limit_price << std::endl;

			pending_entries_.emplace(std::pair<id_t, PendingEntry>(entry.order_id, entry));
			if (!symbol_pending_entries_.contains(symbol))
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
			if (pending_entries_.contains(position.id()))
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
		void onEntryFill(const timestamp_us_t& ts, const Symbol& symbol, const PendingEntry& pending_entry, const size_t& num_shares, const real_t& price)
		{
			// TODO

			// TODO improve these, instead of getting the position after create it; maybe use std::map
			if (!hasPosition(pending_entry.order_id))
			{
				// create new position/add position to the positions array
				positions_.emplace(std::pair<id_t, Position>(pending_entry.order_id, Position{ pending_entry.order_id, symbol, pending_entry.entry_data, ts }));  // Position(
			}

			// have a partially or fully filled entry
			Position& position = getPosition(pending_entry.order_id);

			position.onFill(num_shares, price, ts);

			if (position.isFilled())
			{
				// position is filled, remove from pending_entries_
				pending_entries_.erase(pending_entry.order_id);
				symbol_pending_entries_.at(symbol).erase(pending_entry.order_id);
			}
			//else {}  // have a partially filled entry
		}

		void onExitFill(const timestamp_us_t& ts, const Symbol& symbol, const PendingExit& pending_exit, const size_t& num_shares, const real_t& price)
		{
			if (!hasPosition(pending_exit.position_id))
				throw std::logic_error("position for exit does not exist");

			Position& position = getPosition(pending_exit.position_id);

			if (static_cast<int64_t>(position.num_shares_filled()) - static_cast<int64_t>(num_shares) < 0)
				throw std::logic_error("too many exit shares filled...");

			position.onExitFill(num_shares, price, ts);

			if (position.isClosed())
			{
				// position is closed, remove from positions_ AND pending_exits_
				if (!pending_exits_.erase(pending_exit.order_id))
					throw std::logic_error("invalid pending exit");
				if (!positions_.erase(pending_exit.position_id))
					throw std::logic_error("invalid position");

				const real_t pl = position.getClosedPL(price);
				total_pl += pl;
				total_trades++;
				std::cout << "  onExitFill(" << symbol.symbol << ") closed PL $" << st_real_clr(pl) << std::endl;

				if (on_positions_closed_ && (positions_.empty() || !getSymbolPositionCnt(symbol)))
				{
					// fire on_positions_closed_ callback if set, 
					auto fn_on_closed = on_positions_closed_;
					on_positions_closed_ = {};  // always clear this callback
					fn_on_closed(symbol);
				}
			}
		}

	};
}

#endif // ACCOUNT_CONTROLLER_H
