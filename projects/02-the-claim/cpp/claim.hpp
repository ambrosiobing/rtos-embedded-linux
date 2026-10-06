// projects/02-the-claim/cpp/claim.hpp: the same cascade in C++17, the baseline.
//
// THE SPECIFICATION IS docs/DESIGN.md. This header and ../c/claim.c implement the same
// eight arms in the same order with the same guard names, and a script is meant to be
// able to prove that rather than a reader having to trust it.
//
// WHAT IS DIFFERENT FROM THE C, AND WHY IT IS NOT A DIFFERENT POLICY. The C writes the
// cascade as a chain of ifs calling named predicates. This writes it as a constexpr
// array of {guard, code}, iterated. Both yield the same ordered list of guard names and
// codes, which is what the comparison is of. The array form is here because it is what
// lets a later version PROVE properties of the order at compile time instead of
// asserting them in a test, and because an array is the honest shape for something whose
// order is its specification.
//
// Arm::guarded duplicates a fact the guard pointer already carries, exactly as P01's
// Row::guarded does, and for the same reason: the duplicate is checked against the
// pointer, so a hand edit that changes one and not the other is caught.
//
// NO ALLOCATION AND NO EXCEPTIONS. Nothing here throws and nothing here allocates. The
// spool is a fixed array sized from a byte bound at compile time.
//
// This file is the C++17 baseline and must compile unchanged under C++17, C++23 and
// C++2c. claim23.hpp and claim26.hpp layer on top of it and replace nothing.
#ifndef CLAIM_HPP
#define CLAIM_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace claim {

// ------------------------------------------------------------------- the claim axis

// Chapter 02's seven codes are 0 to 6. Free is the fall-through, the absence of a claim,
// and is deliberately not one of the seven: an ordinary release is told from a no-show by
// the code and not by a separate field that could be dropped.
enum class Code : std::uint8_t {
    Invisible = 0,
    Rejected  = 1,
    Booked    = 2,
    Grace     = 3,
    NoShow    = 4,
    Walkin    = 5,
    Brb       = 6,
    Free      = 7,
};

constexpr std::size_t kCodeCount      = 8;   // including the fall-through
constexpr std::size_t kPublishedCodes = 7;   // chapter 02's seven
constexpr std::size_t kArmCount       = 8;   // arms are numbered 1 to 8

// ----------------------------------------------------------------- the service axis

enum class Service : std::uint8_t {
    Ok             = 0,
    Degraded       = 1,
    NeedsService   = 2,
    OutOfService   = 3,
    InMaintenance  = 4,
    NeedsPart      = 5,
    Replaced       = 6,
};

constexpr std::size_t kServiceStates = 7;

enum class Reason : std::uint8_t {
    None            = 0,
    Sensor          = 1,
    Modem           = 2,
    Panel           = 3,
    Power           = 4,
    Fan             = 5,
    SetupIncomplete = 6,
    Commanded       = 7,
};

constexpr std::size_t kReasonCount = 8;   // including None
constexpr std::size_t kReasons     = 7;   // chapter 02's seven

// Four of the seven block a claim and three do not. Degraded and NeedsService are
// maintenance facts: something should be looked at and the room still works. A room taken
// out of use because a fan needs cleaning is a room lost for a week to a work order.
[[nodiscard]] constexpr bool service_blocks_state(Service s) noexcept {
    switch (s) {
    case Service::OutOfService:
    case Service::InMaintenance:
    case Service::NeedsPart:
    case Service::Replaced:
        return true;
    case Service::Ok:
    case Service::Degraded:
    case Service::NeedsService:
        return false;
    }
    // An unknown service state blocks: the safe answer to "is this room fit to be booked"
    // when the answer is not known is no.
    return true;
}

// ---------------------------------------------------------------------- the inputs

struct Inputs {
    bool    activated          = false;
    bool    window_open        = false;
    bool    present            = false;
    bool    past_grace         = false;
    bool    long_press_pending = false;
    Service service            = Service::Ok;
    Reason  service_reason     = Reason::None;
};

struct Decision {
    Code         code;
    std::uint8_t arm;   // 1 to 8
};

