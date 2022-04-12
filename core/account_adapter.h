#ifndef ACCOUNT_ADAPTER_H
#define ACCOUNT_ADAPTER_H


#include "../src/common.h"


namespace agpred {

	using fn_order_status = std::function<void(id_t order_id, const Symbol& symbol, OrderStatus status, size_t filled, size_t remaining, real_t avg_price)>;
	using fn_account_status = std::function<void(id_t request_id, const AccountStatus& status)>;


	class AccountAdapter {
	public:
		virtual ~AccountAdapter() = default;

		/******** GENERAL ********/

		/**
		* Called after callbacks have been set.
		*/
		virtual void initialize() = 0;

		/**
		* Called at construct time an order a pending order.
		*/
		void setCallbacks(const fn_order_status& on_order_status, const fn_account_status& on_account_status)
		{
			on_order_status_ = on_order_status;
			on_account_status_ = on_account_status;
		}


		/******** ACCOUNT ********/

		/**
		* Request account status update.
		*/
		virtual void accountStatus(const id_t& request_id) = 0;

		/**
		* Returns false if the provided entry_data is not permitted.
		*/
		virtual bool allowEntry(const Symbol& symbol, const EntryData& entry_data, const std::map<id_t, Position>& positions) = 0;

		/******** ORDER ********/

		/**
		* Place an order, MARKET if no limit_price.
		*/
		virtual void newOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size) = 0;
		virtual void newOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size, const real_t& limit_price) = 0;

		/**
		* Cancel an order a pending order.
		*/
		virtual void cancelOrder(const id_t& order_id) = 0;
		



	protected:

		/******** CALLBACKS ********/
		/* Callbacks invoke AccountController logic, and are called by the account adapter implementation. */

		fn_order_status on_order_status_;
		fn_account_status on_account_status_;

	};


}

#endif // ACCOUNT_ADAPTER_H
