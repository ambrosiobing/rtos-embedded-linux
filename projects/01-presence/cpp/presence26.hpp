// projects/01-presence/cpp/presence26.hpp: what C++26 adds, and what it does not yet.
//
// THE HONEST PART FIRST. C++26 is a standard whose compilers are still catching
// up, and a header that used reflection or contracts would not compile on the CI
// runner today. So this file probes for each feature with its feature-test macro
// and the test prints what it found. The finding this file exists to make is as
// much "this feature was not available on gcc N" as "this feature helped", and
// both are recorded in docs/LANGUAGE_IDIOMS.md with the compiler version that
// produced them.
//
// FOUR THINGS C++26 OFFERS THIS TABLE, in order of how much they matter here:
//
//   1. std::inplace_vector<Ev, 16>. The chapter's queue is sixteen deep and the
//      kernel adapter has no allocator. Before this, a fixed-capacity queue meant
//      a std::array plus a hand-written count and the bugs that come with it.
//      inplace_vector is exactly that container with the bugs already written out,
//      and it may be the most useful thing a recent standard offers an embedded
//      queue. **That is a prediction and not a result.** Measured on Sunday
//      5 October 2026, it was unavailable on gcc 13.3 and on clang 18.1.3 at every
//      flag, so in all twelve CI jobs the fallback below is what compiled and what
//      the queue test exercised. The branch above it has never run.
//
//   2. static_assert with a user-generated message, so the totality check can
//      name the missing pair rather than say "a pair is missing".
//
//   3. = delete("reason"), so the copy of a Context, which must not happen
//      because the kernel adapter owns exactly one, refuses with a sentence.
//
//   4. Pack indexing, which this table has no use for, and is listed so that
//      the absence of a use is on record rather than looking like an omission.
#ifndef PRESENCE26_HPP
#define PRESENCE26_HPP

#include "presence23.hpp"

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L
#include <inplace_vector>
#define PRESENCE_HAS_INPLACE_VECTOR 1
#else
#define PRESENCE_HAS_INPLACE_VECTOR 0
#endif

#if defined(__cpp_static_assert) && __cpp_static_assert >= 202306L
#define PRESENCE_HAS_STATIC_ASSERT_MSG 1
#else
#define PRESENCE_HAS_STATIC_ASSERT_MSG 0
#endif

#if defined(__cpp_deleted_function) && __cpp_deleted_function >= 202403L
#define PRESENCE_HAS_DELETE_REASON 1
#else
#define PRESENCE_HAS_DELETE_REASON 0
#endif

#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L
#define PRESENCE_HAS_PACK_INDEXING 1
#else
#define PRESENCE_HAS_PACK_INDEXING 0
#endif

namespace presence::v26 {

constexpr std::size_t kQueueDepth = 16;   // from chapter 01's data-flow figure

// ------------------------------------------------------- the event queue
#if PRESENCE_HAS_INPLACE_VECTOR
// The real thing: fixed capacity, no allocation, the standard's own bounds
// handling. push_back past capacity throws in the standard, and this project is
// exception free, so try_push_back is the only insertion used: it returns a
// pointer, null when full, which is the behaviour a kernel queue has.
using Queue = std::inplace_vector<Ev, kQueueDepth>;

inline bool enqueue(Queue& q, const Ev& ev) { return q.try_push_back(ev) != nullptr; }
#else
// The fallback is the thing inplace_vector replaces: an array and a count, with
// the capacity check written by hand. Same interface, so the test runs either
// way and reports which it ran.
struct Queue {
    std::array<Ev, kQueueDepth> items{};
    std::size_t count = 0;
    [[nodiscard]] std::size_t size() const { return count; }
    [[nodiscard]] bool empty() const { return count == 0; }
    [[nodiscard]] const Ev& operator[](std::size_t i) const { return items[i]; }
    void clear() { count = 0; }
};

inline bool enqueue(Queue& q, const Ev& ev) {
    if (q.count >= kQueueDepth) return false;
    q.items[q.count++] = ev;
    return true;
}
#endif

// ------------------------------------------ static_assert with a message
#if PRESENCE_HAS_STATIC_ASSERT_MSG && PRESENCE_HAS_CONSTEVAL
// A message built at compile time that names the row count, which the plain
// static_assert in presence23.hpp cannot. Small, but it is the difference between
// "a pair is missing" and knowing which one without opening the table.
struct RowCountMessage {
    char text[48]{};
    std::size_t len = 0;
    consteval RowCountMessage() {
        const char* head = "table has ";
        for (const char* p = head; *p; ++p) text[len++] = *p;
        std::size_t n = kRowCount;
        char digits[8]{};
        std::size_t d = 0;
        do { digits[d++] = static_cast<char>('0' + n % 10); n /= 10; } while (n);
        while (d) text[len++] = digits[--d];
        const char* tail = " rows, expected 29";
        for (const char* p = tail; *p; ++p) text[len++] = *p;
    }
    constexpr std::size_t size() const { return len; }
    constexpr const char* data() const { return text; }
};
static_assert(kRowCount == 29, RowCountMessage{});
#endif

// --------------------------------------------- a deleted copy with a reason
// One Context per node. The kernel adapter owns it, and a copy would be two
// state machines claiming to be the same room. C++26 lets the refusal say so.
struct OwnedContext {
    Context ctx{};
    OwnedContext() = default;
#if PRESENCE_HAS_DELETE_REASON
    OwnedContext(const OwnedContext&) = delete("one node has one context; copying it would be two state machines for one room");
    OwnedContext& operator=(const OwnedContext&) = delete("one node has one context");
#else
    OwnedContext(const OwnedContext&) = delete;
    OwnedContext& operator=(const OwnedContext&) = delete;
#endif
};

// ----------------------------------------------------- the feature report
struct Features {
    bool expected;
    bool to_underlying;
    bool consteval_checks;
    bool inplace_vector;
    bool static_assert_message;
    bool delete_with_reason;
    bool pack_indexing;
    long cplusplus;
};

[[nodiscard]] constexpr Features features() {
    return Features{
        PRESENCE_HAS_EXPECTED == 1,
        PRESENCE_HAS_TO_UNDERLYING == 1,
        PRESENCE_HAS_CONSTEVAL == 1,
        PRESENCE_HAS_INPLACE_VECTOR == 1,
        PRESENCE_HAS_STATIC_ASSERT_MSG == 1,
        PRESENCE_HAS_DELETE_REASON == 1,
        PRESENCE_HAS_PACK_INDEXING == 1,
        __cplusplus,
    };
}

} // namespace presence::v26

#endif // PRESENCE26_HPP
