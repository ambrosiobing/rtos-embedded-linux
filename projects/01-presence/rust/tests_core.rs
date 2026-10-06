//! The C test's cases, in Rust, plus the two checks only Rust can make.
//!
//! Included by each edition crate, so every case runs under 2018, 2021 and 2024.
//! The sequences are the ones in `../c/test_presence.c`, row for row, and that
//! file explains why each exists; they are not re-argued here.
//!
//! Four cases are the Rust-specific part:
//!
//! * `the_exhaustive_match_agrees_with_the_table` holds the exhaustive `match`
//!   against the array for every state, every event kind and both sides of every
//!   guard, which is 192 combinations. That is what makes the claim in the module
//!   documentation checkable rather than merely stated.
//! * `the_table_is_total` walks all twenty-four pairs. In C this is a run-time
//!   test, in C++23 a `consteval`; here it is belt and braces, because the array
//!   lookup could be made partial by deleting a row even though the match cannot.
//! * `the_guard_discriminant_needs_no_parallel_bool` records which rows are
//!   guarded, which the C has to keep in a second field beside each pointer.
//! * `the_guard_option_costs_nothing_and_the_sizes_are_printed` checks that the
//!   `Option<fn>` really is free, since a function pointer cannot be null and the
//!   discriminant occupies that niche, and prints what this representation costs
//!   against the C's bare union.

use super::presence::*;

const NEAR: u16 = 1000;
const FAR: u16 = 9000;

fn start() -> Presence {
    let p = Presence::default();
    assert_eq!(p.state, State::Free);
    assert_eq!(p.last_row, None);
    assert!(p.check_invariants());
    p
}

/// Dispatch and check the invariant after every event, not at the end: a lost
/// release can be transient and still wrong.
fn post(p: &mut Presence, ev: &Ev, want_row: u8) {
    let got = p
        .dispatch(ev)
        .expect("the table is total, so every event has a row");
    assert_eq!(
        got, want_row,
        "expected row {want_row}, took {got} ({})",
        TABLE[got as usize].name
    );
    assert!(
        p.check_invariants(),
        "invariant broken after {:?}: state={:?} run={} hold={}",
        ev.kind,
        p.state,
        p.run,
        p.hold_running
    );
}

#[test]
fn a_sitting_with_one_ordinary_gap() {
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 100), 1);
    assert_eq!(p.run, 1);
    post(&mut p, &Ev::reading(NEAR, 200), 0);
    assert_eq!(p.state, State::Occupied);
    assert_eq!(p.run, 0);

    post(&mut p, &Ev::reading(NEAR, 300), 8);
    post(&mut p, &Ev::reading(FAR, 400), 9);
    assert_eq!(p.state, State::Held);
    assert!(p.is_occupied(), "held still reports occupied");
    assert_eq!(p.releases, 0);

    post(&mut p, &Ev::reading(NEAR, 500), 15);
    assert_eq!(p.state, State::Occupied);

    post(&mut p, &Ev::reading(FAR, 600), 9);
    post(&mut p, &Ev::plain(Kind::Timeout, 30_600), 17);
    assert_eq!(p.state, State::Free);
    assert_eq!(p.releases, 1);
}

#[test]
fn the_arrival_is_on_the_reading_that_completes_the_run() {
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 10), 1);
    post(&mut p, &Ev::reading(NEAR, 20), 0);
    assert_eq!(p.state, State::Occupied);

    // arrive_runs of 1 arrives at once, the edge a `>` rather than `>=` would miss.
    let mut p = start();
    let arrive_on_the_first = Settings {
        arrive_runs: 1,
        hold_ms: 1000,
        range_mm_max: 2500,
    };
    post(&mut p, &Ev::settings(arrive_on_the_first, 5), 7);
    post(&mut p, &Ev::reading(NEAR, 10), 0);
    assert_eq!(p.state, State::Occupied);
}

#[test]
fn row_17_is_the_only_release() {
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 10), 1);
    post(&mut p, &Ev::reading(NEAR, 20), 0);
    post(&mut p, &Ev::reading(FAR, 30), 9);
    assert!(p.hold_running);

    post(&mut p, &Ev::plain(Kind::Tick, 40), 19);
    post(&mut p, &Ev::reading(FAR, 50), 16);
    post(&mut p, &Ev::settings(Settings::default(), 60), 22);
    assert_eq!(p.state, State::Held);
    assert_eq!(p.releases, 0, "nothing but the timeout releases");

    // The hold began at 30, so its expiry is stamped 30_030. Anything earlier is
    // an expiry from some other hold and takes row 18 instead.
    post(&mut p, &Ev::plain(Kind::Timeout, 30_030), 17);
    assert_eq!(p.releases, 1);
}

