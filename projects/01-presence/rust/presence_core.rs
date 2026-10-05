//! The same twenty-eight rows, in Rust. One source, three editions.
//!
//! This file is included by `e2018/`, `e2021/` and `e2024/`, each a crate whose
//! only difference is its `edition` in `Cargo.toml`. So "compiles under three
//! editions" is a build result rather than an assertion, and an edition needing
//! its own source would show up as a build failure rather than as a footnote.
//!
//! `no_std` outside tests, no allocation, no `unsafe`, no panic on a reachable
//! path: this compiles for the node as well as for the host.
//!
//! The `no_std` attribute itself is deliberately NOT in this file. It is a crate
//! attribute, and in a file included as a module it is inert: rustc reports
//! `unused_attributes`, and because each crate denies warnings it refuses to
//! build. An earlier draft had it here, where it said nothing. The three crate
//! roots carry it instead, which are the only three places that can carry it, so
//! the central claim about the node is now made where it has an effect. The build
//! is what found the line that had none.
//!
//! # The table is an array, as in the C
//!
//! An earlier draft expressed the table as an exhaustive `match` on
//! `(State, Event)`, which is attractive because the compiler then rejects a
//! partial table outright: the property the C proves with a run-time test and
//! C++23 with a `consteval` check would be a condition of compiling at all.
//!
//! It was rewritten as an array anyway, and the reason is worth recording. The
//! match split the table across three functions, one for the row, one for the
//! destination and one for the action, which is three places for twenty-eight
//! rows to drift between where the C has one. The mechanical cross-check that
//! compares all three languages row for row could not read it either. Parity
//! across the three implementations is the point of this exercise, so the array
//! wins and the exhaustiveness is recovered by `exhaustive_row_of` below, which
//! IS a match and which a test holds against the table on every combination.
//!
//! That is the honest shape of the Rust finding: the language offers a stronger
//! guarantee than the other two, and taking it would have cost the comparison
//! this project exists to make, so it is demonstrated and cross-checked instead
//! of adopted.

// These two are lint attributes, which a module may carry and which apply to
// everything in it. `no_std` is not one of those; see the note above.
#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// What the node is reporting about the room.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum State {
    /// Nobody detected, and no hold outstanding.
    Free,
    /// A run of in-range readings has been seen.
    Occupied,
    /// Readings stopped, the hold timer runs, still reported occupied.
    Held,
    /// A read failed; the state is not trustworthy and says so.
    Fault,
}

/// An event's kind, without its payload.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Kind {
    /// The kernel timer, the heartbeat.
    Tick,
    /// A range reading.
    Reading,
    /// The hold timer expiring.
    Timeout,
    /// The user button, a forced event.
    Button,
    /// Any failed read.
    Fault,
    /// New thresholds arriving.
    Settings,
}

/// Configuration, not constants. The defaults are in `docs/DESIGN.md`, where they
/// are described as arguable rather than measured.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct Settings {
    /// Readings in a row before an arrival.
    pub arrive_runs: u16,
    /// How long a hold outlives the last reading.
    pub hold_ms: u32,
    /// Above this range, nobody is there.
    pub range_mm_max: u16,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            arrive_runs: 2,
            hold_ms: 30_000,
            range_mm_max: 2500,
        }
    }
}

/// One event, with its payload where it has one.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct Ev {
    /// Which kind of event this is.
    pub kind: Kind,
    /// The kernel's monotonic time.
    pub at_ms: u32,
    /// A range in millimetres, for `Kind::Reading`.
    pub range_mm: u16,
    /// Replacement settings, for `Kind::Settings`.
    pub settings: Option<Settings>,
}

impl Ev {
    /// A reading at a time.
    pub fn reading(range_mm: u16, at_ms: u32) -> Self {
        Self {
            kind: Kind::Reading,
            at_ms,
            range_mm,
            settings: None,
        }
    }
    /// An event with no payload.
    pub fn plain(kind: Kind, at_ms: u32) -> Self {
        Self {
            kind,
            at_ms,
            range_mm: 0,
            settings: None,
        }
    }
    /// A settings change.
    pub fn settings(s: Settings, at_ms: u32) -> Self {
        Self {
            kind: Kind::Settings,
            at_ms,
            range_mm: 0,
            settings: Some(s),
        }
    }
}

/// Why a dispatch produced no transition. There is no dropped-event variant: the
/// table is total, so a lookup that found nothing is a defect.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Error {
    /// No row matched. Unreachable while the table is total.
    NoRow,
}

/// How many rows the table has.
pub const ROW_COUNT: usize = 28;

