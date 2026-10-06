//! The same twenty-two cases and the same seven criteria, in Rust, under three editions.
//!
//! One source compiled by three crates that differ only in their edition line, so "the
//! cascade passes under 2018, 2021 and 2024" is a build result rather than a claim.
//!
//! WHAT IS DIFFERENT HERE AND WHY IT IS WORTH SAYING. The C++ proves the six invariants
//! over the whole input space at COMPILE time. Rust cannot: a `const fn` may not call
//! through a function pointer, so the cascade cannot be walked in a const context, and the
//! same exhaustive check is a test at run time instead. That is a real place where the
//! earlier language wins, and the volume's habit is to record those rather than to let a
//! comparison read as a ranking.

use super::claim::{
    checked_settings, decide, emits, Code, Decision, Event, Guard, Inputs, Machine, Reason,
    Service, Settings, SettingsError, Spool, ARM_COUNT, CODE_COUNT, EVENT_BYTES,
    PUBLISHED_CODES, REASON_COUNT, REASONS, SERVICE_STATES, SPOOL_BYTES_MAX, SPOOL_SLOTS,
};

// ------------------------------------------------------------- the twenty-two cases

struct Case {
    what: &'static str,
    activated: bool,
    window_open: bool,
    present: bool,
    past_grace: bool,
    long_press: bool,
    svc: Service,
    reason: Reason,
    want_code: Code,
    want_arm: u8,
}

#[rustfmt::skip]
static CASES: [Case; 22] = [
    // Arm 1: not commissioned, and it outranks every other input including the service axis.
    Case { what: "uncommissioned and quiet", activated: false, window_open: false, present: false, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::SetupIncomplete, want_code: Code::Invisible, want_arm: 1 },
    Case { what: "uncommissioned, booked, holder present", activated: false, window_open: true, present: true, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::Invisible, want_arm: 1 },
    Case { what: "uncommissioned, somebody walks in", activated: false, window_open: false, present: true, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::Invisible, want_arm: 1 },
    Case { what: "uncommissioned and out of service", activated: false, window_open: false, present: false, past_grace: false, long_press: false, svc: Service::OutOfService, reason: Reason::Power, want_code: Code::Invisible, want_arm: 1 },
    // Arm 2: all four blocking states, the last showing it outranks the no-show arm.
    Case { what: "out of service with the holder present", activated: true, window_open: true, present: true, past_grace: false, long_press: false, svc: Service::OutOfService, reason: Reason::Power, want_code: Code::Rejected, want_arm: 2 },
    Case { what: "somebody is working in the room", activated: true, window_open: false, present: true, past_grace: false, long_press: false, svc: Service::InMaintenance, reason: Reason::Commanded, want_code: Code::Rejected, want_arm: 2 },
    Case { what: "a part is missing", activated: true, window_open: false, present: false, past_grace: false, long_press: false, svc: Service::NeedsPart, reason: Reason::Fan, want_code: Code::Rejected, want_arm: 2 },
    Case { what: "superseded, and past grace", activated: true, window_open: true, present: false, past_grace: true, long_press: false, svc: Service::Replaced, reason: Reason::Commanded, want_code: Code::Rejected, want_arm: 2 },
    // Arm 3: the two non-blocking fault states do not take the room out of use.
    Case { what: "booked and the holder is here", activated: true, window_open: true, present: true, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::Booked, want_arm: 3 },
    Case { what: "booked and here, fan needs cleaning", activated: true, window_open: true, present: true, past_grace: false, long_press: false, svc: Service::Degraded, reason: Reason::Fan, want_code: Code::Booked, want_arm: 3 },
    Case { what: "booked and here, sensor due for service", activated: true, window_open: true, present: true, past_grace: false, long_press: false, svc: Service::NeedsService, reason: Reason::Sensor, want_code: Code::Booked, want_arm: 3 },
    // Arm 4: booked, nobody here yet, inside the grace period.
    Case { what: "booked and nobody has arrived yet", activated: true, window_open: true, present: false, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::Grace, want_arm: 4 },
    Case { what: "booked, nobody yet, modem due for service", activated: true, window_open: true, present: false, past_grace: false, long_press: false, svc: Service::NeedsService, reason: Reason::Modem, want_code: Code::Grace, want_arm: 4 },
    // Arm 5: the grace period ran out.
    Case { what: "nobody came", activated: true, window_open: true, present: false, past_grace: true, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::NoShow, want_arm: 5 },
    Case { what: "nobody came, and the panel is faulty", activated: true, window_open: true, present: false, past_grace: true, long_press: false, svc: Service::Degraded, reason: Reason::Panel, want_code: Code::NoShow, want_arm: 5 },
    // Arm 6: a walk-in, and a press by somebody still in the room is still a walk-in.
    Case { what: "somebody walks into an unbooked room", activated: true, window_open: false, present: true, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::Walkin, want_arm: 6 },
    Case { what: "a walk-in who presses while still in the room", activated: true, window_open: false, present: true, past_grace: false, long_press: true, svc: Service::Ok, reason: Reason::None, want_code: Code::Walkin, want_arm: 6 },
    // Arm 7: be right back, reachable only when nobody is present.
    Case { what: "pressed the button on the way out", activated: true, window_open: false, present: false, past_grace: false, long_press: true, svc: Service::Ok, reason: Reason::None, want_code: Code::Brb, want_arm: 7 },
    Case { what: "pressed on the way out, fan needs cleaning", activated: true, window_open: false, present: false, past_grace: false, long_press: true, svc: Service::Degraded, reason: Reason::Fan, want_code: Code::Brb, want_arm: 7 },
    // Arm 8: the fall-through, which is not one of the seven codes.
    Case { what: "nothing at all is happening", activated: true, window_open: false, present: false, past_grace: false, long_press: false, svc: Service::Ok, reason: Reason::None, want_code: Code::Free, want_arm: 8 },
    Case { what: "nothing happening, sensor due for service", activated: true, window_open: false, present: false, past_grace: false, long_press: false, svc: Service::NeedsService, reason: Reason::Sensor, want_code: Code::Free, want_arm: 8 },
    // And grace outranks a press.
    Case { what: "a press during grace creates no claim", activated: true, window_open: true, present: false, past_grace: false, long_press: true, svc: Service::Ok, reason: Reason::None, want_code: Code::Grace, want_arm: 4 },
];

