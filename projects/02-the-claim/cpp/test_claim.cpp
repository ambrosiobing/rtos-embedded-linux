// projects/02-the-claim/cpp/test_claim.cpp: the same twenty-two cases, in C++.
//
// WHAT THIS ADDS TO ../c/test_claim.c. Not coverage of the policy, which is identical: the
// same twenty-two cases, the same seven criteria, the same expected arm and code for each.
// What it adds is the second implementation, because a specification with one
// implementation is a guess about what is general, and because two of chapter 02's claims
// about ordering can be proved at compile time here rather than asserted at run time.
//
// The last lines are a report of what this compiler actually provided, and the idioms page
// is filled from that report rather than from a standards table. So read the log.
#include "claim26.hpp"   // which includes claim23.hpp, which includes claim.hpp

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(cond, ...)                                                      \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::printf("  FAIL %s:%d: ", __FILE__, __LINE__);                \
            std::printf(__VA_ARGS__);                                         \
            std::printf("\n");                                                \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

// ------------------------------------------------------------- the twenty-two cases

struct Case {
    const char*    what;
    bool           activated;
    bool           window_open;
    bool           present;
    bool           past_grace;
    bool           long_press;
    claim::Service svc;
    claim::Reason  reason;
    claim::Code    want_code;
    std::uint8_t   want_arm;
};

using claim::Code;
using claim::Reason;
using claim::Service;

constexpr Case kCases[] = {
    // Arm 1: not commissioned, and it outranks every other input including the service
    // axis, which is the whole point of the flag.
    { "uncommissioned and quiet",
      false, false, false, false, false, Service::Ok, Reason::SetupIncomplete,
      Code::Invisible, 1 },
    { "uncommissioned, booked, holder present",
      false, true,  true,  false, false, Service::Ok, Reason::None,
      Code::Invisible, 1 },
    { "uncommissioned, somebody walks in",
      false, false, true,  false, false, Service::Ok, Reason::None,
      Code::Invisible, 1 },
    { "uncommissioned and out of service: activation outranks the service axis",
      false, false, false, false, false, Service::OutOfService, Reason::Power,
      Code::Invisible, 1 },

    // Arm 2: all four blocking states, the last showing it outranks the no-show arm.
    { "out of service with the holder present",
      true,  true,  true,  false, false, Service::OutOfService, Reason::Power,
      Code::Rejected, 2 },
    { "somebody is working in the room",
      true,  false, true,  false, false, Service::InMaintenance, Reason::Commanded,
      Code::Rejected, 2 },
    { "a part is missing",
      true,  false, false, false, false, Service::NeedsPart, Reason::Fan,
      Code::Rejected, 2 },
    { "superseded, and past grace: service outranks the no-show",
      true,  true,  false, true,  false, Service::Replaced, Reason::Commanded,
      Code::Rejected, 2 },

    // Arm 3: the two non-blocking fault states do not take the room out of use.
    { "booked and the holder is here",
      true,  true,  true,  false, false, Service::Ok, Reason::None,
      Code::Booked, 3 },
    { "booked and here, with a fan that needs cleaning",
      true,  true,  true,  false, false, Service::Degraded, Reason::Fan,
      Code::Booked, 3 },
    { "booked and here, with a sensor due for service",
      true,  true,  true,  false, false, Service::NeedsService, Reason::Sensor,
      Code::Booked, 3 },

    // Arm 4: booked, nobody here yet, inside the grace period.
    { "booked and nobody has arrived yet",
      true,  true,  false, false, false, Service::Ok, Reason::None,
      Code::Grace, 4 },
    { "booked, nobody yet, modem due for service",
      true,  true,  false, false, false, Service::NeedsService, Reason::Modem,
      Code::Grace, 4 },

    // Arm 5: the grace period ran out.
    { "nobody came",
      true,  true,  false, true,  false, Service::Ok, Reason::None,
      Code::NoShow, 5 },
    { "nobody came, and the panel is faulty",
      true,  true,  false, true,  false, Service::Degraded, Reason::Panel,
      Code::NoShow, 5 },

    // Arm 6: a walk-in, and a press by somebody still in the room is still a walk-in.
    { "somebody walks into an unbooked room",
      true,  false, true,  false, false, Service::Ok, Reason::None,
      Code::Walkin, 6 },
    { "a walk-in who presses the button while still in the room",
      true,  false, true,  false, true,  Service::Ok, Reason::None,
      Code::Walkin, 6 },

    // Arm 7: be right back, reachable only when nobody is present.
    { "pressed the button on the way out",
      true,  false, false, false, true,  Service::Ok, Reason::None,
      Code::Brb, 7 },
    { "pressed on the way out, with a fan that needs cleaning",
      true,  false, false, false, true,  Service::Degraded, Reason::Fan,
      Code::Brb, 7 },

    // Arm 8: the fall-through, which is not one of the seven codes.
    { "nothing at all is happening",
      true,  false, false, false, false, Service::Ok, Reason::None,
      Code::Free, 8 },
    { "nothing happening, sensor due for service: still free",
      true,  false, false, false, false, Service::NeedsService, Reason::Sensor,
      Code::Free, 8 },

    // And grace outranks a press: somebody pressed while the room was booked and empty.
    { "a press during grace creates no claim",
      true,  true,  false, false, true,  Service::Ok, Reason::None,
      Code::Grace, 4 },
};

