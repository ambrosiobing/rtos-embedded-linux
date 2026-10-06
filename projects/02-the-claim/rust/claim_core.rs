//! The claim cascade in Rust: one source, three editions, `no_std`, no `unsafe`.
//!
//! The specification is `docs/DESIGN.md`. This file, `../c/claim.c` and `../cpp/claim.hpp`
//! implement the same eight arms in the same order with the same guard names, and the
//! cross-check script compares them rather than a reader having to trust it.
//!
//! WHAT RUST CHANGES FOR THIS SHAPE, and it is not what it changes for P01's table. There
//! the interesting fact was that `match` can be made exhaustive by the compiler. Here the
//! policy is an ordered cascade, and order is exactly what an exhaustive `match` cannot
//! express: a `match` arm's position does not change its meaning, while a cascade arm's
//! position is its meaning. So the cascade stays an ordered array, and what Rust
//! contributes is different:
//!
//!   the enums cannot hold a value outside their variants, so the C's "an unknown service
//!   state blocks" branch has nothing to defend against and does not exist;
//!
//!   `Option<fn>` costs nothing, because a function pointer cannot be null, so the
//!   unguarded arm can be `None` rather than a `fn` that always returns true;
//!
//!   the spool's indices are checked, so the modular arithmetic that the C and C++ get
//!   right by inspection is got right by the compiler instead.
//!
//! The module is `no_std` through the crate roots, which carry the attribute. An attribute
//! written here would be inert: `#![cfg_attr(not(test), no_std)]` in a module does nothing,
//! which P01 learned by having it rejected as `unused_attributes`.

// ---------------------------------------------------------------- the claim axis

/// Chapter 02's seven codes, and `Free`, which is the fall-through rather than one of
/// them: an ordinary release is told from a no-show by the code and not by a separate
/// field that could be dropped.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Code {
    Invisible,
    Rejected,
    Booked,
    Grace,
    NoShow,
    Walkin,
    Brb,
    Free,
}

pub const CODE_COUNT: usize = 8;
pub const PUBLISHED_CODES: usize = 7;
pub const ARM_COUNT: usize = 8;

impl Code {
    pub const ALL: [Code; CODE_COUNT] = [
        Code::Invisible,
        Code::Rejected,
        Code::Booked,
        Code::Grace,
        Code::NoShow,
        Code::Walkin,
        Code::Brb,
        Code::Free,
    ];

    pub fn index(self) -> usize {
        match self {
            Code::Invisible => 0,
            Code::Rejected => 1,
            Code::Booked => 2,
            Code::Grace => 3,
            Code::NoShow => 4,
            Code::Walkin => 5,
            Code::Brb => 6,
            Code::Free => 7,
        }
    }

    pub fn name(self) -> &'static str {
        match self {
            Code::Invisible => "invisible",
            Code::Rejected => "rejected",
            Code::Booked => "booked",
            Code::Grace => "grace",
            Code::NoShow => "no-show",
            Code::Walkin => "walk-in",
            Code::Brb => "be-right-back",
            Code::Free => "free",
        }
    }
}

// -------------------------------------------------------------- the service axis

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Service {
    Ok,
    Degraded,
    NeedsService,
    OutOfService,
    InMaintenance,
    NeedsPart,
    Replaced,
}

pub const SERVICE_STATES: usize = 7;

impl Service {
    pub const ALL: [Service; SERVICE_STATES] = [
        Service::Ok,
        Service::Degraded,
        Service::NeedsService,
        Service::OutOfService,
        Service::InMaintenance,
        Service::NeedsPart,
        Service::Replaced,
    ];

    /// Four of the seven block a claim and three do not. `Degraded` and `NeedsService` are
    /// maintenance facts: something should be looked at and the room still works.
    ///
    /// THE C HAS A BRANCH THIS DOES NOT NEED. `claim_service_blocks` ends with a default
    /// arm treating an unknown state as blocking, because a C enum can hold any value of
    /// its underlying type. A `Service` here cannot be anything but one of the seven, so
    /// the `match` is exhaustive and the defensive arm has nothing to defend against.
    pub fn blocks(self) -> bool {
        match self {
            Service::OutOfService
            | Service::InMaintenance
            | Service::NeedsPart
            | Service::Replaced => true,
            Service::Ok | Service::Degraded | Service::NeedsService => false,
        }
    }

    pub fn index(self) -> usize {
        match self {
            Service::Ok => 0,
            Service::Degraded => 1,
            Service::NeedsService => 2,
            Service::OutOfService => 3,
            Service::InMaintenance => 4,
            Service::NeedsPart => 5,
            Service::Replaced => 6,
        }
    }

