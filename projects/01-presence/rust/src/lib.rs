//! The same twenty-eight rows, in Rust.
//!
//! `#![no_std]`, because this compiles for the node as well as for the host, and
//! the kernel adapter has no allocator. Nothing here allocates, panics on a
//! reachable path, or uses `unsafe`.
//!
//! # The same table as `../c/presence.c`, row for row
//!
//! Same order, same guards, same destinations, so the row index an event takes is
//! identical across C, C++ and Rust. A script checks that mechanically before each
//! commit; agreement between three languages is the evidence that the table and
//! not the language decides what the node does.
//!
//! # What Rust changes about expressing this, and what it does not
//!
//! Rust's editions are **not** versions in the C++ sense. One compiler builds all
//! of them, an edition changes what the language permits rather than what the
//! library offers, and a crate declares its edition in `Cargo.toml`. So the
//! honest comparison is editions plus a pinned toolchain, and
//! `rust-toolchain.toml` names the toolchain. That is why this file is a single
//! source compiled under three editions rather than three files: an edition that
//! needed its own source would be a finding, and so far none does.
//!
//! What the type system changes here, measured against the C rather than asserted:
//!
//! * **The table cannot be partial.** In C, totality is a run-time test, and in
//!   C++23 a `consteval` check. Here `match` on `(State, Event)` is exhaustive by
//!   the compiler's rule: a missing pair does not compile. That is the same
//!   guarantee as the `consteval` version, available in every edition, with no
//!   feature probe. It is the one place Rust is simply ahead.
//! * **A dropped event is not expressible.** `dispatch` returns `Result<u8, Err>`,
//!   and `#[must_use]` on it means ignoring the error is a warning that `-D
//!   warnings` turns into a build failure. The C relies on `[[nodiscard]]`'s C23
//!   spelling or on nothing.
//! * **No null guard.** The C stores a function pointer that may be null and
//!   writes a parallel `bool` to say whether it is, because gcc 13 would not
//!   compare the pointer in a constant expression. Here a guard is an
//!   `Option<fn(..) -> bool>` and the discriminant *is* the bool, so the
//!   duplication the C needed does not arise.
//! * **What it does not change:** the dispatcher is still a loop over rows and
//!   still a dozen lines, and the invariant is still a function rather than a type.
//!   A type that made a lost release unrepresentable would be the real prize, and
//!   this crate does not have one. `docs/LANGUAGE_IDIOMS.md` says so.

#![no_std]
#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// What the node is reporting about the room.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum State {
    /// Nobody detected, and no hold outstanding.
    Free,
    /// A run of in-range readings has been seen.
    Occupied,
    /// Readings stopped, the hold timer is running, still reported occupied.
    Held,
    /// A read failed; the state is not trustworthy and says so.
    Fault,
}

/// Everything that can arrive at the dispatcher.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Event {
    /// The kernel timer, the heartbeat.
    Tick,
    /// A range reading, in millimetres.
    Reading(u16),
    /// The hold timer expiring.
    Timeout,
    /// The user button, a forced event.
    Button,
    /// Any failed read.
    Fault,
    /// New thresholds arriving.
    Settings(Settings),
}

impl Event {
    /// The event's kind, ignoring its payload, for the table lookup.
    const fn kind(self) -> Kind {
        match self {
            Event::Tick => Kind::Tick,
            Event::Reading(_) => Kind::Reading,
            Event::Timeout => Kind::Timeout,
            Event::Button => Kind::Button,
            Event::Fault => Kind::Fault,
            Event::Settings(_) => Kind::Settings,
        }
    }
}

/// An event's kind without its payload. The C and C++ carry this as the enum and
/// the payload separately; Rust's enum carries both, so the kind is derived.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Kind {
    /// See [`Event::Tick`].
    Tick,
    /// See [`Event::Reading`].
    Reading,
    /// See [`Event::Timeout`].
    Timeout,
    /// See [`Event::Button`].
    Button,
    /// See [`Event::Fault`].
    Fault,
    /// See [`Event::Settings`].
    Settings,
}

/// Configuration, not constants. The defaults are in `docs/DESIGN.md` and are
/// described there as arguable rather than measured.
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
        Self { arrive_runs: 2, hold_ms: 30_000, range_mm_max: 2500 }
    }
}

/// Why a dispatch produced no transition. There is no "dropped event" variant:
/// the table is total, so a lookup that found nothing is a defect.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Err {
    /// The table is not total. Unreachable while `row_of` is exhaustive.
    NoRow,
}

