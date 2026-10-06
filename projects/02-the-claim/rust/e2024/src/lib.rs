//! Edition 2024. The cascade itself is in `../../claim_core.rs`, included below, and the
//! tests in `../../tests_core.rs`. This file exists only to give the shared source an
//! edition to be compiled under.
//!
//! The `no_std` attribute lives HERE and not in the module, because an inner attribute in a
//! module file is inert for this purpose: P01 learned that by having
//! `#![cfg_attr(not(test), no_std)]` inside its module rejected as `unused_attributes`,
//! which meant the project's central claim about the node was being made in a file
//! incapable of making it.
#![cfg_attr(not(test), no_std)]
#![forbid(unsafe_code)]

#[path = "../../claim_core.rs"]
pub mod claim;

/// Which edition this crate was built under, for the test's report line.
pub const EDITION: &str = "2024";

#[cfg(test)]
#[path = "../../tests_core.rs"]
mod tests;