    pub fn name(self) -> &'static str {
        match self {
            Service::Ok => "ok",
            Service::Degraded => "degraded",
            Service::NeedsService => "needs-service",
            Service::OutOfService => "out-of-service",
            Service::InMaintenance => "in-maintenance",
            Service::NeedsPart => "needs-part",
            Service::Replaced => "replaced",
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Reason {
    None,
    Sensor,
    Modem,
    Panel,
    Power,
    Fan,
    SetupIncomplete,
    Commanded,
}

pub const REASON_COUNT: usize = 8;
pub const REASONS: usize = 7;

impl Reason {
    pub const ALL: [Reason; REASON_COUNT] = [
        Reason::None,
        Reason::Sensor,
        Reason::Modem,
        Reason::Panel,
        Reason::Power,
        Reason::Fan,
        Reason::SetupIncomplete,
        Reason::Commanded,
    ];

    pub fn index(self) -> usize {
        match self {
            Reason::None => 0,
            Reason::Sensor => 1,
            Reason::Modem => 2,
            Reason::Panel => 3,
            Reason::Power => 4,
            Reason::Fan => 5,
            Reason::SetupIncomplete => 6,
            Reason::Commanded => 7,
        }
    }

    pub fn name(self) -> &'static str {
        match self {
            Reason::None => "none",
            Reason::Sensor => "sensor",
            Reason::Modem => "modem",
            Reason::Panel => "panel",
            Reason::Power => "power",
            Reason::Fan => "fan",
            Reason::SetupIncomplete => "setup-incomplete",
            Reason::Commanded => "commanded",
        }
    }
}

// ----------------------------------------------------------------- the inputs

#[derive(Copy, Clone, Debug)]
pub struct Inputs {
    pub activated: bool,
    pub window_open: bool,
    pub present: bool,
    pub past_grace: bool,
    pub long_press_pending: bool,
    pub service: Service,
    pub service_reason: Reason,
}

impl Default for Inputs {
    fn default() -> Self {
        Inputs {
            activated: false,
            window_open: false,
            present: false,
            past_grace: false,
            long_press_pending: false,
            service: Service::Ok,
            service_reason: Reason::None,
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct Decision {
    pub code: Code,
    pub arm: u8,
}

// ------------------------------------------------- the arms, each one named

// The names are about the situation and not the outcome: `booked_and_waiting` says what is
// true of the room, not that the answer is `Grace`. An arm whose guard is named after its
// own code cannot be checked against anything.

fn not_activated(input: &Inputs) -> bool {
    !input.activated
}

fn service_blocks(input: &Inputs) -> bool {
    input.service.blocks()
}

fn booked_and_present(input: &Inputs) -> bool {
    input.window_open && input.present
}

fn booked_and_waiting(input: &Inputs) -> bool {
    input.window_open && !input.present && !input.past_grace
}

fn booked_and_nobody_came(input: &Inputs) -> bool {
    input.window_open && !input.present && input.past_grace
}

fn unbooked_and_present(input: &Inputs) -> bool {
    !input.window_open && input.present
}

fn press_pending(input: &Inputs) -> bool {
    input.long_press_pending
}

/// A guard, or no guard at all for the fall-through.
///
/// `Option<fn>` is the whole of the saving Rust offers here: a function pointer cannot be
/// null, so the `None` case uses the niche and the option costs nothing beside a bare `fn`.
/// The C++ writes an `always` predicate returning true and a separate `guarded` bool; this
/// needs neither, and the test asserts the sizes to show it.
pub type Guard = fn(&Inputs) -> bool;

pub struct Arm {
    pub guard: Option<Guard>,
    pub code: Code,
}

/// THE ORDER IS THE SPECIFICATION. `docs/DESIGN.md` carries a table of the defect each
/// permutation would cause, and every row of it is a failure somebody would ship.
///
/// One arm per line on purpose, so that this reads beside `claim.c`'s chain and
/// `claim.hpp`'s array as the same eight arms rather than as three shapes.
#[rustfmt::skip]
pub static CASCADE: [Arm; ARM_COUNT] = [
    Arm { guard: Some(not_activated),          code: Code::Invisible },
    Arm { guard: Some(service_blocks),         code: Code::Rejected },
    Arm { guard: Some(booked_and_present),     code: Code::Booked },
    Arm { guard: Some(booked_and_waiting),     code: Code::Grace },
    Arm { guard: Some(booked_and_nobody_came), code: Code::NoShow },
    Arm { guard: Some(unbooked_and_present),   code: Code::Walkin },
    Arm { guard: Some(press_pending),          code: Code::Brb },
    Arm { guard: None,                         code: Code::Free },
];

// --------------------------------------------------------------- the cascade

pub fn decide(input: &Inputs) -> Decision {
    for (i, arm) in CASCADE.iter().enumerate() {
        let fires = match arm.guard {
            Some(guard) => guard(input),
            None => true,
        };
        if fires {
            return Decision {
                code: arm.code,
                arm: (i + 1) as u8,
            };
        }
    }
    // Unreachable while the last arm carries no guard, which `the_only_unguarded_arm_is_last`
    // checks. Returning the fall-through rather than panicking keeps this `no_std` and
    // keeps a release build free of a panic path it cannot reach.
    Decision {
        code: Code::Free,
        arm: ARM_COUNT as u8,
    }
}

/// An event leaves the unit when the code changes, and `Grace` never leaves at all. The
/// calendar already knows about the booking because it sent it; what the unit knows and the
/// calendar cannot is whether anybody came, and during grace it has observed nothing on
/// that question.
pub fn emits(prev: Code, now: Code) -> bool {
    if now == Code::Grace {
        return false;
    }
    now != prev
}

// --------------------------------------------------------------- the settings

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct Settings {
    pub grace_s: u32,
    pub brb_hold_s: u32,
    pub walkin_len_s: u32,
    pub spool_bytes: u32,
    pub stale_after_s: u32,
    pub activated: bool,
}

impl Default for Settings {
    fn default() -> Self {
        Settings {
            grace_s: 300,
            brb_hold_s: 900,
            walkin_len_s: 1800,
            spool_bytes: 4096,
            stale_after_s: 600,
            // Off, and that is the point: the failure mode of a half-finished installation
            // is a room nobody can book rather than a room that swallows bookings.
            activated: false,
        }
    }
}

/// What the C++23 header does with `std::expected` and the C does not do at all. A `Result`
/// has been in the language since before any of the three editions, so this is the one
/// place where Rust's baseline is the later C++'s addition.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum SettingsError {
    GraceIsZero,
    SpoolHoldsNoEvents,
    WalkinShorterThanGrace,
}

impl SettingsError {
    pub fn name(self) -> &'static str {
        match self {
            SettingsError::GraceIsZero => "a grace period of zero releases every booking at once",
            SettingsError::SpoolHoldsNoEvents => {
                "a spool bound below one event discards everything in silence"
            }
            SettingsError::WalkinShorterThanGrace => {
                "a walk-in shorter than the grace period frees a room in use"
            }
        }
    }
}

pub fn checked_settings(s: Settings) -> Result<Settings, SettingsError> {
    if s.grace_s == 0 {
        return Err(SettingsError::GraceIsZero);
    }
    if (s.spool_bytes as usize) < EVENT_BYTES {
        return Err(SettingsError::SpoolHoldsNoEvents);
    }
    if s.walkin_len_s < s.grace_s {
        return Err(SettingsError::WalkinShorterThanGrace);
    }
    Ok(s)
}

// ------------------------------------------------------------------ the spool

#[derive(Copy, Clone, Default, PartialEq, Eq, Debug)]
pub struct Event {
    pub at_s: u32,
    pub code: u8,
    pub arm: u8,
    pub service: u8,
    pub service_reason: u8,
}

/// Rust may reorder fields, so this is the size the layout happens to produce rather than
/// one the declaration order guarantees. The C and C++ pin theirs at 8 bytes with a static
/// assertion and may not reorder; the test prints all three so the difference is on the
/// record rather than assumed away.
pub const EVENT_BYTES: usize = core::mem::size_of::<Event>();

pub const SPOOL_BYTES_MAX: u32 = 4096;
pub const SPOOL_SLOTS: usize = 512;

pub struct Spool {
    slot: [Event; SPOOL_SLOTS],
    cap: usize,
    head: usize,
    count: usize,
    discarded: u32,
    offered: u32,
}

impl Default for Spool {
    fn default() -> Self {
        let mut sp = Spool {
            slot: [Event::default(); SPOOL_SLOTS],
            cap: SPOOL_SLOTS,
            head: 0,
            count: 0,
            discarded: 0,
            offered: 0,
        };
        sp.init(SPOOL_BYTES_MAX);
        sp
    }
}

impl Spool {
    /// The bound is in BYTES, because bytes are what run out.
    pub fn init(&mut self, bytes: u32) {
        self.head = 0;
        self.count = 0;
        self.discarded = 0;
        self.offered = 0;

        let mut want = bytes as usize / EVENT_BYTES;
        if want > SPOOL_SLOTS {
            want = SPOOL_SLOTS;
        }
        if want == 0 {
            want = 1;
        }
        self.cap = want;
    }