/// A guard decides whether its row applies.
///
/// `Option<fn>` rather than a nullable pointer, and this is where Rust removes
/// something the C needed: `presence.h` carries a `bool guarded` beside the
/// pointer because gcc 13 would not compare a function pointer with null in a
/// constant expression. Here the discriminant *is* that bool, so there is no
/// duplicated fact and nothing to keep in step.
type Guard = fn(&Presence, &Ev) -> bool;
type Action = fn(&mut Presence, &Ev);

/// One row of the table.
pub struct Row {
    /// The state this row leaves.
    pub from: State,
    /// The event kind it responds to.
    pub kind: Kind,
    /// Its guard, or `None` when unguarded.
    pub guard: Option<Guard>,
    /// What it does.
    pub action: Action,
    /// Where it arrives.
    pub to: State,
    /// Its name, for a failure message.
    pub name: &'static str,
}

// ----------------------------------------------------------------- guards
fn in_range(p: &Presence, ev: &Ev) -> bool {
    ev.range_mm <= p.settings.range_mm_max
}
fn out_of_range(p: &Presence, ev: &Ev) -> bool {
    ev.range_mm > p.settings.range_mm_max
}
fn run_completes(p: &Presence, ev: &Ev) -> bool {
    // The C's `arrive_runs == 0 ? 1 : arrive_runs`, which `max` says in one word.
    let needed = p.settings.arrive_runs.max(1);
    in_range(p, ev) && u32::from(p.run) + 1 >= u32::from(needed)
}

// ---------------------------------------------------------------- actions
fn none(_p: &mut Presence, _ev: &Ev) {}
fn count_run(p: &mut Presence, ev: &Ev) {
    p.run = p.run.saturating_add(1);
    p.last_reading_ms = ev.at_ms;
}
fn reset_run(p: &mut Presence, ev: &Ev) {
    p.run = 0;
    p.last_reading_ms = ev.at_ms;
}
fn on_arrive(p: &mut Presence, ev: &Ev) {
    p.run = 0;
    p.hold_running = false;
    p.last_reading_ms = ev.at_ms;
}
fn refresh(p: &mut Presence, ev: &Ev) {
    p.last_reading_ms = ev.at_ms;
}
fn start_hold(p: &mut Presence, ev: &Ev) {
    p.hold_running = true;
    p.hold_due_ms = ev.at_ms.wrapping_add(p.settings.hold_ms);
    p.last_reading_ms = ev.at_ms;
}
fn cancel_hold(p: &mut Presence, ev: &Ev) {
    p.hold_running = false;
    p.last_reading_ms = ev.at_ms;
}
/// Row 17. The only release in the table.
fn release(p: &mut Presence, _ev: &Ev) {
    p.hold_running = false;
    p.run = 0;
    p.releases += 1;
}
fn force_occupied(p: &mut Presence, _ev: &Ev) {
    p.run = 0;
    p.hold_running = false;
}
fn force_free(p: &mut Presence, _ev: &Ev) {
    p.run = 0;
    p.hold_running = false;
}
fn latch_fault(p: &mut Presence, _ev: &Ev) {
    p.hold_running = false;
    p.run = 0;
    p.faults_latched += 1;
}
fn clear_fault(p: &mut Presence, _ev: &Ev) {
    p.run = 0;
    p.hold_running = false;
}
fn apply_settings(p: &mut Presence, ev: &Ev) {
    if let Some(mut s) = ev.settings {
        if s.arrive_runs == 0 {
            s.arrive_runs = 1;
        }
        p.settings = s;
    }
}