fn inputs_of(k: &Case) -> Inputs {
    Inputs {
        activated: k.activated,
        window_open: k.window_open,
        present: k.present,
        past_grace: k.past_grace,
        long_press_pending: k.long_press,
        service: k.svc,
        service_reason: k.reason,
    }
}

// ----------------------------------------- 1. every code, every arm, every reason code

#[test]
fn every_code_and_arm_is_reached() {
    let mut m = Machine::default();

    for (i, k) in CASES.iter().enumerate() {
        m.settings.activated = k.activated;
        let d = m.step(&inputs_of(k), (i + 1) as u32);

        assert_eq!(
            d.code,
            k.want_code,
            "case {} ({}): wrong code",
            i + 1,
            k.what
        );
        assert_eq!(d.arm, k.want_arm, "case {} ({}): wrong arm", i + 1, k.what);
        assert!(
            m.check_invariants(),
            "case {} ({}) broke an invariant",
            i + 1,
            k.what
        );
    }

    for i in 0..ARM_COUNT {
        assert!(m.arm_taken(i) > 0, "arm {} was never taken", i + 1);
    }
    for c in Code::ALL.iter() {
        assert!(
            m.code_seen(c.index()) > 0,
            "code {} was never produced",
            c.name()
        );
    }
    for s in Service::ALL.iter() {
        assert!(
            m.service_seen(s.index()) > 0,
            "service state {} was never exercised",
            s.name()
        );
    }
    for r in Reason::ALL.iter() {
        if r.index() == 0 {
            continue; // None is not one of the chapter's seven
        }
        assert!(
            m.reason_seen(r.index()) > 0,
            "service reason {} was never exercised",
            r.name()
        );
    }

    println!(
        "{} cases, {} claim codes and the fall-through, {} service states, {} reasons",
        CASES.len(),
        PUBLISHED_CODES,
        SERVICE_STATES,
        REASONS
    );
}

// ------------------------------------------------- 2. a live booking beats a walk-in

#[test]
fn a_live_booking_beats_a_walk_in() {
    let mut input = Inputs {
        activated: true,
        window_open: true,
        present: true,
        ..Inputs::default()
    };

    let d = decide(&input);
    assert_eq!(d.code, Code::Booked, "a booked room with its holder present");
    assert_eq!(d.arm, 3, "the booking arm is 3");

    input.window_open = false;
    assert_eq!(
        decide(&input).code,
        Code::Walkin,
        "the same presence without a window is a walk-in"
    );
}

// ---------------------------------------------- 3. a no-show releases exactly once

#[test]
fn a_no_show_releases_exactly_once_and_says_so() {
    let mut m = Machine::default();
    m.settings.activated = true;

    let mut input = Inputs {
        window_open: true,
        ..Inputs::default()
    };

    for i in 0..5u32 {
        m.step(&input, i);
    }
    let before = m.emitted();

    input.past_grace = true;
    for i in 0..20u32 {
        m.step(&input, 100 + i);
    }

    assert_eq!(m.last().code, Code::NoShow, "the claim must be a no-show");
    assert_eq!(m.emitted(), before + 1, "exactly one event for a no-show");
    assert_eq!(
        m.published(),
        Code::NoShow,
        "the event that left must carry the no-show code"
    );
}