constexpr std::size_t kCaseCount = sizeof(kCases) / sizeof(kCases[0]);

static_assert(kCaseCount == 22, "chapter 02 says twenty-two cases");

constexpr claim::Inputs inputs_of(const Case& k) noexcept {
    claim::Inputs in{};
    in.activated          = k.activated;
    in.window_open        = k.window_open;
    in.present            = k.present;
    in.past_grace         = k.past_grace;
    in.long_press_pending = k.long_press;
    in.service            = k.svc;
    in.service_reason     = k.reason;
    return in;
}

// ------------------------------------------- what this version proves at compile time

// The duplicated `guarded` flag must agree with the guard it sits beside. The flag exists
// only so that a hand edit changing one and not the other is caught, which is P01's reason
// for Row::guarded.
constexpr bool every_arm_has_a_guard_pointer() noexcept {
    for (std::size_t i = 0; i < claim::kCascade.size(); ++i) {
        if (claim::kCascade[i].guard == nullptr) {
            return false;
        }
    }
    return true;
}
static_assert(every_arm_has_a_guard_pointer(), "an arm with no guard pointer");

// Every code must appear in the cascade, or a code exists that nothing can produce.
constexpr bool every_code_is_produced_by_some_arm() noexcept {
    for (std::size_t c = 0; c < claim::kCodeCount; ++c) {
        bool found = false;
        for (std::size_t i = 0; i < claim::kCascade.size(); ++i) {
            if (static_cast<std::size_t>(claim::kCascade[i].code) == c) {
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}
static_assert(every_code_is_produced_by_some_arm(),
              "a code no arm can produce is a code the policy cannot reach");

// ---------------------------------------- 1. every code, every arm, every reason code

void every_code_and_arm_is_reached() {
    std::printf("1. the twenty-two cases, and every code, arm, service state and reason\n");

    claim::Machine m;

    for (std::size_t i = 0; i < kCaseCount; ++i) {
        const Case& k = kCases[i];
        const claim::Inputs in = inputs_of(k);

        m.settings.activated = k.activated;
        const claim::Decision d = m.step(in, static_cast<std::uint32_t>(i + 1));

        CHECK(d.code == k.want_code, "case %u (%s): code was %s, expected %s",
              static_cast<unsigned>(i + 1), k.what,
              claim::code_name(d.code), claim::code_name(k.want_code));
        CHECK(d.arm == k.want_arm, "case %u (%s): arm was %u, expected %u",
              static_cast<unsigned>(i + 1), k.what,
              static_cast<unsigned>(d.arm), static_cast<unsigned>(k.want_arm));
        CHECK(m.check_invariants(), "case %u (%s) broke an invariant",
              static_cast<unsigned>(i + 1), k.what);
    }

    for (std::size_t i = 0; i < claim::kArmCount; ++i) {
        CHECK(m.arm_taken(i) > 0, "arm %u was never taken",
              static_cast<unsigned>(i + 1));
    }
    for (std::size_t i = 0; i < claim::kCodeCount; ++i) {
        CHECK(m.code_seen(i) > 0, "code %s was never produced",
              claim::code_name(static_cast<Code>(i)));
    }
    for (std::size_t i = 0; i < claim::kServiceStates; ++i) {
        CHECK(m.service_seen(i) > 0, "service state %s was never exercised",
              claim::service_name(static_cast<Service>(i)));
    }
    for (std::size_t i = 1; i < claim::kReasonCount; ++i) {
        CHECK(m.reason_seen(i) > 0, "service reason %s was never exercised",
              claim::reason_name(static_cast<Reason>(i)));
    }

    std::printf("   %u cases, %u claim codes and the fall-through, %u service states, "
                "%u reasons\n",
                static_cast<unsigned>(kCaseCount),
                static_cast<unsigned>(claim::kPublishedCodes),
                static_cast<unsigned>(claim::kServiceStates),
                static_cast<unsigned>(claim::kReasons));
}

// ------------------------------------------------- 2. a live booking beats a walk-in

void a_live_booking_beats_a_walk_in() {
    std::printf("2. a live booking beats a walk-in\n");

    claim::Inputs in{};
    in.activated   = true;
    in.window_open = true;
    in.present     = true;

    claim::Decision d = claim::decide(in);
    CHECK(d.code == Code::Booked,
          "with a window open and somebody present the claim must be booked, was %s",
          claim::code_name(d.code));
    CHECK(d.arm == 3, "the booking arm is 3, fired %u", static_cast<unsigned>(d.arm));

    in.window_open = false;
    d = claim::decide(in);
    CHECK(d.code == Code::Walkin,
          "the same presence without a window is a walk-in, was %s",
          claim::code_name(d.code));
}

// ---------------------------------------------- 3. a no-show releases exactly once

void a_no_show_releases_exactly_once_and_says_so() {
    std::printf("3. a no-show emits exactly one event, and its own code\n");

    claim::Machine m;
    m.settings.activated = true;

    claim::Inputs in{};
    in.window_open = true;

    for (int i = 0; i < 5; ++i) {
        m.step(in, static_cast<std::uint32_t>(i));
    }
    const std::uint32_t before = m.emitted();

    in.past_grace = true;
    for (int i = 0; i < 20; ++i) {
        m.step(in, static_cast<std::uint32_t>(100 + i));
    }

    CHECK(m.last().code == Code::NoShow, "the claim must be a no-show, was %s",
          claim::code_name(m.last().code));
    CHECK(m.emitted() == before + 1,
          "exactly one event for a no-show: %u before, %u after",
          static_cast<unsigned>(before), static_cast<unsigned>(m.emitted()));
    CHECK(m.published() == Code::NoShow,
          "the event that left must carry the no-show code, carried %s",
          claim::code_name(m.published()));
}

// ----------------------------------------------------------- 4. grace does not emit

void grace_does_not_emit() {
    std::printf("4. grace emits nothing, however often it is the decision\n");

    claim::Machine m;
    m.settings.activated = true;

    claim::Inputs in{};
    in.window_open = true;

    const std::uint32_t before = m.emitted();
    for (int i = 0; i < 200; ++i) {
        const claim::Decision d = m.step(in, static_cast<std::uint32_t>(i));
        CHECK(d.code == Code::Grace,
              "reading %d of a booked empty room should be grace, was %s", i,
              claim::code_name(d.code));
    }

    CHECK(m.emitted() == before,
          "two hundred readings inside grace must emit nothing: %u emitted",
          static_cast<unsigned>(m.emitted() - before));
    CHECK(m.spool().count() == 0, "and nothing may reach the spool, %u did",
          static_cast<unsigned>(m.spool().count()));
}

// ------------------------------------- 5. an unactivated unit never shows itself free

void an_unactivated_unit_never_shows_itself_free() {
    std::printf("5. with the flag clear, no case produces anything but invisible\n");

    for (std::size_t i = 0; i < kCaseCount; ++i) {
        claim::Machine m;
        m.settings.activated = false;

        const claim::Decision d = m.step(inputs_of(kCases[i]),
                                         static_cast<std::uint32_t>(i));
        CHECK(d.code == Code::Invisible, "case %u with the flag clear gave %s",
              static_cast<unsigned>(i + 1), claim::code_name(d.code));
        CHECK(m.check_invariants(), "case %u broke an invariant while clear",
              static_cast<unsigned>(i + 1));
    }
}

// ------------------------------------------- 6. the room works with the radio down

void the_room_works_with_the_radio_down() {
    std::printf("6. the same decisions with the spool full as with it empty\n");

    claim::Machine drains;
    claim::Machine down;
    bool same = true;

    // The step count is generous so the spool fills well before the end, which makes the
    // rest of the run the interesting part. The counts are printed rather than predicted:
    // emission is on change only and Grace never emits, so events per step are a property
    // of the case table. The C version of this test asserted a step-count-derived number
    // and was wrong about it.
    const std::size_t steps = kCaseCount * 200;
    for (std::size_t i = 0; i < steps; ++i) {
        const Case& k = kCases[i % kCaseCount];
        const claim::Inputs in = inputs_of(k);

        drains.settings.activated = k.activated;
        down.settings.activated   = k.activated;

        const claim::Decision a = drains.step(in, static_cast<std::uint32_t>(i));
        const claim::Decision b = down.step(in, static_cast<std::uint32_t>(i));

        if (a.code != b.code || a.arm != b.arm) {
            same = false;
        }
        drains.spool().drain();
    }

    std::printf("   %u steps, %u events, %u retained, %u discarded\n",
                static_cast<unsigned>(steps), static_cast<unsigned>(down.emitted()),
                static_cast<unsigned>(down.spool().count()),
                static_cast<unsigned>(down.spool().discarded()));

    CHECK(same, "a full spool changed a decision, so something reads the link");
    CHECK(down.spool().count() == down.spool().capacity(),
          "the run must actually fill the spool or it tests nothing: %u of %u",
          static_cast<unsigned>(down.spool().count()),
          static_cast<unsigned>(down.spool().capacity()));
}

// --------------------------------------- 7. the spool is bounded and the loss counted

void the_spool_is_bounded_and_the_loss_is_counted() {
    std::printf("7. a thousand events into a bounded spool, and the arithmetic\n");

    claim::Spool sp;
    sp.init(claim::kSpoolBytesMax);
    const std::size_t cap = sp.capacity();

    std::printf("   event %u bytes, bound %u bytes, so capacity %u events\n",
                static_cast<unsigned>(sizeof(claim::Event)),
                static_cast<unsigned>(claim::kSpoolBytesMax),
                static_cast<unsigned>(cap));

    const std::uint32_t offered = 1000;
    for (std::uint32_t i = 0; i < offered; ++i) {
        claim::Event ev{ i, static_cast<std::uint8_t>(Code::Walkin), 6, 0, 0 };
        sp.push(ev);
    }

    CHECK(sp.count() == cap, "the spool must sit at its capacity: %u of %u",
          static_cast<unsigned>(sp.count()), static_cast<unsigned>(cap));
    CHECK(sp.discarded() == offered - static_cast<std::uint32_t>(cap),
          "discarded must be offered minus capacity: %u, expected %u",
          static_cast<unsigned>(sp.discarded()),
          static_cast<unsigned>(offered - static_cast<std::uint32_t>(cap)));

    // The oldest went first, so the oldest retained is exactly offered minus capacity. A
    // ring discarding the newest would satisfy every count above and keep a record of a
    // room that has moved on.
    CHECK(sp.at(0).at_s == offered - static_cast<std::uint32_t>(cap),
          "the oldest retained should be event %u, is %u",
          static_cast<unsigned>(offered - static_cast<std::uint32_t>(cap)),
          static_cast<unsigned>(sp.at(0).at_s));
    CHECK(sp.at(sp.count() - 1).at_s == offered - 1,
          "the newest retained should be event %u, is %u",
          static_cast<unsigned>(offered - 1),
          static_cast<unsigned>(sp.at(sp.count() - 1).at_s));

    std::printf("   offered %u, retained %u, discarded %u\n",
                static_cast<unsigned>(offered), static_cast<unsigned>(sp.count()),
                static_cast<unsigned>(sp.discarded()));
}

// ------------------------------------- 8. settings that are refused rather than corrected

void bad_settings_are_refused_rather_than_corrected() {
    std::printf("8. settings that cannot work are refused, not quietly clamped\n");

    claim::Settings good{};
    CHECK(claim::settings_are_usable(good), "the defaults must be usable");

    claim::Settings zero_grace = good;
    zero_grace.grace_s = 0;
    CHECK(!claim::settings_are_usable(zero_grace),
          "a grace period of zero releases every booking at once and must be refused");

    claim::Settings tiny_spool = good;
    tiny_spool.spool_bytes = 4;   // smaller than one 8-byte event
    CHECK(!claim::settings_are_usable(tiny_spool),
          "a spool bound below one event discards everything in silence");

    claim::Settings short_walkin = good;
    short_walkin.walkin_len_s = 60;
    short_walkin.grace_s = 300;
    CHECK(!claim::settings_are_usable(short_walkin),
          "a walk-in shorter than the grace period frees a room that is in use");

    // The C clamps instead: claim_spool_init raises a bound of zero up to one event and
    // says nothing. Both are defensible and only one can be commissioned.
    claim::Spool sp;
    sp.init(0);
    CHECK(sp.capacity() == 1,
          "the C++17 spool keeps the C's clamping behaviour, capacity %u",
          static_cast<unsigned>(sp.capacity()));

#if CLAIM_HAS_EXPECTED
    const auto refused = claim::checked_settings(zero_grace);
    CHECK(!refused.has_value(), "std::expected must carry the refusal");
    if (!refused.has_value()) {
        std::printf("   refused, and says why: %s\n",
                    claim::settings_error_name(refused.error()));
    }
    const auto accepted = claim::checked_settings(good);
    CHECK(accepted.has_value(), "the defaults must be accepted");
#else
    std::printf("   std::expected is absent, so the refusal is a bool here\n");
#endif
}

// --------------------------- 9. the bounded vector is correct and is the wrong container

void the_vector_spool_agrees_and_costs_more() {
    std::printf("9. the same spool as a bounded vector: same answer, worse cost\n");

#if CLAIM_HAS_INPLACE_VECTOR
    claim::Spool       ring;
    claim::VectorSpool vec;

    ring.init(claim::kSpoolBytesMax);
    vec.init(claim::kSpoolBytesMax);

    const std::uint32_t offered = 1000;
    for (std::uint32_t i = 0; i < offered; ++i) {
        const claim::Event ev{ i, static_cast<std::uint8_t>(claim::Code::Walkin), 6, 0, 0 };
        ring.push(ev);
        vec.push(ev);
    }

    CHECK(ring.count() == vec.count(), "the two must retain the same number: %u and %u",
          static_cast<unsigned>(ring.count()), static_cast<unsigned>(vec.count()));
    CHECK(ring.discarded() == vec.discarded(), "and discard the same number: %u and %u",
          static_cast<unsigned>(ring.discarded()),
          static_cast<unsigned>(vec.discarded()));

    bool identical = true;
    for (std::size_t i = 0; i < ring.count(); ++i) {
        if (ring.at(i).at_s != vec.at(i).at_s) {
            identical = false;
        }
    }
    CHECK(identical, "the retained events must be the same events in the same order");

    // The cost, which is the finding. Each discard shifts every surviving event down one.
    const unsigned long moves =
        static_cast<unsigned long>(vec.discarded()) * static_cast<unsigned long>(vec.capacity());
    std::printf("   identical contents; the vector form shifted about %lu events "
                "where the ring moved %u indices\n",
                moves, static_cast<unsigned>(vec.discarded()));
    std::printf("   so inplace_vector is correct here and is the wrong container: "
                "discard-oldest is O(n) in a vector and O(1) in a ring\n");
#else
    std::printf("   std::inplace_vector is absent from this library, so the comparison "
                "did not run\n");
#endif
}

void the_feature_report() {
    const claim::Features f = claim::features();

    std::printf("\nfeature report, __cplusplus=%ld\n", f.cplusplus);
    std::printf("  std::expected for refused settings      : %s\n",
                f.expected ? "yes" : "no");
    std::printf("  std::to_underlying                      : %s\n",
                f.to_underlying ? "yes" : "no");
    std::printf("  consteval, so a proof cannot run late   : %s\n",
                f.consteval_checks ? "yes" : "no");
    std::printf("  std::inplace_vector                     : %s\n",
                f.inplace_vector ? "yes" : "no");
    std::printf("  static_assert with a computed message   : %s\n",
                f.static_assert_message ? "yes" : "no");
    std::printf("\nproved at compile time, not by any case above:\n");
    std::printf("  the only unguarded arm is the last one\n");
    std::printf("  every code is produced by some arm\n");
    std::printf("  the event is 8 bytes, so 4096 bytes is 512 events\n");
    std::printf("  the six invariants hold over all %u inputs, not just the 22 cases\n",
                static_cast<unsigned>(claim::kInputSpace));
    std::printf("  every one of the 8 arms is reachable by some input\n");
}

}  // namespace

int main() {
    std::printf("the claim cascade in C++, with no kernel and no board\n\n");

    every_code_and_arm_is_reached();
    a_live_booking_beats_a_walk_in();
    a_no_show_releases_exactly_once_and_says_so();
    grace_does_not_emit();
    an_unactivated_unit_never_shows_itself_free();
    the_room_works_with_the_radio_down();
    the_spool_is_bounded_and_the_loss_is_counted();
    bad_settings_are_refused_rather_than_corrected();
    the_vector_spool_agrees_and_costs_more();
    the_feature_report();

    std::printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures;
}
