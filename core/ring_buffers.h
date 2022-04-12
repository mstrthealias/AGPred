#ifndef TMP_RING_BUFFER
#define TMP_RING_BUFFER

#include <algorithm>
#include <array>
#include <execution>

#include <xtensor/xio.hpp>
#include <xtensor/xvectorize.hpp>
#include <xtensor/xview.hpp>

#include "../src/common.h"
#include "../src/util.h"
#include "core.h"


constexpr bool DEBUG_RING = false;


static const agpred::xtensor_quotes zeros_quotes = xt::zeros<real_t>(agpred::shape_quotes_t());
static const agpred::xtensor_trades zeros_trades = xt::zeros<real_t>(agpred::shape_trades_t());


/**
 * quotes_ring implements a circular buffer with a queue-like interface for QuoteData
 */
template <size_t _Size = 15000, typename _Ttime = real_t, typename _Treal = real_t, typename _Tsize = real_t, class _Tcond = real_t>
class quotes_ring
{
public:
    using value_type = agpred::QuoteData;
    using size_type = size_t;

    const size_t max_size = _Size;
    const size_t max_internal_size = 2 * _Size;


    _NODISCARD bool empty() const noexcept {  // strengthened
        return start_ >= end_;
    }

    _NODISCARD size_type size() const noexcept {  // strengthened 
        return end_ - start_;
    }

    // TODO value_type ?
    void push(const agpred::QuoteData& _Val) {
        if (end_ >= max_internal_size)
        {
            assert(start_ != 0);  // start_ should not be 0

            // at end of ring, roll to start_=0

            // end:
            //   - .cbegin() + end_ - 1
            //   - should be same as .cend()

            std::copy(std::execution::seq, timestamp_.cbegin() + start_, timestamp_.cend(), timestamp_.begin());
            std::copy(std::execution::seq, bid_.cbegin() + start_, bid_.cend(), bid_.begin());
            std::copy(std::execution::seq, ask_.cbegin() + start_, ask_.cend(), ask_.begin());
            std::copy(std::execution::seq, bid_size_.cbegin() + start_, bid_size_.cend(), bid_size_.begin());
            std::copy(std::execution::seq, ask_size_.cbegin() + start_, ask_size_.cend(), ask_size_.begin());
            std::copy(std::execution::seq, cond1_.cbegin() + start_, cond1_.cend(), cond1_.begin());
            std::copy(std::execution::seq, cond2_.cbegin() + start_, cond2_.cend(), cond2_.begin());
            std::copy(std::execution::seq, cond3_.cbegin() + start_, cond3_.cend(), cond3_.begin());

            const auto prev_start = start_;
            const auto prev_end = end_;
            end_ -= start_;
            start_ = 0;

            if (DEBUG_RING)
				std::cout << "push() roll start->end " << prev_start << "->" << prev_end << " TO " << start_ << "->" << end_ << std::endl;
        }

    	if (end_ - start_ >= max_size)  // TODO while?
        {
            // buffer is full:
            //   - remove from front (increment start_; queue size remains same)
            //   - insert at the end

            const auto prev_start = start_;
            const auto prev_end = end_;

            ++start_;
            
            if (DEBUG_RING)
				std::cout << "push() full start->end " << prev_start << "->" << prev_end << " TO " << start_ << "->" << end_ << std::endl;
        }

        const auto bid = static_cast<_Treal>((_Val.quote.bid <= 0 ? 0.0f : std::log(_Val.quote.bid)));
        const auto ask = static_cast<_Treal>((_Val.quote.ask <= 0 ? 0.0f : std::log(_Val.quote.ask)));

        timestamp_[end_] = static_cast<_Ttime>(static_cast<real_t>(_Val.quote.timestamp) / 1000.0);
        bid_[end_] = bid;
        ask_[end_] = ask;
        bid_size_[end_] = static_cast<_Tsize>(std::cbrt(_Val.quote.bid_size * bid));
        ask_size_[end_] = static_cast<_Tsize>(std::cbrt(_Val.quote.ask_size * ask));
        cond1_[end_] = static_cast<_Tcond>(static_cast<int32_t>(_Val.cond[0]));
        cond2_[end_] = static_cast<_Tcond>(static_cast<int32_t>(_Val.cond[1]));
        cond3_[end_] = static_cast<_Tcond>(static_cast<int32_t>(_Val.cond[2]));

        ++end_;

        if (DEBUG_RING)
            std::cout << "push() pushed start->end " << start_ << "->" << end_ << std::endl;
    }