#[test]
fn a_timer_can_outlive_its_hold() {
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 10), 1);
    post(&mut p, &Ev::reading(NEAR, 20), 0);
    post(&mut p, &Ev::reading(FAR, 30), 9);
    post(&mut p, &Ev::reading(NEAR, 40), 15);
    assert!(!p.hold_running);
    post(&mut p, &Ev::plain(Kind::Timeout, 50), 11);
    assert_eq!(p.state, State::Occupied);
    assert_eq!(p.releases, 0, "a stale timeout is not a release");

    post(&mut p, &Ev::reading(FAR, 60), 9);
    post(&mut p, &Ev::plain(Kind::Button, 70), 20);
    post(&mut p, &Ev::plain(Kind::Timeout, 80), 4);
    assert_eq!(p.state, State::Free);
}

/// The third case of the same phenomenon, and the reason row 17 carries a guard.
///
/// The two in `a_timer_can_outlive_its_hold` arrive in a state that is not holding,
/// so an unguarded row absorbs them. This one arrives while a NEWER hold is running,
/// where an unguarded row 17 would release it up to `hold_ms` early. Take the guard
/// off row 17 and the first assertion below fails: the room goes free with somebody
/// in it. `docs/RTOS_VARIANTS.md` draws the sequence and names the kernel that can
/// deliver it, which is QNX, whose channels are ordered by priority rather than by
/// arrival.
#[test]
fn a_stale_timer_cannot_release_a_newer_hold() {
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 10), 1);
    post(&mut p, &Ev::reading(NEAR, 20), 0);
    post(&mut p, &Ev::reading(FAR, 30), 9);
    assert_eq!(p.hold_due_ms, 30_030, "the first hold is due at 30030");
    post(&mut p, &Ev::reading(NEAR, 40), 15);
    post(&mut p, &Ev::reading(FAR, 50), 9);
    assert_eq!(p.hold_due_ms, 30_050, "the second hold is due at 30050");

    // The first hold's expiry, stamped when it fired rather than when it was read,
    // which is what the adapter contract requires of every adapter.
    post(&mut p, &Ev::plain(Kind::Timeout, 30_030), 18);
    assert_eq!(
        p.state,
        State::Held,
        "a stale expiry must not end a running hold"
    );
    assert_eq!(p.releases, 0, "and must not count as a release");
    assert!(p.hold_running, "the newer hold is still outstanding");

    // The real one still releases, so the guard has not closed the only exit.
    post(&mut p, &Ev::plain(Kind::Timeout, 30_050), 17);
    assert_eq!(p.state, State::Free);
    assert_eq!(p.releases, 1);
}

#[test]
fn a_fault_leaves_only_by_the_button() {
    let mut p = start();
    post(&mut p, &Ev::plain(Kind::Fault, 10), 6);
    assert_eq!(p.faults_latched, 1);
    post(&mut p, &Ev::plain(Kind::Tick, 20), 24);
    post(&mut p, &Ev::reading(NEAR, 30), 25);
    assert_eq!(
        p.state,
        State::Fault,
        "a good reading must not clear a fault"
    );
    post(&mut p, &Ev::plain(Kind::Timeout, 40), 26);
    post(&mut p, &Ev::plain(Kind::Fault, 50), 27);
    post(&mut p, &Ev::settings(Settings::default(), 60), 28);
    post(&mut p, &Ev::plain(Kind::Button, 70), 23);
    assert_eq!(p.state, State::Free);
}

