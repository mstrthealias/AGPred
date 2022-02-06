#ifndef ACCOUNT_ADAPTER_H
#define ACCOUNT_ADAPTER_H


namespace agpred {

	using fn_order_status = std::function<void(id_t order_id, const Symbol& symbol, OrderStatus status, size_t filled, size_t remaining, double avg_price)>;


	class AccountAdapter {
	public:
		virtual ~AccountAdapter() = default;
		virtual void newOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size) = 0;
		virtual void newOrder(const id_t& order_id, const Symbol& symbol, const OrderType& order_type, const size_t& size, const double& limit_price) = 0;
		virtual void cancelOrder(const id_t& order_id) = 0;

		void setCallbacks(const fn_order_status& on_order_status)
		{
			on_order_status_ = on_order_status;
		}



	protected:

		fn_order_status on_order_status_;

	};


}

#endif // ACCOUNT_ADAPTER_H