    /// Always accepts, discarding the oldest when full, because the newest event is the one
    /// describing the room now. Returns whether a discard happened.
    pub fn push(&mut self, ev: Event) -> bool {
        let mut discarded = false;

        self.offered += 1;
        if self.count == self.cap {
            self.head = (self.head + 1) % self.cap;
            self.count -= 1;
            self.discarded += 1;
            discarded = true;
        }
        let at = (self.head + self.count) % self.cap;
        self.slot[at] = ev;
        self.count += 1;
        discarded
    }

    pub fn capacity(&self) -> usize {
        self.cap
    }

    pub fn count(&self) -> usize {
        self.count
    }

    pub fn discarded(&self) -> u32 {
        self.discarded
    }

    pub fn offered(&self) -> u32 {
        self.offered
    }

    /// Oldest first, so `at(0)` is the oldest retained.
    pub fn at(&self, i: usize) -> Event {
        self.slot[(self.head + i) % self.cap]
    }

    pub fn drain(&mut self) {
        self.head = 0;
        self.count = 0;
        self.discarded = 0;
        self.offered = 0;
    }
}

// ----------------------------------------------------------------- the context

pub struct Machine {
    pub settings: Settings,
    last_in: Inputs,
    last: Decision,
    published: Code,
    spool: Spool,
    emitted: u32,
    arm_taken: [u32; ARM_COUNT],
    code_seen: [u32; CODE_COUNT],
    service_seen: [u32; SERVICE_STATES],
    reason_seen: [u32; REASON_COUNT],
}

impl Default for Machine {
    fn default() -> Self {
        Machine {
            settings: Settings::default(),
            last_in: Inputs::default(),
            // There is no decision yet. `Invisible` is the honest initial value because the
            // unit is not commissioned, and arm 0 says no arm has fired.
            last: Decision {
                code: Code::Invisible,
                arm: 0,
            },
            published: Code::Invisible,
            spool: Spool::default(),
            emitted: 0,
            arm_taken: [0; ARM_COUNT],
            code_seen: [0; CODE_COUNT],
            service_seen: [0; SERVICE_STATES],
            reason_seen: [0; REASON_COUNT],
        }
    }
}

impl Machine {
    pub fn step(&mut self, input: &Inputs, at_s: u32) -> Decision {
        let mut eff = *input;
        // Activation is a setting, not a per-reading input, and the settings win because
        // that is where commissioning is recorded.
        eff.activated = self.settings.activated;

        let d = decide(&eff);

        self.arm_taken[d.arm as usize - 1] += 1;
        self.code_seen[d.code.index()] += 1;
        self.service_seen[eff.service.index()] += 1;
        self.reason_seen[eff.service_reason.index()] += 1;

        if emits(self.last.code, d.code) {
            let ev = Event {
                at_s,
                code: d.code.index() as u8,
                arm: d.arm,
                service: eff.service.index() as u8,
                service_reason: eff.service_reason.index() as u8,
            };
            self.spool.push(ev);
            self.emitted += 1;
            self.published = d.code;
        }

        self.last_in = eff;
        self.last = d;
        d
    }

