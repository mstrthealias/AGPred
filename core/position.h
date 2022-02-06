#ifndef POSITION_H
#define POSITION_H

#include <xlocmon>

#include "core.h"


namespace agpred {


	enum class PositionState : uint8_t
	{
		INITIAL = 0,  // TODO needed?
		//PENDING = 1,
		PARTIAL_FILLED = 1,
		FILLED = 2,
		CLOSING = 3,
		CLOSED = 4,
	};


	class Position {
		const id_t id_;  // TODO id_t = size_t ?  // The original order_id...

		const Symbol& symbol_;

		const EntryData entry_data_;
		
		const PositionType type_;  // TODO type and orderType??
		const OrderType order_type_;  // TODO type and orderType??

		PositionState state_;

		size_t num_shares_;
		size_t num_shares_filled_;
		double avg_price_;
		bool exiting_;

	public:

		explicit Position(const id_t& order_id, const Symbol& symbol, const EntryData& entry_data)
			: id_(order_id),
			symbol_(symbol),
			entry_data_(entry_data),
			type_(entry_data.type),
			order_type_(entry_data.type == PositionType::LONG ? OrderType::BUY : OrderType::SELL),
			state_(PositionState::INITIAL),
			num_shares_(0), num_shares_filled_(0), avg_price_(0),  // TODO default avg_price to -1?
			exiting_(false)
		{
		}

		~Position() = default;

		const id_t& id() const
		{
			return id_;
		}

		const Symbol& symbol() const
		{
			return symbol_;
		}

		const PositionType& type() const
		{
			return type_;
		}

		const size_t& num_shares_filled() const
		{
			return num_shares_filled_;
		}

		const size_t& filled_size() const
		{
			return num_shares_;
		}

		const bool& exiting() const
		{
			return exiting_;
		}

		void setExiting()
		{
			exiting_ = true;
		}

		/**
		 * TODO num_shares is total # of filled shares for order_id
		 * TODO price is average of all fills for order_id
		 * Note: handling as size = total # of filled shares, and price is average of all filled shares...
		 */
		void onFill(const size_t size, const double price)
		{
			if (!size)
				throw std::logic_error("onFill() invalid size");
			if (price <= 0)
				throw std::logic_error("onFill() invalid price");

			num_shares_filled_ = size;
			num_shares_ = size;
			avg_price_ = price;

			if (num_shares_ >= entry_data_.size)
				state_ = PositionState::FILLED;
			else
				state_ = PositionState::PARTIAL_FILLED;
		}

		/**
		 * TODO num_shares is total # of filled shares for order_id
		 * TODO price is average of all fills for order_id
		 * Note: handling as size = total # of filled shares, and price is average of all filled shares...
		 */
		void onExitFill(const size_t size, const double price)
		{
			if (!size)
				throw std::logic_error("onExitFill() invalid size");
			if (price <= 0)
				throw std::logic_error("onExitFill() invalid price");

			num_shares_ = entry_data_.size - size;
			//avg_price_ = price;

			if (num_shares_)
				state_ = PositionState::CLOSING;
			else
				state_ = PositionState::CLOSED;
		}

		bool isFilled() const
		{
			return state_ == PositionState::FILLED;
		}

		bool isClosing() const
		{
			return state_ == PositionState::CLOSING || state_ == PositionState::CLOSED;
		}

		bool isClosed() const
		{
			return state_ == PositionState::CLOSED;
		}

		double getClosedPL(double closed_avg) const
		{
			if (!isClosed())
				throw std::logic_error("position not closed");

			return static_cast<double>(num_shares_filled_) * (closed_avg - avg_price_);
		}
	};


}

#endif // POSITION_H