#[test]
fn every_row_is_reachable() {
    let mut taken = [0u32; ROW_COUNT];
    // Zipped rather than indexed. clippy's `needless_range_loop` objects to the
    // index, and its own suggestion keeps one in order to reach into the second
    // array; zipping removes it from both. The two arrays being the same length
    // stops being an assumption the loop bound restates and becomes the shape of
    // the iterator, which is the better reason to write it this way.
    let mut sweep = |p: &Presence| {
        for (total, this_run) in taken.iter_mut().zip(&p.row_taken) {
            *total += this_run;
        }
    };

    // The sequences above, rerun here so the coverage total is one number.
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 10), 1);
    post(&mut p, &Ev::reading(NEAR, 20), 0);
    post(&mut p, &Ev::reading(NEAR, 30), 8);
    post(&mut p, &Ev::reading(FAR, 40), 9);
    post(&mut p, &Ev::reading(NEAR, 50), 15);
    post(&mut p, &Ev::reading(FAR, 60), 9);
    post(&mut p, &Ev::plain(Kind::Tick, 70), 19);
    post(&mut p, &Ev::reading(FAR, 80), 16);
    post(&mut p, &Ev::settings(Settings::default(), 90), 22);
    post(&mut p, &Ev::plain(Kind::Timeout, 30_060), 17);
    sweep(&p);

    let mut p = start();
    post(&mut p, &Ev::reading(FAR, 10), 2);
    post(&mut p, &Ev::plain(Kind::Tick, 20), 3);
    post(&mut p, &Ev::plain(Kind::Timeout, 25), 4);
    post(&mut p, &Ev::plain(Kind::Button, 30), 5);
    post(&mut p, &Ev::plain(Kind::Tick, 40), 10);
    post(&mut p, &Ev::plain(Kind::Timeout, 45), 11);
    post(&mut p, &Ev::settings(Settings::default(), 50), 14);
    post(&mut p, &Ev::plain(Kind::Fault, 60), 13);
    post(&mut p, &Ev::plain(Kind::Button, 70), 23);
    post(&mut p, &Ev::reading(NEAR, 80), 1);
    post(&mut p, &Ev::reading(NEAR, 90), 0);
    post(&mut p, &Ev::plain(Kind::Button, 100), 12);
    post(&mut p, &Ev::plain(Kind::Fault, 110), 6);
    post(&mut p, &Ev::plain(Kind::Tick, 120), 24);
    post(&mut p, &Ev::reading(NEAR, 130), 25);
    post(&mut p, &Ev::plain(Kind::Timeout, 140), 26);
    post(&mut p, &Ev::plain(Kind::Fault, 150), 27);
    post(&mut p, &Ev::settings(Settings::default(), 160), 28);
    post(&mut p, &Ev::plain(Kind::Button, 170), 23);
    post(&mut p, &Ev::reading(NEAR, 180), 1);
    post(&mut p, &Ev::reading(NEAR, 190), 0);
    post(&mut p, &Ev::reading(FAR, 200), 9);
    post(&mut p, &Ev::plain(Kind::Fault, 210), 21);
    post(&mut p, &Ev::plain(Kind::Button, 220), 23);
    post(&mut p, &Ev::settings(Settings::default(), 230), 7);
    post(&mut p, &Ev::reading(NEAR, 240), 1);
    post(&mut p, &Ev::reading(NEAR, 250), 0);
    post(&mut p, &Ev::reading(FAR, 260), 9);
    post(&mut p, &Ev::plain(Kind::Button, 270), 20);
    sweep(&p);

    // Row 18, which no sequence above reaches: an expiry left over from a hold that
    // was cancelled, delivered while a newer hold is running. In C and C++ the case
    // that proves this is in the same accumulating run as the rest; here every test
    // is its own run, so the sequence belongs in this one as well. Its absence is
    // what the assertion below reported the first time this file was compiled with
    // twenty-nine rows, which is the whole purpose of counting.
    let mut p = start();
    post(&mut p, &Ev::reading(NEAR, 10), 1);
    post(&mut p, &Ev::reading(NEAR, 20), 0);
    post(&mut p, &Ev::reading(FAR, 30), 9);
    post(&mut p, &Ev::reading(NEAR, 40), 15);
    post(&mut p, &Ev::reading(FAR, 50), 9);
    post(&mut p, &Ev::plain(Kind::Timeout, 30_030), 18);
    sweep(&p);

    let untaken: Vec<usize> = (0..ROW_COUNT).filter(|&i| taken[i] == 0).collect();
    // The argument is passed rather than captured inline, and the reason is an
    // edition difference worth knowing: `assert!` hands its message straight to
    // `panic!`, and a lone literal is `panic!`'s payload form in edition 2018, not
    // a format string, so the capture would have been printed literally, braces
    // and all, rather than the list of rows. `assert_eq!` wraps its message in
    // `format_args!` and does not have the problem, which is why the captures
    // elsewhere in this file are safe.
    assert!(
        untaken.is_empty(),
        "rows never taken: {:?}. Either unreachable, which is a defect in \
         the table, or a gap in this file",
        untaken
    );
}