// ----------------------------------------------------------- 4. grace does not emit

#[test]
fn grace_does_not_emit() {
    let mut m = Machine::default();
    m.settings.activated = true;

    let input = Inputs {
        window_open: true,
        ..Inputs::default()
    };

    let before = m.emitted();
    for i in 0..200u32 {
        assert_eq!(
            m.step(&input, i).code,
            Code::Grace,
            "a booked empty room inside grace"
        );
    }

    assert_eq!(m.emitted(), before, "grace must emit nothing");
    assert_eq!(m.spool().count(), 0, "and nothing may reach the spool");
}

// ------------------------------------- 5. an unactivated unit never shows itself free

#[test]
fn an_unactivated_unit_never_shows_itself_free() {
    for (i, k) in CASES.iter().enumerate() {
        let mut m = Machine::default();
        m.settings.activated = false;

        let d = m.step(&inputs_of(k), i as u32);
        assert_eq!(
            d.code,
            Code::Invisible,
            "case {} with the flag clear",
            i + 1
        );
        assert!(m.check_invariants(), "case {} broke an invariant", i + 1);
    }
}

// ------------------------------------------- 6. the room works with the radio down

#[test]
fn the_room_works_with_the_radio_down() {
    let mut drains = Machine::default();
    let mut down = Machine::default();

    let steps = CASES.len() * 200;
    for i in 0..steps {
        let k = &CASES[i % CASES.len()];
        let input = inputs_of(k);

        drains.settings.activated = k.activated;
        down.settings.activated = k.activated;

        let a = drains.step(&input, i as u32);
        let b = down.step(&input, i as u32);
        assert_eq!(a, b, "a full spool changed a decision at step {}", i);

        drains.spool_mut().drain();
    }

    println!(
        "{} steps, {} events, {} retained, {} discarded",
        steps,
        down.emitted(),
        down.spool().count(),
        down.spool().discarded()
    );

    assert_eq!(
        down.spool().count(),
        down.spool().capacity(),
        "the run must actually fill the spool or it tests nothing"
    );
    assert!(down.check_invariants(), "invariants with the radio down");
}

// --------------------------------------- 7. the spool is bounded and the loss counted

#[test]
fn the_spool_is_bounded_and_the_loss_is_counted() {
    let mut sp = Spool::default();
    sp.init(SPOOL_BYTES_MAX);
    let cap = sp.capacity();

    println!(
        "event {} bytes, bound {} bytes, so capacity {} events",
        EVENT_BYTES, SPOOL_BYTES_MAX, cap
    );

    let offered: u32 = 1000;
    for i in 0..offered {
        sp.push(Event {
            at_s: i,
            code: Code::Walkin.index() as u8,
            arm: 6,
            service: 0,
            service_reason: 0,
        });
    }

    assert_eq!(sp.count(), cap, "the spool must sit at its capacity");
    assert_eq!(
        sp.discarded(),
        offered - cap as u32,
        "discarded must be offered minus capacity"
    );
    assert_eq!(
        sp.count() as u32 + sp.discarded(),
        offered,
        "retained plus discarded must equal offered"
    );

    // The oldest went first. A ring discarding the newest would satisfy every count above
    // and keep a record of a room that has moved on.
    assert_eq!(
        sp.at(0).at_s,
        offered - cap as u32,
        "the oldest retained is offered minus capacity"
    );
    assert_eq!(
        sp.at(sp.count() - 1).at_s,
        offered - 1,
        "the newest retained is the last offered"
    );

    println!(
        "offered {}, retained {}, discarded {}",
        offered,
        sp.count(),
        sp.discarded()
    );
}

// ------------------------- 8. settings refused, which Rust has had since before 2018

#[test]
fn bad_settings_are_refused_rather_than_corrected() {
    assert!(
        checked_settings(Settings::default()).is_ok(),
        "the defaults must be usable"
    );

    let zero_grace = Settings {
        grace_s: 0,
        ..Settings::default()
    };
    assert_eq!(
        checked_settings(zero_grace),
        Err(SettingsError::GraceIsZero),
        "a grace period of zero releases every booking at once"
    );

    let tiny_spool = Settings {
        spool_bytes: 4,
        ..Settings::default()
    };
    assert_eq!(
        checked_settings(tiny_spool),
        Err(SettingsError::SpoolHoldsNoEvents),
        "a spool bound below one event discards everything in silence"
    );

    let short_walkin = Settings {
        walkin_len_s: 60,
        grace_s: 300,
        ..Settings::default()
    };
    assert_eq!(
        checked_settings(short_walkin),
        Err(SettingsError::WalkinShorterThanGrace),
        "a walk-in shorter than grace frees a room in use"
    );

    // The C clamps instead and says nothing, and the Rust spool keeps that behaviour so the
    // two sit side by side rather than one being quietly replaced.
    let mut sp = Spool::default();
    sp.init(0);
    assert_eq!(sp.capacity(), 1, "a zero bound is clamped, as in the C");

    println!(
        "a refusal carries its reason: {}",
        SettingsError::GraceIsZero.name()
    );
}