    // TODO value_type ?
    void push(agpred::QuoteData&& _Val) {
        const agpred::QuoteData quote = std::move(_Val);
        push(quote);
    }
    
    template <class... _Valty>
    decltype(auto) emplace(_Valty&&... _Val) {
        push(std::forward<_Valty>(_Val)...);
    }

    /*template <class... _Valty>
    decltype(auto) emplace(_Valty&&... _Val) {
#if _HAS_CXX17
        return push(std::forward<_Valty>(_Val)...);
#else // ^^^ C++17 or newer / C++14 vvv
        push(std::forward<_Valty>(_Val)...);
#endif // _HAS_CXX17
    }*/
    
    void pop() /*noexcept(noexcept(c.pop_front()))*/ {  // strengthened  TODO noexcept?
        // TODO zero removed ?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        ++start_;
        
        if (DEBUG_RING)
            std::cout << "pop() popped start->end " << start_ << "->" << end_ << std::endl;
    }


    _NODISCARD agpred::QuoteData front()  {  // strengthened  TODO noexcept?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        // TODO:
        /*
#if _CONTAINER_DEBUG_LEVEL > 0
        _STL_VERIFY(_My_data._Myfirst != _My_data._Mylast, "front() called on empty vector");
#endif // _CONTAINER_DEBUG_LEVEL > 0
        */

        return {
            {
                static_cast<timestamp_t>(timestamp_[start_]),
                static_cast<real_t>(bid_[start_]),
                static_cast<real_t>(ask_[start_]),
                static_cast<uint32_t>(bid_size_[start_]),
                static_cast<uint32_t>(ask_size_[start_])
            },
            {
                static_cast<QuoteCondition>(static_cast<int32_t>(cond1_[start_])),
                static_cast<QuoteCondition>(static_cast<int32_t>(cond2_[start_])),
                static_cast<QuoteCondition>(static_cast<int32_t>(cond3_[start_])) } };
    }
    
    _NODISCARD agpred::QuoteData back() {  // strengthened  TODO noexcept?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        const auto pos = end_ - 1;
        return {
            {
                static_cast<timestamp_t>(timestamp_[pos]),
                static_cast<real_t>(bid_[pos]),
                static_cast<real_t>(ask_[pos]),
                static_cast<uint32_t>(bid_size_[pos]),
                static_cast<uint32_t>(ask_size_[pos])
            },
            {
                static_cast<QuoteCondition>(static_cast<int32_t>(cond1_[pos])),
                static_cast<QuoteCondition>(static_cast<int32_t>(cond2_[pos])),
                static_cast<QuoteCondition>(static_cast<int32_t>(cond3_[pos])) } };
    }

