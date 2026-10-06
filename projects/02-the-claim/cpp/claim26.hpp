// projects/02-the-claim/cpp/claim26.hpp: what C++26 adds, and the one place it does not fit.
//
// THE FINDING IN THIS FILE IS NEGATIVE, AND IT IS THE MOST USEFUL ONE IN THE THREE
// HEADERS. std::inplace_vector is a bounded container with its capacity in its type and no
// allocation, which is exactly the shape of a spool with a hard byte bound, and it is the
// wrong container for this spool. The reason is the cost of the one operation the design
// depends on.
//
// The spool discards the OLDEST when it is full, because the newest event describes the
// room now. In a ring that is one index step: O(1). In a vector it is erase(begin()),
// which shifts every surviving element down one: O(n), and n is 512. The run that fills
// the spool in the test offers about 1400 events and discards about 880 of them, so the
// vector form would move roughly 450,000 events to achieve what the ring does with 880
// index increments. On a Cortex-M that is not a micro-optimisation, it is the difference
// between a spool and a stall.
//
// SO WHAT IS inplace_vector ACTUALLY FOR HERE. A bounded batch that is filled and then
// drained whole, which is what a transmit batch is: take up to N events, hand them to the
// radio, clear. There is no discard-oldest in that pattern, so the cost that rules it out
// of the spool never arises. That is P11's problem and the batch below is the shape it
// will want, written here because this is the chapter where the bound is argued.
//
// WHAT IS CHECKED AND WHAT IS ONLY COUNTED. The test drives the same thousand events
// through both forms and requires identical retained contents in identical order, so
// "correct" is checked. The cost is NOT measured: the number of element moves is computed
// from the discard count and the capacity, which is arithmetic rather than a timing. A
// measurement would need the board, and any figure taken on a host is a measurement of the
// host. The complexity claim stands on the shape of the operation, not on a stopwatch.
//
// And all of this compiles only where the library actually has std::inplace_vector. Where
// it does not, the comparison does not run and the test says so rather than passing in
// silence.
#ifndef CLAIM26_HPP
#define CLAIM26_HPP

#include "claim23.hpp"

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L
#include <inplace_vector>
#define CLAIM_HAS_INPLACE_VECTOR 1
#else
#define CLAIM_HAS_INPLACE_VECTOR 0
#endif

// C++26's user-generated static_assert message, which lets a failing proof print a
// computed string rather than a fixed one. C++17 already allowed static_assert with no
// message at all; this is the later and different thing.
#if defined(__cpp_static_assert) && __cpp_static_assert >= 202306L
#define CLAIM_HAS_STATIC_ASSERT_MSG 1
#else
#define CLAIM_HAS_STATIC_ASSERT_MSG 0
#endif

namespace claim {

// ------------------------------------------------- the bounded batch, where it does fit

#if CLAIM_HAS_INPLACE_VECTOR

// A transmit batch: filled, handed over, cleared. No discard-oldest, so the cost that
// rules inplace_vector out of the spool never arises. The capacity is in the type, which
// is the part worth having: a batch that cannot exceed its bound by construction needs no
// check at the call site.
template <std::size_t N>
class Batch {
public:
    [[nodiscard]] bool take(const Event& ev) noexcept {
        if (events_.size() == events_.capacity()) {
            return false;   // full, and the caller drains rather than the batch discarding
        }
        events_.push_back(ev);
        return true;
    }

    [[nodiscard]] std::size_t size()     const noexcept { return events_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return events_.capacity(); }
    [[nodiscard]] const Event& at(std::size_t i) const noexcept { return events_[i]; }

    void clear() noexcept { events_.clear(); }

private:
    std::inplace_vector<Event, N> events_{};
};

// The same bound the spool uses, so the two are comparable.
using TransmitBatch = Batch<kSpoolSlots>;

// ----------------------------------- the spool written the wrong way, on purpose

// This exists to be compared, not to be used. It is the discard-oldest spool expressed as
// a vector, which is the form a reader reaches for when a bounded container appears in the
// standard library. It is correct and it is O(n) per discard.
class VectorSpool {
public:
    void init(std::uint32_t bytes) noexcept {
        events_.clear();
        discarded_ = 0;
        offered_ = 0;

        std::size_t want = static_cast<std::size_t>(bytes / sizeof(Event));
        if (want > kSpoolSlots) {
            want = kSpoolSlots;
        }
        if (want == 0) {
            want = 1;
        }
        cap_ = want;
    }

    bool push(const Event& ev) noexcept {
        bool discarded = false;

        ++offered_;
        if (events_.size() == cap_) {
            // The O(n) step, and the whole point of this class. Every surviving event
            // moves down one so that the oldest can leave from the front.
            events_.erase(events_.begin());
            ++discarded_;
            discarded = true;
        }
        events_.push_back(ev);
        return discarded;
    }

    [[nodiscard]] std::size_t   capacity()  const noexcept { return cap_; }
    [[nodiscard]] std::size_t   count()     const noexcept { return events_.size(); }
    [[nodiscard]] std::uint32_t discarded() const noexcept { return discarded_; }
    [[nodiscard]] std::uint32_t offered()   const noexcept { return offered_; }
    [[nodiscard]] const Event&  at(std::size_t i) const noexcept { return events_[i]; }

private:
    std::inplace_vector<Event, kSpoolSlots> events_{};
    std::size_t   cap_       = kSpoolSlots;
    std::uint32_t discarded_ = 0;
    std::uint32_t offered_   = 0;
};

#endif  // CLAIM_HAS_INPLACE_VECTOR

// --------------------------------------------------------------- the feature report

struct Features {
    bool expected;
    bool to_underlying;
    bool consteval_checks;
    bool inplace_vector;
    bool static_assert_message;
    long cplusplus;
};

[[nodiscard]] constexpr Features features() noexcept {
    return Features{
        CLAIM_HAS_EXPECTED == 1,
        CLAIM_HAS_TO_UNDERLYING == 1,
        CLAIM_HAS_CONSTEVAL == 1,
        CLAIM_HAS_INPLACE_VECTOR == 1,
        CLAIM_HAS_STATIC_ASSERT_MSG == 1,
        static_cast<long>(__cplusplus),
    };
}

}  // namespace claim

#endif  // CLAIM26_HPP