// ------------------- 9. the whole input space, which in Rust is a test and not a proof

#[test]
fn the_invariants_hold_over_every_input() {
    let mut arms_seen = [false; ARM_COUNT];
    let mut checked = 0u32;

    for bits in 0u32..32 {
        for svc in Service::ALL.iter() {
            let input = Inputs {
                activated: (bits & 1) != 0,
                window_open: (bits & 2) != 0,
                present: (bits & 4) != 0,
                past_grace: (bits & 8) != 0,
                long_press_pending: (bits & 16) != 0,
                service: *svc,
                service_reason: Reason::None,
            };

            let d = decide(&input);
            checked += 1;
            arms_seen[d.arm as usize - 1] = true;

            assert!(d.arm >= 1 && d.arm as usize <= ARM_COUNT, "some arm fires");

            if !input.activated {
                assert_eq!(d.code, Code::Invisible, "an unactivated unit is invisible");
            }
            if input.activated && input.service.blocks() {
                assert_eq!(d.code, Code::Rejected, "a blocking service state rejects");
            }
            if d.code == Code::Walkin {
                assert!(!input.window_open, "a booking always beats a walk-in");
            }
            if d.code == Code::Grace {
                assert!(!input.present, "grace means nobody has arrived");
            }
            if d.code == Code::NoShow {
                assert!(
                    input.window_open && !input.present && input.past_grace,
                    "a no-show needs an open window, an empty room and an expired period"
                );
            }
            if d.code == Code::Booked {
                assert!(
                    input.window_open && input.present,
                    "booked needs the holder present"
                );
            }
            if d.code == Code::Brb {
                assert!(!input.present, "be-right-back is for a room nobody is in");
            }
        }
    }

    for (i, seen) in arms_seen.iter().enumerate() {
        assert!(*seen, "arm {} is unreachable by any input", i + 1);
    }

    println!(
        "the invariants hold over all {} inputs, and every arm is reachable",
        checked
    );
    println!(
        "in Rust this is a test; in C++ the same check is a compile-time proof, because a \
         const fn may not call through a function pointer"
    );
}

// ----------------------------------------------- 10. what the representation costs

#[test]
fn the_guard_option_costs_nothing_and_the_sizes_are_printed() {
    use core::mem::size_of;

    // A function pointer cannot be null, so Option<fn> uses the niche. The C++ writes an
    // `always` predicate plus a separate `guarded` bool to say the same thing.
    assert_eq!(
        size_of::<Option<Guard>>(),
        size_of::<Guard>(),
        "Option<fn> must cost nothing, or the unguarded arm is paying for its clarity"
    );

    println!("Option<Guard> {} bytes, Guard {} bytes", size_of::<Option<Guard>>(), size_of::<Guard>());
    println!("Event {} bytes, the C and C++ pin theirs at 8", EVENT_BYTES);
    println!("Inputs {} bytes, Decision {} bytes", size_of::<Inputs>(), size_of::<Decision>());
    println!("Code {} bytes, Service {} bytes", size_of::<Code>(), size_of::<Service>());

    // The spool arithmetic depends on this, and Rust may reorder fields where the C may
    // not, so it is printed and asserted rather than assumed.
    assert_eq!(
        EVENT_BYTES, 8,
        "the event must be 8 bytes for 4096 to buy 512 of them"
    );
    assert_eq!(SPOOL_SLOTS * EVENT_BYTES, SPOOL_BYTES_MAX as usize);
    assert_eq!(CODE_COUNT, 8);
    assert_eq!(REASON_COUNT, 8);
}

// -------------------------------------------------- 11. the emission rule in isolation

#[test]
fn only_a_changed_code_emits_and_grace_never_does() {
    assert!(emits(Code::Free, Code::Walkin), "a change emits");
    assert!(!emits(Code::Walkin, Code::Walkin), "no change does not");
    assert!(!emits(Code::Free, Code::Grace), "grace never emits");
    assert!(!emits(Code::Grace, Code::Grace), "and still does not");
    assert!(
        emits(Code::Grace, Code::NoShow),
        "leaving grace for a no-show emits once"
    );
}