    // _NODISCARD _CONSTEXPR17 const_iterator begin() const noexcept {
    //     return const_iterator(timestamp_, bid_, ask_, bid_size_, ask_size_, start_, 0);
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_iterator end() const noexcept {
    //     return const_iterator(timestamp_, bid_, ask_, bid_size_, ask_size_, start_, end_);
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator rbegin() const noexcept {
    //     return const_reverse_iterator(end());
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator rend() const noexcept {
    //     return const_reverse_iterator(begin());
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_iterator cbegin() const noexcept {
    //     return begin();
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_iterator cend() const noexcept {
    //     return end();
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator crbegin() const noexcept {
    //     return rbegin();
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator crend() const noexcept {
    //     return rend();
    // }

    /*void swap(queue& _Right) noexcept(_Is_nothrow_swappable<_Container>::value) {
        _Swap_adl(c, _Right.c);
    }*/

    void toTensor(agpred::xtensor_quotes& quotes) const
    {
        // // TODO test an iterator with this...
        // size_t i = 0;
        // for (const agpred::QuoteData& quote : *this)
        // {
        //     quotes(i, 0) = static_cast<real_t>(quote.quote.timestamp / 1000);
        //     quotes(i, 1) = quote.quote.bid;
        //     quotes(i, 2) = quote.quote.ask;
        //     quotes(i, 3) = static_cast<real_t>(quote.quote.bid_size);
        //     quotes(i, 4) = static_cast<real_t>(quote.quote.ask_size);
        //     quotes(i, 5) = static_cast<real_t>(static_cast<int32_t>(quote.cond[0]));
        //     quotes(i, 6) = static_cast<real_t>(static_cast<int32_t>(quote.cond[1]));
        //     quotes(i, 7) = static_cast<real_t>(static_cast<int32_t>(quote.cond[2]));
        //     ++i;
        // }

        if (static_cast<int64_t>(end_) - static_cast<int64_t>(start_) < static_cast<int64_t>(NUM_QUOTES)) {
            // zero entire quotes array if end_ - start_ < NUM_QUOTES?
            std::copy(zeros_quotes.cbegin(), zeros_quotes.cend(), quotes.begin());
        }

        auto timestamp = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(0));
        auto bid = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(1));
        auto ask = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(2));
        auto bid_size = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(3));
        auto ask_size = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(4));
        auto cond1 = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(5));
        auto cond2 = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(6));
        auto cond3 = xt::view(quotes, xt::all(), static_cast<ptrdiff_t>(7));

        std::copy(timestamp_.cbegin() + start_, timestamp_.cbegin() + end_, timestamp.begin());
        std::copy(bid_.cbegin() + start_, bid_.cbegin() + end_, bid.begin());
        std::copy(ask_.cbegin() + start_, ask_.cbegin() + end_, ask.begin());
        std::copy(bid_size_.cbegin() + start_, bid_size_.cbegin() + end_, bid_size.begin());
        std::copy(ask_size_.cbegin() + start_, ask_size_.cbegin() + end_, ask_size.begin());
        std::copy(cond1_.cbegin() + start_, cond1_.cbegin() + end_, cond1.begin());
        std::copy(cond2_.cbegin() + start_, cond2_.cbegin() + end_, cond2.begin());
        std::copy(cond3_.cbegin() + start_, cond3_.cbegin() + end_, cond3.begin());

        //std::cout << "toTensor: quotes=" << std::endl << quotes << std::endl;
        //std::cout << "toTensor: quotes.shape=" << std::endl << quotes.shape() << std::endl;
    }

private:
    size_t start_ = 0;  // inclusive start
    size_t end_ = 0;  // exclusive end
    
    std::array<_Ttime, 2 * _Size> timestamp_ = {};
    std::array<_Treal, 2 * _Size> bid_ = {};
    std::array<_Treal, 2 * _Size> ask_ = {};
    std::array<_Tsize, 2 * _Size> bid_size_ = {};
    std::array<_Tsize, 2 * _Size> ask_size_ = {};
    std::array<_Tcond, 2 * _Size> cond1_ = {};
    std::array<_Tcond, 2 * _Size> cond2_ = {};
    std::array<_Tcond, 2 * _Size> cond3_ = {};
};


/**
 * trades_ring implements a circular buffer with a queue-like interface for TradeData
 */
template <size_t _Size = 15000, typename _Ttime = real_t, typename _Treal = real_t, typename _Tsize = real_t, class _Tcond = real_t>
class trades_ring
{
public:
    using value_type = agpred::TradeData;
    using size_type = size_t;

    const size_t max_size = _Size;
    const size_t max_internal_size = 2 * _Size;

    // using const_iterator = ring_const_iterator<value_type, _Ttime, _Treal, _Tsize, _Size>;
    // using const_reverse_iterator = _STD reverse_iterator<const_iterator>;


    _NODISCARD bool empty() const noexcept {  // strengthened
        return start_ >= end_;
    }

    _NODISCARD size_type size() const noexcept {  // strengthened 
        return end_ - start_;
    }