// ------------------------------------------------------- the arms, each one named

// The names are about the SITUATION and not the outcome: booked_and_waiting says what is
// true of the room, not that the answer is Grace. An arm whose guard is named after its
// own code cannot be checked against anything.
namespace detail {

[[nodiscard]] constexpr bool not_activated(const Inputs& in) noexcept {
    return !in.activated;
}
[[nodiscard]] constexpr bool service_blocks(const Inputs& in) noexcept {
    return service_blocks_state(in.service);
}
[[nodiscard]] constexpr bool booked_and_present(const Inputs& in) noexcept {
    return in.window_open && in.present;
}
[[nodiscard]] constexpr bool booked_and_waiting(const Inputs& in) noexcept {
    return in.window_open && !in.present && !in.past_grace;
}
[[nodiscard]] constexpr bool booked_and_nobody_came(const Inputs& in) noexcept {
    return in.window_open && !in.present && in.past_grace;
}
[[nodiscard]] constexpr bool unbooked_and_present(const Inputs& in) noexcept {
    return !in.window_open && in.present;
}
[[nodiscard]] constexpr bool press_pending(const Inputs& in) noexcept {
    return in.long_press_pending;
}
// The fall-through. Named so the array has no null entry to special-case, and marked
// unguarded in the arm itself.
[[nodiscard]] constexpr bool always(const Inputs&) noexcept {
    return true;
}

}  // namespace detail

using Guard = bool (*)(const Inputs&) noexcept;

struct Arm {
    Guard        guard;
    Code         code;
    bool         guarded;
};

#define CLAIM_ARM(g, c, gd) ::claim::Arm{ ::claim::detail::g, ::claim::Code::c, gd }

// THE ORDER IS THE SPECIFICATION. docs/DESIGN.md carries a table of the defect each
// permutation would cause, and every row of it is a failure somebody would ship.
inline constexpr std::array<Arm, kArmCount> kCascade = {{
    CLAIM_ARM(not_activated,          Invisible, true),
    CLAIM_ARM(service_blocks,         Rejected,  true),
    CLAIM_ARM(booked_and_present,     Booked,    true),
    CLAIM_ARM(booked_and_waiting,     Grace,     true),
    CLAIM_ARM(booked_and_nobody_came, NoShow,    true),
    CLAIM_ARM(unbooked_and_present,   Walkin,    true),
    CLAIM_ARM(press_pending,          Brb,       true),
    CLAIM_ARM(always,                 Free,      false),
}};

#undef CLAIM_ARM

// Available already in C++17, and worth having here rather than only in the test: an arm
// with no guard shadows everything below it, so there must be exactly one and it must be
// last. claim23.hpp proves more than this.
[[nodiscard]] constexpr bool only_the_last_arm_is_unguarded() noexcept {
    for (std::size_t i = 0; i + 1 < kCascade.size(); ++i) {
        if (!kCascade[i].guarded) {
            return false;
        }
    }
    return !kCascade[kCascade.size() - 1].guarded;
}

static_assert(only_the_last_arm_is_unguarded(),
              "an unguarded arm anywhere but last shadows every arm below it");

// -------------------------------------------------------------------- the cascade

[[nodiscard]] constexpr Decision decide(const Inputs& in) noexcept {
    for (std::size_t i = 0; i < kCascade.size(); ++i) {
        if (kCascade[i].guard(in)) {
            return Decision{ kCascade[i].code, static_cast<std::uint8_t>(i + 1) };
        }
    }
    // Unreachable while the last arm is unguarded, which the static_assert above holds.
    return Decision{ Code::Free, static_cast<std::uint8_t>(kArmCount) };
}

// An event leaves the unit when the code changes, and Grace never leaves at all. The
// calendar already knows about the booking because it sent it; what the unit knows and the
// calendar cannot is whether anybody came, and during grace it has observed nothing on
// that question.
[[nodiscard]] constexpr bool emits(Code prev, Code now) noexcept {
    if (now == Code::Grace) {
        return false;
    }
    return now != prev;
}

// ------------------------------------------------------------------- the settings

