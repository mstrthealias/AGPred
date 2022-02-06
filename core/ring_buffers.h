#ifndef TMP_RING_BUFFER
#define TMP_RING_BUFFER

#include <algorithm>
#include <array>
#include <execution>

#include "../src/common.h" 
#include "core.h"


constexpr bool DEBUG_RING = false;


/**
 * quotes_ring implements a circular buffer with a queue-like interface for QuoteData
 */
template <size_t _Size = 15000, typename _Ttime = timestamp_t, typename _Treal = double, typename _Tsize = uint32_t>
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

        timestamp_[end_] = _Val.quote.timestamp;
        bid_[end_] = _Val.quote.bid;
        ask_[end_] = _Val.quote.ask;
        bid_size_[end_] = _Val.quote.bid_size;
        ask_size_[end_] = _Val.quote.ask_size;
        cond1_[end_] = _Val.cond[0];
        cond2_[end_] = _Val.cond[1];
        cond3_[end_] = _Val.cond[2];

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
	            timestamp_[start_],
                bid_[start_],
                ask_[start_],
	            bid_size_[start_],
	            ask_size_[start_]
            },
            { cond1_[start_], cond2_[start_], cond3_[start_]}
        };
    }
    
    _NODISCARD agpred::QuoteData back() {  // strengthened  TODO noexcept?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        const auto pos = end_ - 1;
        return {
            {
                timestamp_[pos],
                bid_[pos],
                ask_[pos],
                bid_size_[pos],
                ask_size_[pos]
            },
            { cond1_[pos], cond2_[pos], cond3_[pos]}
        };
    }

    /*void swap(queue& _Right) noexcept(_Is_nothrow_swappable<_Container>::value) {
        _Swap_adl(c, _Right.c);
    }*/

private:
    size_t start_ = 0;  // inclusive start
    size_t end_ = 0;  // exclusive end
    
    std::array<_Ttime, 2 * _Size> timestamp_ = {};
    std::array<_Treal, 2 * _Size> bid_ = {};
    std::array<_Treal, 2 * _Size> ask_ = {};
    std::array<_Tsize, 2 * _Size> bid_size_ = {};
    std::array<_Tsize, 2 * _Size> ask_size_ = {};
    std::array<QuoteCondition, 2 * _Size> cond1_ = {};
    std::array<QuoteCondition, 2 * _Size> cond2_ = {};
    std::array<QuoteCondition, 2 * _Size> cond3_ = {};
};


/**
 * trades_ring implements a circular buffer with a queue-like interface for TradeData
 */
template <size_t _Size = 15000, typename _Ttime = timestamp_t, typename _Treal = double, typename _Tsize = uint32_t>
class trades_ring
{
public:
    using value_type = agpred::TradeData;
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

        timestamp_[end_] = _Val.trade.timestamp;
        price_[end_] = _Val.trade.price;
        size_[end_] = _Val.trade.size;
        cond1_[end_] = _Val.cond[0];
        cond2_[end_] = _Val.cond[1];
        cond3_[end_] = _Val.cond[2];

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
                timestamp_[start_],
                price_[start_],
                size_[start_]
            },
            { cond1_[start_], cond2_[start_], cond3_[start_]}
        };
    }

    _NODISCARD agpred::TradeData back() {  // strengthened  TODO noexcept?
        if (start_ >= end_)
            throw std::logic_error("ring empty");  // TODO no throw? vector has noexcept for pop_back

        const auto pos = end_ - 1;
        return {
            {
                timestamp_[pos],
                price_[pos],
                size_[pos]
            },
            { cond1_[pos], cond2_[pos], cond3_[pos]}
        };
    }

    /*void swap(queue& _Right) noexcept(_Is_nothrow_swappable<_Container>::value) {
        _Swap_adl(c, _Right.c);
    }*/

private:
    size_t start_ = 0;  // inclusive start
    size_t end_ = 0;  // exclusive end

    std::array<_Ttime, 2 * _Size> timestamp_ = {};
    std::array<_Treal, 2 * _Size> price_ = {};
    std::array<_Tsize, 2 * _Size> size_ = {};
    std::array<TradeCondition, 2 * _Size> cond1_ = {};
    std::array<TradeCondition, 2 * _Size> cond2_ = {};
    std::array<TradeCondition, 2 * _Size> cond3_ = {};
};


// TODO consolidate?
using quotes_queue = quotes_ring<NUM_QUOTES, timestamp_t, double, uint32_t>;
using trades_queue = trades_ring<NUM_TRADES, timestamp_t, double, uint32_t>;


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