/// The twenty-eight rows, in the C table's order, row for row.
///
/// Formatting is suppressed here on purpose, and this is the one place in the
/// crate where that is true. rustfmt's `struct_lit_width` is 18 characters, so by
/// default every row becomes an eight-line block and the table becomes two hundred
/// and twenty-four lines. One row per line is not a preference: `presence.c` uses
/// a `ROW(...)` macro to get exactly this shape, and reading the two tables beside
/// each other is how a row that has drifted is found. A reader comparing them
/// should be comparing rows, not counting braces.
#[rustfmt::skip]
pub static TABLE: [Row; ROW_COUNT] = [
    // 0 to 7: Free
    Row { from: State::Free, kind: Kind::Reading, guard: Some(run_completes), action: on_arrive, to: State::Occupied, name: "free reading run_completes" },
    Row { from: State::Free, kind: Kind::Reading, guard: Some(in_range), action: count_run, to: State::Free, name: "free reading in_range" },
    Row { from: State::Free, kind: Kind::Reading, guard: Some(out_of_range), action: reset_run, to: State::Free, name: "free reading out_of_range" },
    Row { from: State::Free, kind: Kind::Tick, guard: None, action: none, to: State::Free, name: "free tick" },
    Row { from: State::Free, kind: Kind::Timeout, guard: None, action: none, to: State::Free, name: "free timeout stale" },
    Row { from: State::Free, kind: Kind::Button, guard: None, action: force_occupied, to: State::Occupied, name: "free button" },
    Row { from: State::Free, kind: Kind::Fault, guard: None, action: latch_fault, to: State::Fault, name: "free fault" },
    Row { from: State::Free, kind: Kind::Settings, guard: None, action: apply_settings, to: State::Free, name: "free settings" },
    // 8 to 14: Occupied
    Row { from: State::Occupied, kind: Kind::Reading, guard: Some(in_range), action: refresh, to: State::Occupied, name: "occupied reading in_range" },
    Row { from: State::Occupied, kind: Kind::Reading, guard: Some(out_of_range), action: start_hold, to: State::Held, name: "occupied reading out_of_range" },
    Row { from: State::Occupied, kind: Kind::Tick, guard: None, action: none, to: State::Occupied, name: "occupied tick" },
    Row { from: State::Occupied, kind: Kind::Timeout, guard: None, action: none, to: State::Occupied, name: "occupied timeout stale" },
    Row { from: State::Occupied, kind: Kind::Button, guard: None, action: force_free, to: State::Free, name: "occupied button" },
    Row { from: State::Occupied, kind: Kind::Fault, guard: None, action: latch_fault, to: State::Fault, name: "occupied fault" },
    Row { from: State::Occupied, kind: Kind::Settings, guard: None, action: apply_settings, to: State::Occupied, name: "occupied settings" },
    // 15 to 21: Held
    Row { from: State::Held, kind: Kind::Reading, guard: Some(in_range), action: cancel_hold, to: State::Occupied, name: "held reading in_range" },
    Row { from: State::Held, kind: Kind::Reading, guard: Some(out_of_range), action: none, to: State::Held, name: "held reading out_of_range" },
    Row { from: State::Held, kind: Kind::Timeout, guard: None, action: release, to: State::Free, name: "held timeout release" },
    Row { from: State::Held, kind: Kind::Tick, guard: None, action: none, to: State::Held, name: "held tick" },
    Row { from: State::Held, kind: Kind::Button, guard: None, action: force_free, to: State::Free, name: "held button" },
    Row { from: State::Held, kind: Kind::Fault, guard: None, action: latch_fault, to: State::Fault, name: "held fault" },
    Row { from: State::Held, kind: Kind::Settings, guard: None, action: apply_settings, to: State::Held, name: "held settings" },
    // 22 to 27: Fault, a latch left only by the button
    Row { from: State::Fault, kind: Kind::Button, guard: None, action: clear_fault, to: State::Free, name: "fault button" },
    Row { from: State::Fault, kind: Kind::Tick, guard: None, action: none, to: State::Fault, name: "fault tick" },
    Row { from: State::Fault, kind: Kind::Reading, guard: None, action: none, to: State::Fault, name: "fault reading" },
    Row { from: State::Fault, kind: Kind::Timeout, guard: None, action: none, to: State::Fault, name: "fault timeout" },
    Row { from: State::Fault, kind: Kind::Fault, guard: None, action: none, to: State::Fault, name: "fault fault" },
    Row { from: State::Fault, kind: Kind::Settings, guard: None, action: apply_settings, to: State::Fault, name: "fault settings" },
];

/// The node's state, its settings and its counters.
#[derive(Copy, Clone, Debug)]
pub struct Presence {
    /// What the node is reporting.
    pub state: State,
    /// The thresholds in force.
    pub settings: Settings,
    /// Consecutive in-range readings, towards an arrival.
    pub run: u16,
    /// Whether a hold timer is outstanding.
    pub hold_running: bool,
    /// When that hold expires, meaningful only while `hold_running`.
    pub hold_due_ms: u32,
    /// When the last reading arrived.
    pub last_reading_ms: u32,
    /// How many times each row has been taken.
    pub row_taken: [u32; ROW_COUNT],
    /// The row the last event took, or `None` before the first.
    pub last_row: Option<u8>,
    /// Row 17, the only release.
    pub releases: u32,
    /// How many times a fault has latched.
    pub faults_latched: u32,
}

impl Default for Presence {
    fn default() -> Self {
        Self::new(Settings::default())
    }
}