struct Settings {
    std::uint32_t grace_s       = 300;
    std::uint32_t brb_hold_s    = 900;
    std::uint32_t walkin_len_s  = 1800;
    std::uint32_t spool_bytes   = 4096;
    std::uint32_t stale_after_s = 600;
    // Off, and that is the point: the failure mode of a half-finished installation is a
    // room nobody can book rather than a room that swallows bookings.
    bool          activated     = false;
};

// ---------------------------------------------------------------------- the spool

// Fixed width and ordered largest first, so the size is 8 bytes with no padding.
struct Event {
    std::uint32_t at_s;
    std::uint8_t  code;
    std::uint8_t  arm;
    std::uint8_t  service;
    std::uint8_t  service_reason;
};

static_assert(sizeof(Event) == 8,
              "the spool event must be 8 bytes, or the byte bound buys a different "
              "number of events than the page says");

constexpr std::uint32_t kSpoolBytesMax = 4096;
constexpr std::size_t   kSpoolSlots    = kSpoolBytesMax / sizeof(Event);

static_assert(kSpoolSlots == 512, "4096 bytes of 8 gives 512 slots");

// The bound is in BYTES, because bytes are what run out.
class Spool {
public:
    Spool() noexcept { init(kSpoolBytesMax); }

    void init(std::uint32_t bytes) noexcept {
        head_ = 0;
        count_ = 0;
        discarded_ = 0;
        offered_ = 0;

        std::size_t want = static_cast<std::size_t>(bytes / sizeof(Event));
        if (want > kSpoolSlots) {
            want = kSpoolSlots;
        }
        if (want == 0) {
            want = 1;   // a spool of no events would discard everything silently
        }
        cap_ = want;
    }

    // Always accepts, discarding the oldest when full, because the newest event is the
    // one describing the room now. Returns true when a discard happened.
    bool push(const Event& ev) noexcept {
        bool discarded = false;

        ++offered_;
        if (count_ == cap_) {
            head_ = (head_ + 1) % cap_;
            --count_;
            ++discarded_;
            discarded = true;
        }
        slot_[(head_ + count_) % cap_] = ev;
        ++count_;
        return discarded;
    }

    [[nodiscard]] std::size_t   capacity()  const noexcept { return cap_; }
    [[nodiscard]] std::size_t   count()     const noexcept { return count_; }
    [[nodiscard]] std::uint32_t discarded() const noexcept { return discarded_; }
    [[nodiscard]] std::uint32_t offered()   const noexcept { return offered_; }

    // Oldest first, so at(0) is the oldest retained.
    [[nodiscard]] const Event& at(std::size_t i) const noexcept {
        return slot_[(head_ + i) % cap_];
    }

    void drain() noexcept {
        head_ = 0;
        count_ = 0;
        discarded_ = 0;
        offered_ = 0;
    }

private:
    std::array<Event, kSpoolSlots> slot_{};
    std::size_t   cap_       = kSpoolSlots;
    std::size_t   head_      = 0;
    std::size_t   count_     = 0;
    std::uint32_t discarded_ = 0;
    std::uint32_t offered_   = 0;
};

// -------------------------------------------------------------------- the context

class Machine {
public:
    Machine() noexcept { spool_.init(settings.spool_bytes); }

    Settings settings{};

    Decision step(const Inputs& in, std::uint32_t at_s) noexcept {
        Inputs eff = in;
        // Activation is a setting, not a per-reading input, and the settings win because
        // that is where commissioning is recorded.
        eff.activated = settings.activated;

        const Decision d = decide(eff);

        ++arm_taken_[static_cast<std::size_t>(d.arm) - 1];
        ++code_seen_[static_cast<std::size_t>(d.code)];
        ++service_seen_[static_cast<std::size_t>(eff.service)];
        ++reason_seen_[static_cast<std::size_t>(eff.service_reason)];

        if (emits(last_.code, d.code)) {
            const Event ev{ at_s,
                            static_cast<std::uint8_t>(d.code),
                            d.arm,
                            static_cast<std::uint8_t>(eff.service),
                            static_cast<std::uint8_t>(eff.service_reason) };
            spool_.push(ev);
            ++emitted_;
            published_ = d.code;
        }

        last_in_ = eff;
        last_ = d;
        return d;
    }

