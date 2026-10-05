//! Edition 2021. The table itself is in `../../presence_core.rs`, included
//! below, and the tests in `../../tests_core.rs`. This file exists only to give
//! the shared source an edition to be compiled under.
#![cfg_attr(not(test), no_std)]

#[path = "../../presence_core.rs"]
pub mod presence;

/// Which edition this crate was built under, for the test's report line.
pub const EDITION: &str = "2021";

#[cfg(test)]
#[path = "../../tests_core.rs"]
mod tests;
