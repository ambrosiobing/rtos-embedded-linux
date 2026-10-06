// projects/02-the-claim/cpp/claim23.hpp: what C++23 adds to the same cascade.
//
// Nothing here replaces claim.hpp. It layers on top, and every addition sits behind the
// feature-test macro that says the compiler actually provides it. A compiler lacking the
// feature compiles this header to the baseline and the test reports which branch was
// taken, so the claim "C++23 buys this" is checked by the compiler making it.
//
// THE ONE THAT MATTERS, AND IT IS NOT std::expected. The C checks the six invariants of
// docs/DESIGN.md after each of twenty-two cases. Twenty-two points is a sample. This
// header proves the same invariants over the WHOLE INPUT SPACE at compile time: five
// booleans and seven service states is 224 combinations, which is nothing for a
// constant evaluator and everything for the strength of the claim. A sampled invariant
// can hold at every point tested and fail at the point nobody wrote a case for; an
// exhausted one cannot.
//
// AND THE HONEST PART. That proof does not need C++23. It would compile as a constexpr
// function behind a static_assert under C++17, because calling through a function pointer
// is allowed in a constant expression when the pointer is one. What `consteval` adds is
// that the function CANNOT be called at run time, so the proof cannot silently become a
// run-time cost. That is a real difference and a small one, and saying so is the point of
// the comparison: a version is not interesting because of what it permits but because of
// what it changes here.
//
// WHAT C++23 GENUINELY CHANGES. std::expected for settings that may be refused. The C has
// no settings validation at all: claim_spool_init silently clamps a bound of zero up to
// one event, which is a quiet correction of a configuration somebody got wrong. A device
// that corrects its configuration in silence is a device that cannot be commissioned
// reliably, so this version returns the refusal instead.
#ifndef CLAIM23_HPP
#define CLAIM23_HPP

#include "claim.hpp"

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
#define CLAIM_HAS_EXPECTED 1
#else
#define CLAIM_HAS_EXPECTED 0
#endif

#if defined(__cpp_lib_to_underlying) && __cpp_lib_to_underlying >= 202102L
#include <utility>
#define CLAIM_HAS_TO_UNDERLYING 1
#else
#define CLAIM_HAS_TO_UNDERLYING 0
#endif

#if defined(__cpp_consteval) && __cpp_consteval >= 201811L
#define CLAIM_HAS_CONSTEVAL 1
#define CLAIM_CONSTEVAL consteval
#else
#define CLAIM_HAS_CONSTEVAL 0
#define CLAIM_CONSTEVAL constexpr
#endif

