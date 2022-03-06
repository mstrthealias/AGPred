#ifndef SIMULATED_ACCOUNT_ADAPTER_H
#define SIMULATED_ACCOUNT_ADAPTER_H


#include "../core/core.h"



namespace agpred {


	class SimulatedAccountAdapter : public AccountAdapter {
	//private:
	//	Simulator& simulator_;

	public:
		explicit SimulatedAccountAdapter()  //(Simulator & simulator)
			: AccountAdapter()//,
			//simulator_(simulator)
		{
		}
		~SimulatedAccountAdapter() override = default;

		virtual void onOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size) = 0;
		virtual void onOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size, const real_t& limit_price) = 0;
		virtual void cancelOrder(const id_t& order_id) override = 0;

		void newOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size) override
		{
			// TODO simulator needs to know state for each symbol

			// MARKET order
			onOrder(order_id, symbol, order_type, size);
			//simulator_.onOrder(symbol, order_type, size);
		}

		void newOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size, const real_t& limit_price) override
		{
			// LIMIT order
			onOrder(order_id, symbol, order_type, size, limit_price);
			//simulator_.onOrder(symbol, order_type, size, limit_price);
		}


	};


}

#endif // SIMULATED_ACCOUNT_ADAPTER_H