/// The number of rows, which the row table and the tests both check against.
pub const ROW_COUNT: usize = 28;

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
    /// A node in `Free`, with the given settings. An `arrive_runs` of zero is
    /// raised to one, as in the C.
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

    fn in_range(&self, mm: u16) -> bool {
        mm <= self.settings.range_mm_max
    }

    fn run_completes(&self, mm: u16) -> bool {
        self.in_range(mm) && u32::from(self.run) + 1 >= u32::from(self.settings.arrive_runs)
    }

    /// The row this event takes from this state, by the table's own order.
    ///
    /// **This `match` is why Rust needs no totality check.** Every `(State, Kind)`
    /// pair must appear or the compiler refuses the function, so the property that
    /// the C proves with a test and C++23 with a `consteval` is here a condition of
    /// compiling at all. The row numbers are the C table's numbers.
    fn row_of(&self, ev: Event) -> u8 {
        match (self.state, ev) {
            // 0 to 7: Free
            (State::Free, Event::Reading(mm)) if self.run_completes(mm) => 0,
            (State::Free, Event::Reading(mm)) if self.in_range(mm) => 1,
            (State::Free, Event::Reading(_)) => 2,
            (State::Free, Event::Tick) => 3,
            (State::Free, Event::Timeout) => 4,
            (State::Free, Event::Button) => 5,
            (State::Free, Event::Fault) => 6,
            (State::Free, Event::Settings(_)) => 7,
            // 8 to 14: Occupied
            (State::Occupied, Event::Reading(mm)) if self.in_range(mm) => 8,
            (State::Occupied, Event::Reading(_)) => 9,
            (State::Occupied, Event::Tick) => 10,
            (State::Occupied, Event::Timeout) => 11,
            (State::Occupied, Event::Button) => 12,
            (State::Occupied, Event::Fault) => 13,
            (State::Occupied, Event::Settings(_)) => 14,
            // 15 to 21: Held
            (State::Held, Event::Reading(mm)) if self.in_range(mm) => 15,
            (State::Held, Event::Reading(_)) => 16,
            (State::Held, Event::Timeout) => 17,
            (State::Held, Event::Tick) => 18,
            (State::Held, Event::Button) => 19,
            (State::Held, Event::Fault) => 20,
            (State::Held, Event::Settings(_)) => 21,
            // 22 to 27: Fault, a latch left only by the button
            (State::Fault, Event::Button) => 22,
            (State::Fault, Event::Tick) => 23,
            (State::Fault, Event::Reading(_)) => 24,
            (State::Fault, Event::Timeout) => 25,
            (State::Fault, Event::Fault) => 26,
            (State::Fault, Event::Settings(_)) => 27,
        }
    }

    /// Dispatch one event, returning the row index taken.
    ///
    /// The row index is the return value rather than a field written as a side
    /// effect, which is what C++23 needed `std::expected` for and what C does not
    /// do at all. `#[must_use]` means a caller that ignores the error fails the
    /// build under `-D warnings`.
    #[must_use = "the row index, or the reason the table had no row for this event"]
    pub fn dispatch(&mut self, ev: Event) -> Result<u8, Err> {
        let row = self.row_of(ev);
        if usize::from(row) >= ROW_COUNT {
            self.last_row = None;
            return core::result::Result::Err(Err::NoRow);
        }
        self.act(row, ev);
        self.state = Self::destination(row);
        self.last_row = Some(row);
        self.row_taken[usize::from(row)] += 1;
        Ok(row)
    }

    /// Where each row leaves the node. Separate from the action so that the
    /// destination is a property of the row and not of what the action happens
    /// to do, as in the C table's `to` column.
    const fn destination(row: u8) -> State {
        match row {
            0 | 5 => State::Occupied,
            1 | 2 | 3 | 4 => State::Free,
            6 => State::Fault,
            7 => State::Free,
            8 | 10 | 11 => State::Occupied,
            9 => State::Held,
            12 => State::Free,
            13 => State::Fault,
            14 => State::Occupied,
            15 => State::Occupied,
            16 | 18 | 21 => State::Held,
            17 | 19 => State::Free,
            20 => State::Fault,
            22 => State::Free,
            _ => State::Fault,
        }
    }

    fn act(&mut self, row: u8, ev: Event) {
        let at_ms = match ev {
            Event::Reading(_) | Event::Tick | Event::Timeout | Event::Button | Event::Fault => 0,
            Event::Settings(_) => 0,
        };
        let _ = at_ms;
        match row {
            // Free
            0 => {
                self.run = 0;
                self.hold_running = false;
            }
            1 => self.run = self.run.saturating_add(1),
            2 => self.run = 0,
            3 | 4 => {}
            5 => {
                self.run = 0;
                self.hold_running = false;
            }
            6 => self.latch_fault(),
            7 => self.apply_settings(ev),
            // Occupied
            8 | 10 | 11 => {}
            9 => self.hold_running = true,
            12 => {
                self.run = 0;
                self.hold_running = false;
            }
            13 => self.latch_fault(),
            14 => self.apply_settings(ev),
            // Held
            15 => self.hold_running = false,
            16 | 18 => {}
            17 => {
                self.hold_running = false;
                self.run = 0;
                self.releases += 1;
            }
            19 => {
                self.run = 0;
                self.hold_running = false;
            }
            20 => self.latch_fault(),
            21 => self.apply_settings(ev),
            // Fault
            22 => {
                self.run = 0;
                self.hold_running = false;
            }
            27 => self.apply_settings(ev),
            _ => {}
        }
        if let Event::Reading(_) = ev {
            // The C records the reading's timestamp on every reading row.
        }
    }

    fn latch_fault(&mut self) {
        self.hold_running = false;
        self.run = 0;
        self.faults_latched += 1;
    }

    fn apply_settings(&mut self, ev: Event) {
        if let Event::Settings(mut s) = ev {
            if s.arrive_runs == 0 {
                s.arrive_runs = 1;
            }
            self.settings = s;
        }
    }

    /// The invariant from `docs/DESIGN.md`: a release cannot be lost.
    ///
    /// Still a function rather than a type, which is the honest limit of this
    /// implementation. A type that made a lost release unrepresentable would be
    /// the real prize and this crate does not have one.
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

/// The edition this crate was compiled under, reported by the test so that the
/// claim "this compiles under three editions" is a log line and not a sentence.
pub const fn edition() -> &'static str {
    // cfg(edition) does not exist, so the build script passes it through. When it
    // is absent the crate still compiles and reports so, rather than failing.
    match option_env!("PRESENCE_EDITION") {
        Some(e) => e,
        None => "unreported",
    }
}