namespace claim {

// ------------------------------------------- the invariants, over every possible input

// Five booleans and seven service states. The service reason is not an input to the
// cascade at all, which is itself worth knowing: it travels with the event for the record
// and no arm reads it.
constexpr unsigned kInputBitPatterns = 32;
constexpr unsigned kInputSpace = kInputBitPatterns * static_cast<unsigned>(kServiceStates);

[[nodiscard]] constexpr Inputs input_at(unsigned bits, std::size_t svc) noexcept {
    Inputs in{};
    in.activated          = (bits & 1u) != 0u;
    in.window_open        = (bits & 2u) != 0u;
    in.present            = (bits & 4u) != 0u;
    in.past_grace         = (bits & 8u) != 0u;
    in.long_press_pending = (bits & 16u) != 0u;
    in.service            = static_cast<Service>(svc);
    return in;
}

// Every clause below is one of the six invariants of docs/DESIGN.md, or one of the
// ordering rules its permutation table names. Each is a property of the policy rather
// than of a case, which is why it can be stated over the whole space.
[[nodiscard]] CLAIM_CONSTEVAL bool invariants_hold_over_every_input() noexcept {
    for (unsigned bits = 0; bits < kInputBitPatterns; ++bits) {
        for (std::size_t svc = 0; svc < kServiceStates; ++svc) {
            const Inputs in = input_at(bits, svc);
            const Decision d = decide(in);

            // Total: some arm always fires, and it is one of the eight.
            if (d.arm < 1 || d.arm > kArmCount) {
                return false;
            }

            // 1. An unactivated unit yields Invisible and nothing else.
            if (!in.activated && d.code != Code::Invisible) {
                return false;
            }

            // 2. A blocking service state always yields Rejected.
            if (in.activated && service_blocks_state(in.service) &&
                d.code != Code::Rejected) {
                return false;
            }

            // A live booking beats a walk-in: Walkin is impossible while a window is open.
            if (d.code == Code::Walkin && in.window_open) {
                return false;
            }

            // Grace means nobody has arrived, so it is impossible while present.
            if (d.code == Code::Grace && in.present) {
                return false;
            }

            // A no-show needs an open window and an empty room, and nothing else produces
            // it. This is the clause that would catch the grace and no-show arms swapped.
            if (d.code == Code::NoShow && (in.present || !in.window_open ||
                                           !in.past_grace)) {
                return false;
            }

            // Booked needs the holder present.
            if (d.code == Code::Booked && !(in.window_open && in.present)) {
                return false;
            }

            // Be-right-back is reachable only when nobody is present, which is what it is
            // for: it holds the room for somebody who pressed on the way out.
            if (d.code == Code::Brb && in.present) {
                return false;
            }
        }
    }
    return true;
}

static_assert(invariants_hold_over_every_input(),
              "an invariant from docs/DESIGN.md fails somewhere in the 224 input "
              "combinations, which no sample of cases would have to find");

// Every arm must be reachable by some input, or it is dead code pretending to be policy.
[[nodiscard]] CLAIM_CONSTEVAL bool every_arm_is_reachable() noexcept {
    bool seen[kArmCount] = {};

    for (unsigned bits = 0; bits < kInputBitPatterns; ++bits) {
        for (std::size_t svc = 0; svc < kServiceStates; ++svc) {
            seen[decide(input_at(bits, svc)).arm - 1] = true;
        }
    }
    for (std::size_t i = 0; i < kArmCount; ++i) {
        if (!seen[i]) {
            return false;
        }
    }
    return true;
}

static_assert(every_arm_is_reachable(),
              "an arm no input can reach is dead code in the shape of a policy");

// ------------------------------------------------ settings that may be refused

enum class SettingsError : std::uint8_t {
    GraceIsZero,
    SpoolHoldsNoEvents,
    WalkinShorterThanGrace,
};

[[nodiscard]] constexpr const char* settings_error_name(SettingsError e) noexcept {
    switch (e) {
    case SettingsError::GraceIsZero:            return "a grace period of zero releases every booking at once";
    case SettingsError::SpoolHoldsNoEvents:     return "a spool bound below one event discards everything in silence";
    case SettingsError::WalkinShorterThanGrace: return "a walk-in shorter than the grace period is a room that frees itself while in use";
    }
    return "?";
}

// What the C does instead: claim_spool_init clamps a zero bound up to one event and says
// nothing. Both behaviours are defensible and only one of them can be commissioned, which
// is the argument for this being the later version's job.
[[nodiscard]] constexpr bool settings_are_usable(const Settings& s) noexcept {
    if (s.grace_s == 0) {
        return false;
    }
    if (s.spool_bytes < sizeof(Event)) {
        return false;
    }
    if (s.walkin_len_s < s.grace_s) {
        return false;
    }
    return true;
}

[[nodiscard]] constexpr SettingsError first_settings_error(const Settings& s) noexcept {
    if (s.grace_s == 0) {
        return SettingsError::GraceIsZero;
    }
    if (s.spool_bytes < sizeof(Event)) {
        return SettingsError::SpoolHoldsNoEvents;
    }
    return SettingsError::WalkinShorterThanGrace;
}

#if CLAIM_HAS_EXPECTED
// The refusal is the return value rather than a bool plus an out-parameter, so a caller
// cannot read the settings after a failed check and get the defaults back.
[[nodiscard]] constexpr std::expected<Settings, SettingsError>
checked_settings(const Settings& s) noexcept {
    if (!settings_are_usable(s)) {
        return std::unexpected(first_settings_error(s));
    }
    return s;
}
#endif

// --------------------------------------------------------------- the small change

#if CLAIM_HAS_TO_UNDERLYING
[[nodiscard]] constexpr std::uint8_t code_value(Code c) noexcept {
    return std::to_underlying(c);
}
#else
[[nodiscard]] constexpr std::uint8_t code_value(Code c) noexcept {
    return static_cast<std::uint8_t>(c);
}
#endif

}  // namespace claim

#endif  // CLAIM23_HPP