    // The six invariants of docs/DESIGN.md. The test calls this after every single case
    // rather than at the end, because a broken invariant can be transient and still wrong.
    [[nodiscard]] bool check_invariants() const noexcept {
        if (!settings.activated && last_.arm != 0 && last_.code != Code::Invisible) {
            return false;
        }
        if (last_.arm != 0 && settings.activated &&
            service_blocks_state(last_in_.service) && last_.code != Code::Rejected) {
            return false;
        }
        for (std::size_t i = 0; i < spool_.count(); ++i) {
            if (spool_.at(i).code == static_cast<std::uint8_t>(Code::Grace)) {
                return false;
            }
        }
        if (spool_.count() > spool_.capacity() || spool_.capacity() > kSpoolSlots) {
            return false;
        }
        if (static_cast<std::uint32_t>(spool_.count()) + spool_.discarded() !=
            spool_.offered()) {
            return false;
        }
        if (emitted_ != spool_.offered()) {
            return false;
        }
        return true;
    }

    [[nodiscard]] const Decision& last()      const noexcept { return last_; }
    [[nodiscard]] Code            published() const noexcept { return published_; }
    [[nodiscard]] std::uint32_t   emitted()   const noexcept { return emitted_; }
    [[nodiscard]] const Spool&    spool()     const noexcept { return spool_; }
    [[nodiscard]] Spool&          spool()           noexcept { return spool_; }

    [[nodiscard]] std::uint32_t arm_taken(std::size_t i)    const noexcept { return arm_taken_[i]; }
    [[nodiscard]] std::uint32_t code_seen(std::size_t i)    const noexcept { return code_seen_[i]; }
    [[nodiscard]] std::uint32_t service_seen(std::size_t i) const noexcept { return service_seen_[i]; }
    [[nodiscard]] std::uint32_t reason_seen(std::size_t i)  const noexcept { return reason_seen_[i]; }

private:
    Inputs       last_in_{};
    // There is no decision yet. Invisible is the honest initial value because the unit is
    // not commissioned, and arm 0 says no arm has fired.
    Decision     last_{ Code::Invisible, 0 };
    Code         published_ = Code::Invisible;
    Spool        spool_{};
    std::uint32_t emitted_ = 0;

    std::array<std::uint32_t, kArmCount>      arm_taken_{};
    std::array<std::uint32_t, kCodeCount>     code_seen_{};
    std::array<std::uint32_t, kServiceStates> service_seen_{};
    std::array<std::uint32_t, kReasonCount>   reason_seen_{};
};

// ----------------------------------------------------------------------- the names

[[nodiscard]] constexpr const char* code_name(Code c) noexcept {
    switch (c) {
    case Code::Invisible: return "invisible";
    case Code::Rejected:  return "rejected";
    case Code::Booked:    return "booked";
    case Code::Grace:     return "grace";
    case Code::NoShow:    return "no-show";
    case Code::Walkin:    return "walk-in";
    case Code::Brb:       return "be-right-back";
    case Code::Free:      return "free";
    }
    return "?";
}

[[nodiscard]] constexpr const char* service_name(Service s) noexcept {
    switch (s) {
    case Service::Ok:            return "ok";
    case Service::Degraded:      return "degraded";
    case Service::NeedsService:  return "needs-service";
    case Service::OutOfService:  return "out-of-service";
    case Service::InMaintenance: return "in-maintenance";
    case Service::NeedsPart:     return "needs-part";
    case Service::Replaced:      return "replaced";
    }
    return "?";
}

[[nodiscard]] constexpr const char* reason_name(Reason r) noexcept {
    switch (r) {
    case Reason::None:            return "none";
    case Reason::Sensor:          return "sensor";
    case Reason::Modem:           return "modem";
    case Reason::Panel:           return "panel";
    case Reason::Power:           return "power";
    case Reason::Fan:             return "fan";
    case Reason::SetupIncomplete: return "setup-incomplete";
    case Reason::Commanded:       return "commanded";
    }
    return "?";
}

}  // namespace claim

#endif  // CLAIM_HPP
