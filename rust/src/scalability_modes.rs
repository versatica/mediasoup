//! Scalability mode.

#[cfg(test)]
mod tests;

use once_cell::sync::OnceCell;
use regex::Regex;
use serde::{Deserialize, Serialize};
use std::str::FromStr;
use thiserror::Error;

/// Scalability mode.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct ScalabilityMode {
    /// Number of spatial layers.
    pub spatial_layers: u8,
    /// Number of temporal layers.
    pub temporal_layers: u8,
    /// K-SVC mode.
    pub ksvc: bool,
}

impl Default for ScalabilityMode {
    fn default() -> Self {
        Self {
            spatial_layers: 1,
            temporal_layers: 1,
            ksvc: false,
        }
    }
}

/// Error that caused [`ScalabilityMode`] parsing error.
#[derive(Debug, Error, Eq, PartialEq)]
pub enum ParseScalabilityModeError {
    /// Invalid input string
    #[error("Invalid input string")]
    InvalidInput,
}

impl FromStr for ScalabilityMode {
    type Err = ParseScalabilityModeError;

    fn from_str(scalability_mode: &str) -> Result<Self, Self::Err> {
        static SCALABILITY_MODE_REGEX: OnceCell<Regex> = OnceCell::new();

        SCALABILITY_MODE_REGEX
            .get_or_init(|| Regex::new(r"^[LS]([1-9][0-9]?)T([1-9][0-9]?)(_KEY)?").unwrap())
            .captures(scalability_mode)
            .map(|captures| ScalabilityMode {
                spatial_layers: captures.get(1).unwrap().as_str().parse().unwrap(),
                temporal_layers: captures.get(2).unwrap().as_str().parse().unwrap(),
                ksvc: captures.get(3).is_some(),
            })
            .ok_or(ParseScalabilityModeError::InvalidInput)
    }
}