impl Presence {
    /// A node in `Free`. An `arrive_runs` of zero is raised to one, as in the C.
    pub fn new(mut settings: Settings) -> Self {
        if settings.arrive_runs == 0 {
            settings.arrive_runs = 1;
        }
        Self {
            state: State::Free,
            settings,
            run: 0,
            hold_running: false,
            hold_due_ms: 0,
            last_reading_ms: 0,
            row_taken: [0; ROW_COUNT],
            last_row: None,
            releases: 0,
            faults_latched: 0,
        }
    }

    /// True while the room should be reported in use. `Held` reports occupied:
    /// that is the entire purpose of the state.
    pub fn is_occupied(&self) -> bool {
        matches!(self.state, State::Occupied | State::Held)
    }

    /// Dispatch one event and return the row index taken.
    ///
    /// The row index is the return value rather than a field written as a side
    /// effect, which is what C++23 needed `std::expected` for and what the C does
    /// not do at all. `#[must_use]` makes ignoring the error a warning, and
    /// `-D warnings` makes that a build failure.
    #[must_use = "the row index taken, or the reason the table had no row"]
    pub fn dispatch(&mut self, ev: &Ev) -> Result<u8, Error> {
        for (i, row) in TABLE.iter().enumerate() {
            if row.from != self.state || row.kind != ev.kind {
                continue;
            }
            // The C writes this as one condition, `row->guarded && !row->guard(p, ev)`,
            // and `Option::is_some_and` is that same single condition. The nested
            // `if let` this replaces is what clippy asks to be collapsed into a
            // let-chain under edition 2024, a form editions 2018 and 2021 cannot
            // parse. One source for three editions cannot accept that offer, so it
            // takes this one, which has been available in every edition since 1.70.
            if row.guard.is_some_and(|g| !g(self, ev)) {
                continue;
            }
            (row.action)(self, ev);
            self.state = row.to;
            self.last_row = Some(i as u8);
            self.row_taken[i] += 1;
            return Ok(i as u8);
        }
        self.last_row = None;
        Err(Error::NoRow)
    }

    /// The same lookup as an exhaustive `match`, which the compiler will not let
    /// be partial.
    ///
    /// This is not what `dispatch` uses, and the module documentation says why.
    /// It exists so the stronger guarantee Rust offers is demonstrated and
    /// checked: `tests_core` holds this against `TABLE` for every state, every
    /// kind and both sides of every guard. If a row is ever deleted from `TABLE`,
    /// the array lookup starts returning `NoRow` and this function stops agreeing
    /// with it, and the test names the pair.
    pub fn exhaustive_row_of(&self, ev: &Ev) -> u8 {
        match (self.state, ev.kind) {
            (State::Free, Kind::Reading) => {
                if run_completes(self, ev) {
                    0
                } else if in_range(self, ev) {
                    1
                } else {
                    2
                }
            }
            (State::Free, Kind::Tick) => 3,
            (State::Free, Kind::Timeout) => 4,
            (State::Free, Kind::Button) => 5,
            (State::Free, Kind::Fault) => 6,
            (State::Free, Kind::Settings) => 7,
            (State::Occupied, Kind::Reading) => {
                if in_range(self, ev) {
                    8
                } else {
                    9
                }
            }
            (State::Occupied, Kind::Tick) => 10,
            (State::Occupied, Kind::Timeout) => 11,
            (State::Occupied, Kind::Button) => 12,
            (State::Occupied, Kind::Fault) => 13,
            (State::Occupied, Kind::Settings) => 14,
            (State::Held, Kind::Reading) => {
                if in_range(self, ev) {
                    15
                } else {
                    16
                }
            }
            (State::Held, Kind::Timeout) => 17,
            (State::Held, Kind::Tick) => 18,
            (State::Held, Kind::Button) => 19,
            (State::Held, Kind::Fault) => 20,
            (State::Held, Kind::Settings) => 21,
            (State::Fault, Kind::Button) => 22,
            (State::Fault, Kind::Tick) => 23,
            (State::Fault, Kind::Reading) => 24,
            (State::Fault, Kind::Timeout) => 25,
            (State::Fault, Kind::Fault) => 26,
            (State::Fault, Kind::Settings) => 27,
        }
    }

    /// The invariant from `docs/DESIGN.md`: a release cannot be lost.
    ///
    /// Still a function rather than a type, which is this implementation's honest
    /// limit. A type making a lost release unrepresentable would be the real
    /// prize, and this crate does not have one.
    pub fn check_invariants(&self) -> bool {
        if self.hold_running && self.state != State::Held {
            return false;
        }
        if self.state == State::Held && !self.hold_running {
            return false;
        }
        if self.run != 0 && self.state != State::Free {
            return false;
        }
        if self.state == State::Fault && (self.run != 0 || self.hold_running) {
            return false;
        }
        self.settings.arrive_runs != 0
    }
}