    // TODO value_type ?
    void push(const agpred::TradeData& _Val) {
        if (end_ >= max_internal_size)
        {
            assert(start_ != 0);  // start_ should not be 0

            // at end of ring, roll to start_=0

            // end:
            //   - .cbegin() + end_ - 1
            //   - should be same as .cend()

            std::copy(std::execution::seq, timestamp_.cbegin() + start_, timestamp_.cend(), timestamp_.begin());
            std::copy(std::execution::seq, price_.cbegin() + start_, price_.cend(), price_.begin());
            std::copy(std::execution::seq, size_.cbegin() + start_, size_.cend(), size_.begin());
            std::copy(std::execution::seq, cond1_.cbegin() + start_, cond1_.cend(), cond1_.begin());
            std::copy(std::execution::seq, cond2_.cbegin() + start_, cond2_.cend(), cond2_.begin());
            std::copy(std::execution::seq, cond3_.cbegin() + start_, cond3_.cend(), cond3_.begin());

            const auto prev_start = start_;
            const auto prev_end = end_;
            end_ -= start_;
            start_ = 0;

            if (DEBUG_RING)
                std::cout << "push() roll start->end " << prev_start << "->" << prev_end << " TO " << start_ << "->" << end_ << std::endl;
        }

        if (end_ - start_ >= max_size)  // TODO while?
        {
            // buffer is full:
            //   - remove from front (increment start_; queue size remains same)
            //   - insert at the end

            const auto prev_start = start_;
            const auto prev_end = end_;

            ++start_;

            if (DEBUG_RING)
                std::cout << "push() full start->end " << prev_start << "->" << prev_end << " TO " << start_ << "->" << end_ << std::endl;
        }

        const auto price = static_cast<_Treal>((_Val.trade.price <= 0 ? 0.0f : std::log(_Val.trade.price)));
        timestamp_[end_] = static_cast<_Ttime>(static_cast<real_t>(_Val.trade.timestamp) / 1000.0);
        price_[end_] = price;
        size_[end_] = static_cast<_Tsize>(std::cbrt(_Val.trade.size * price));
        cond1_[end_] = static_cast<_Tcond>(static_cast<uint32_t>(_Val.cond[0]));
        cond2_[end_] = static_cast<_Tcond>(static_cast<uint32_t>(_Val.cond[1]));
        cond3_[end_] = static_cast<_Tcond>(static_cast<uint32_t>(_Val.cond[2]));

        ++end_;

        if (DEBUG_RING)
            std::cout << "push() pushed start->end " << start_ << "->" << end_ << std::endl;
    }

    // TODO value_type ?
    void push(agpred::TradeData&& _Val) {
        const agpred::TradeData quote = std::move(_Val);
        push(quote);
    }

    template <class... _Valty>
    decltype(auto) emplace(_Valty&&... _Val) {
        push(std::forward<_Valty>(_Val)...);
    }

    /*template <class... _Valty>
    decltype(auto) emplace(_Valty&&... _Val) {
#if _HAS_CXX17
        return push(std::forward<_Valty>(_Val)...);
#else // ^^^ C++17 or newer / C++14 vvv
        push(std::forward<_Valty>(_Val)...);
#endif // _HAS_CXX17
    }*/

    void pop() /*noexcept(noexcept(c.pop_front()))*/ {  // strengthened  TODO noexcept?
        // TODO zero removed ?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        ++start_;

        if (DEBUG_RING)
            std::cout << "pop() popped start->end " << start_ << "->" << end_ << std::endl;
    }


    _NODISCARD agpred::TradeData front() {  // strengthened  TODO noexcept?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        // TODO:
        /*
#if _CONTAINER_DEBUG_LEVEL > 0
        _STL_VERIFY(_My_data._Myfirst != _My_data._Mylast, "front() called on empty vector");
#endif // _CONTAINER_DEBUG_LEVEL > 0
        */

        return {
            {
                static_cast<timestamp_t>(timestamp_[start_]),
                static_cast<real_t>(price_[start_]),
                static_cast<uint32_t>(size_[start_])
            },
            { static_cast<TradeCondition>(static_cast<uint32_t>(cond1_[start_])),
                static_cast<TradeCondition>(static_cast<uint32_t>(cond2_[start_])),
                static_cast<TradeCondition>(static_cast<uint32_t>(cond3_[start_])) } };
    }

