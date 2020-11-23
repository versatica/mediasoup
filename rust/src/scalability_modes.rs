use once_cell::sync::OnceCell;
use regex::Regex;
use serde::{Deserialize, Serialize};

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct ScalabilityMode {
    pub spatial_layers: u8,
    pub temporal_layers: u8,
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

pub fn parse(scalability_mode: &str) -> ScalabilityMode {
    static SCALABILITY_MODE_REGEX: OnceCell<Regex> = OnceCell::new();

    SCALABILITY_MODE_REGEX
        .get_or_init(|| Regex::new(r"^[LS]([1-9][0-9]?)T([1-9][0-9]?)(_KEY)?").unwrap())
        .captures(scalability_mode)
        .map(|captures| ScalabilityMode {
            spatial_layers: captures.get(1).unwrap().as_str().parse().unwrap(),
            temporal_layers: captures.get(2).unwrap().as_str().parse().unwrap(),
            ksvc: captures.get(3).is_some(),
        })
        .unwrap_or_default()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_scalability_modes() {
        assert_eq!(
            parse("L1T3"),
            ScalabilityMode {
                spatial_layers: 1,
                temporal_layers: 3,
                ksvc: false
            }
        );

        assert_eq!(
            parse("L3T2_KEY"),
            ScalabilityMode {
                spatial_layers: 3,
                temporal_layers: 2,
                ksvc: true
            }
        );

        assert_eq!(
            parse("S2T3"),
            ScalabilityMode {
                spatial_layers: 2,
                temporal_layers: 3,
                ksvc: false
            }
        );

        assert_eq!(
            parse("foo"),
            ScalabilityMode {
                spatial_layers: 1,
                temporal_layers: 1,
                ksvc: false
            }
        );

        assert_eq!(
            parse("ull"),
            ScalabilityMode {
                spatial_layers: 1,
                temporal_layers: 1,
                ksvc: false
            }
        );

        assert_eq!(
            parse("S0T3"),
            ScalabilityMode {
                spatial_layers: 1,
                temporal_layers: 1,
                ksvc: false
            }
        );

        assert_eq!(
            parse("S1T0"),
            ScalabilityMode {
                spatial_layers: 1,
                temporal_layers: 1,
                ksvc: false
            }
        );

        assert_eq!(
            parse("L20T3"),
            ScalabilityMode {
                spatial_layers: 20,
                temporal_layers: 3,
                ksvc: false
            }
        );

        assert_eq!(
            parse("S200T3"),
            ScalabilityMode {
                spatial_layers: 1,
                temporal_layers: 1,
                ksvc: false
            }
        );

        assert_eq!(
            parse("L4T7_KEY_SHIFT"),
            ScalabilityMode {
                spatial_layers: 4,
                temporal_layers: 7,
                ksvc: true
            }
        );
    }
}