#[test]
fn the_exhaustive_match_agrees_with_the_table() {
    // Every state, every kind, and both sides of every guard. This is the check
    // that makes the module's claim about exhaustiveness worth making.
    let states = [State::Free, State::Occupied, State::Held, State::Fault];
    let kinds = [
        Kind::Tick,
        Kind::Reading,
        Kind::Timeout,
        Kind::Button,
        Kind::Fault,
        Kind::Settings,
    ];
    // The three payload dimensions any guard in this table reads, both sides of
    // each: the range, which `in_range` and `out_of_range` split; the run so far,
    // which `run_completes` splits; and the hold's due time against the event's,
    // which `hold_expired` splits. Eight payloads, so 4 x 6 x 8 = 192 combinations.
    // The due dimension arrived with row 18 and is the reason this count grew.
    let payloads = [
        (NEAR, 0u16, 0u32),
        (NEAR, 0, 1000),
        (NEAR, 1, 0),
        (NEAR, 1, 1000),
        (FAR, 0, 0),
        (FAR, 0, 1000),
        (FAR, 1, 0),
        (FAR, 1, 1000),
    ];
    let mut checked = 0;
    for st in states {
        for kind in kinds {
            for (mm, run, due) in payloads {
                // Built in one expression rather than default-then-assign. clippy
                // objects to the latter, and it is right to: a partly initialised
                // value exists between the statements, and here that value breaks
                // the invariant, since `Held` without a running hold is exactly
                // what `check_invariants` rejects.
                let run_for = if st == State::Free { run } else { 0 };
                let p = Presence {
                    state: st,
                    run: run_for,
                    hold_running: st == State::Held,
                    hold_due_ms: due,
                    ..Presence::default()
                };
                let ev = match kind {
                    Kind::Reading => Ev::reading(mm, 0),
                    Kind::Settings => Ev::settings(Settings::default(), 0),
                    k => Ev::plain(k, 0),
                };
                let from_match = p.exhaustive_row_of(&ev);
                let from_table = {
                    let mut probe = p;
                    probe.dispatch(&ev).expect("the table is total")
                };
                assert_eq!(
                    from_match, from_table,
                    "state {st:?} kind {kind:?} mm {mm} run {run} due {due}: \
                     match says {from_match}, table says {from_table}"
                );
                checked += 1;
            }
        }
    }
    assert_eq!(checked, 4 * 6 * 8);
}

#[test]
fn the_table_is_total() {
    let states = [State::Free, State::Occupied, State::Held, State::Fault];
    let kinds = [
        Kind::Tick,
        Kind::Reading,
        Kind::Timeout,
        Kind::Button,
        Kind::Fault,
        Kind::Settings,
    ];
    for st in states {
        for kind in kinds {
            let found = TABLE.iter().any(|r| r.from == st && r.kind == kind);
            assert!(found, "the table has no row for ({:?}, {:?})", st, kind);
        }
    }
    assert_eq!(TABLE.len(), ROW_COUNT);
}

#[test]
fn the_guard_discriminant_needs_no_parallel_bool() {
    // The C carries a `bool guarded` beside its function pointer because gcc 13
    // would not compare the pointer in a constant expression. Here the Option's
    // discriminant is that fact, so this test has nothing to reconcile and exists
    // only to record which rows are guarded.
    let guarded: Vec<usize> = (0..ROW_COUNT)
        .filter(|&i| TABLE[i].guard.is_some())
        .collect();
    assert_eq!(guarded, vec![0, 1, 2, 8, 9, 15, 16, 17], "the guarded rows");
}

#[test]
fn the_guard_option_costs_nothing_and_the_sizes_are_printed() {
    // The design's central claim about the guard, checked rather than asserted in
    // prose: a function pointer cannot be null, so `Option<fn>` occupies that niche
    // and is exactly the size of the bare pointer. The C spends a `bool guarded`
    // beside its pointer because gcc 13 would not compare the pointer in a constant
    // expression. This is the line that says the Rust pays nothing for the same
    // fact, and it would fail if that niche ever stopped being used.
    type G = fn(&Presence, &Ev) -> bool;
    assert_eq!(
        core::mem::size_of::<Option<G>>(),
        core::mem::size_of::<G>(),
        "Option<fn> must cost nothing beyond the pointer itself"
    );

    // Printed, not asserted, because a layout is not a thing to assume stays put.
    // The C pins its event at 20 bytes and its context at 152 with `_Static_assert`;
    // `Option<Settings>` carries a discriminant where the C carries a bare union,
    // and `Option<u8>` does the same where the C spends an `int16_t` and the value
    // minus one. What that costs is a number. `--nocapture` puts it in the log, and
    // it is printed under each edition in case one ever lays these out differently.
    let settings = core::mem::size_of::<Settings>();
    let ev = core::mem::size_of::<Ev>();
    let ctx = core::mem::size_of::<Presence>();
    let row = core::mem::size_of::<Row>();
    println!();
    println!("size report, edition {}", super::EDITION);
    println!("  Settings            {settings:3} B, C 12");
    println!("  Ev                  {ev:3} B, C union 20");
    println!("  Presence            {ctx:3} B, C 156");
    println!("  Row                 {row:3} B, mostly pointers");
}