    _NODISCARD agpred::TradeData back() {  // strengthened  TODO noexcept?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        const auto pos = end_ - 1;
        return {
            {
                static_cast<timestamp_t>(timestamp_[pos]),
                static_cast<real_t>(price_[pos]),
                static_cast<uint32_t>(size_[pos])
            },
            { static_cast<TradeCondition>(static_cast<uint32_t>(cond1_[pos])),
                static_cast<TradeCondition>(static_cast<uint32_t>(cond2_[pos])),
                static_cast<TradeCondition>(static_cast<uint32_t>(cond3_[pos])) } };
    }

    // _NODISCARD _CONSTEXPR17 const_iterator begin() const noexcept {
    //     return const_iterator(timestamp_, price_, size_, start_, 0);
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_iterator end() const noexcept {
    //     return const_iterator(timestamp_, price_, size_, start_, end_);
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator rbegin() const noexcept {
    //     return const_reverse_iterator(end());
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator rend() const noexcept {
    //     return const_reverse_iterator(begin());
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_iterator cbegin() const noexcept {
    //     return begin();
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_iterator cend() const noexcept {
    //     return end();
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator crbegin() const noexcept {
    //     return rbegin();
    // }
    //
    // _NODISCARD _CONSTEXPR17 const_reverse_iterator crend() const noexcept {
    //     return rend();
    // }

    /*void swap(queue& _Right) noexcept(_Is_nothrow_swappable<_Container>::value) {
        _Swap_adl(c, _Right.c);
    }*/

    void toTensor(agpred::xtensor_trades& trades) const
    {
        if (static_cast<int64_t>(end_) - static_cast<int64_t>(start_) < static_cast<int64_t>(NUM_QUOTES)) {
            // zero entire trades array if end_ - start_ < NUM_TRADES?
            std::copy(zeros_trades.cbegin(), zeros_trades.cend(), trades.begin());
        }

        auto timestamp = xt::view(trades, xt::all(), static_cast<ptrdiff_t>(0));
        auto price = xt::view(trades, xt::all(), static_cast<ptrdiff_t>(1));
        auto size = xt::view(trades, xt::all(), static_cast<ptrdiff_t>(2));
        auto cond1 = xt::view(trades, xt::all(), static_cast<ptrdiff_t>(3));
        auto cond2 = xt::view(trades, xt::all(), static_cast<ptrdiff_t>(4));
        auto cond3 = xt::view(trades, xt::all(), static_cast<ptrdiff_t>(5));

        std::copy(timestamp_.cbegin() + start_, timestamp_.cbegin() + end_, timestamp.begin());
        std::copy(price_.cbegin() + start_, price_.cbegin() + end_, price.begin());
        std::copy(size_.cbegin() + start_, size_.cbegin() + end_, size.begin());
        std::copy(cond1_.cbegin() + start_, cond1_.cbegin() + end_, cond1.begin());
        std::copy(cond2_.cbegin() + start_, cond2_.cbegin() + end_, cond2.begin());
        std::copy(cond3_.cbegin() + start_, cond3_.cbegin() + end_, cond3.begin());

        //std::cout << "toTensor: trades=" << std::endl << trades << std::endl;
        //std::cout << "toTensor: trades.shape=" << std::endl << trades.shape() << std::endl;
    }

private:
    size_t start_ = 0;  // inclusive start
    size_t end_ = 0;  // exclusive end

    std::array<_Ttime, 2 * _Size> timestamp_ = {};
    std::array<_Treal, 2 * _Size> price_ = {};
    std::array<_Tsize, 2 * _Size> size_ = {};
    std::array<_Tcond, 2 * _Size> cond1_ = {};
    std::array<_Tcond, 2 * _Size> cond2_ = {};
    std::array<_Tcond, 2 * _Size> cond3_ = {};
};


// TODO consolidate?
using quotes_queue = quotes_ring<NUM_QUOTES, real_t, real_t, real_t, real_t>;
using trades_queue = trades_ring<NUM_TRADES, real_t, real_t, real_t, real_t>;
//using quotes_queue = quotes_ring<NUM_QUOTES, timestamp_t, real_t, uint32_t>;
//using trades_queue = trades_ring<NUM_TRADES, timestamp_t, real_t, uint32_t>;


/*
template <class _Ty, class _Container = std::deque<_Ty>>
class queue {
public:
    using value_type = typename _Container::value_type;
    using reference = typename _Container::reference;
    using const_reference = typename _Container::const_reference;
    using size_type = typename _Container::size_type;
    using container_type = _Container;

    static_assert(is_same_v<_Ty, value_type>, "container adaptors require consistent types");

    queue() = default;

    explicit queue(const _Container& _Cont) : c(_Cont) {}

    explicit queue(_Container&& _Cont) noexcept(is_nothrow_move_constructible_v<_Container>) // strengthened
        : c(_STD move(_Cont)) {}

    template <class _Alloc, enable_if_t<uses_allocator_v<_Container, _Alloc>, int> = 0>
    explicit queue(const _Alloc& _Al) noexcept(is_nothrow_constructible_v<_Container, const _Alloc&>) // strengthened
        : c(_Al) {}

    template <class _Alloc, enable_if_t<uses_allocator_v<_Container, _Alloc>, int> = 0>
    queue(const _Container& _Cont, const _Alloc& _Al) : c(_Cont, _Al) {}

    template <class _Alloc, enable_if_t<uses_allocator_v<_Container, _Alloc>, int> = 0>
    queue(_Container&& _Cont, const _Alloc& _Al) noexcept(
        is_nothrow_constructible_v<_Container, _Container, const _Alloc&>) // strengthened
        : c(_STD move(_Cont), _Al) {}

    template <class _Alloc, enable_if_t<uses_allocator_v<_Container, _Alloc>, int> = 0>
    queue(const queue& _Right, const _Alloc& _Al) : c(_Right.c, _Al) {}

    template <class _Alloc, enable_if_t<uses_allocator_v<_Container, _Alloc>, int> = 0>
    queue(queue&& _Right, const _Alloc& _Al) noexcept(
        is_nothrow_constructible_v<_Container, _Container, const _Alloc&>) // strengthened
        : c(_STD move(_Right.c), _Al) {}

    _NODISCARD bool empty() const noexcept(noexcept(c.empty()))  {  // strengthened 
        return c.empty();
    }

    _NODISCARD size_type size() const noexcept(noexcept(c.size())) {  // strengthened 
        return c.size();
    }

    _NODISCARD reference front() noexcept(noexcept(c.front())) {  // strengthened 
        return c.front();
    }

    _NODISCARD const_reference front() const noexcept(noexcept(c.front())) {  // strengthened 
        return c.front();
    }

    _NODISCARD reference back() noexcept(noexcept(c.back())) {  // strengthened 
        return c.back();
    }

    _NODISCARD const_reference back() const noexcept(noexcept(c.back())) {  // strengthened 
        return c.back();
    }

    void push(const value_type& _Val) {
        c.push_back(_Val);
    }

    void push(value_type&& _Val) {
        c.push_back(_STD move(_Val));
    }

    template <class... _Valty>
    decltype(auto) emplace(_Valty&&... _Val) {
#if _HAS_CXX17
        return c.emplace_back(_STD forward<_Valty>(_Val)...);
#else // ^^^ C++17 or newer / C++14 vvv
        c.emplace_back(_STD forward<_Valty>(_Val)...);
#endif // _HAS_CXX17
    }

    void pop() noexcept(noexcept(c.pop_front())) {  // strengthened 
        c.pop_front();
    }

    void swap(queue& _Right) noexcept(_Is_nothrow_swappable<_Container>::value) {
        _Swap_adl(c, _Right.c);
    }

    _NODISCARD const _Container& _Get_container() const noexcept {
        return c;
    }

protected:
    _Container c{};
};
*/


#endif // TMP_RING_BUFFER