    /// The six invariants of `docs/DESIGN.md`. The test calls this after every single case
    /// rather than at the end, because a broken invariant can be transient and still wrong.
    pub fn check_invariants(&self) -> bool {
        if !self.settings.activated && self.last.arm != 0 && self.last.code != Code::Invisible {
            return false;
        }
        if self.last.arm != 0
            && self.settings.activated
            && self.last_in.service.blocks()
            && self.last.code != Code::Rejected
        {
            return false;
        }
        for i in 0..self.spool.count() {
            if self.spool.at(i).code == Code::Grace.index() as u8 {
                return false;
            }
        }
        if self.spool.count() > self.spool.capacity() || self.spool.capacity() > SPOOL_SLOTS {
            return false;
        }
        if self.spool.count() as u32 + self.spool.discarded() != self.spool.offered() {
            return false;
        }
        if self.emitted != self.spool.offered() {
            return false;
        }
        true
    }

    pub fn last(&self) -> Decision {
        self.last
    }

    pub fn published(&self) -> Code {
        self.published
    }

    pub fn emitted(&self) -> u32 {
        self.emitted
    }

    pub fn spool(&self) -> &Spool {
        &self.spool
    }

    pub fn spool_mut(&mut self) -> &mut Spool {
        &mut self.spool
    }

    pub fn arm_taken(&self, i: usize) -> u32 {
        self.arm_taken[i]
    }

    pub fn code_seen(&self, i: usize) -> u32 {
        self.code_seen[i]
    }

    pub fn service_seen(&self, i: usize) -> u32 {
        self.service_seen[i]
    }

    pub fn reason_seen(&self, i: usize) -> u32 {
        self.reason_seen[i]
    }
}
