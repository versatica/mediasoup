use super::*;

#[test]
fn parse_scalability_modes() {
    assert_eq!(
        "L1T3".parse(),
        Ok(ScalabilityMode {
            spatial_layers: 1,
            temporal_layers: 3,
            ksvc: false
        }),
    );

    assert_eq!(
        "L3T2_KEY".parse(),
        Ok(ScalabilityMode {
            spatial_layers: 3,
            temporal_layers: 2,
            ksvc: true
        }),
    );

    assert_eq!(
        "S2T3".parse(),
        Ok(ScalabilityMode {
            spatial_layers: 2,
            temporal_layers: 3,
            ksvc: false
        }),
    );

    assert_eq!(
        "foo".parse::<ScalabilityMode>(),
        Err(ParseScalabilityModeError::InvalidInput),
    );

    assert_eq!(
        "ull".parse::<ScalabilityMode>(),
        Err(ParseScalabilityModeError::InvalidInput),
    );

    assert_eq!(
        "S0T3".parse::<ScalabilityMode>(),
        Err(ParseScalabilityModeError::InvalidInput),
    );

    assert_eq!(
        "S1T0".parse::<ScalabilityMode>(),
        Err(ParseScalabilityModeError::InvalidInput),
    );

    assert_eq!(
        "L20T3".parse(),
        Ok(ScalabilityMode {
            spatial_layers: 20,
            temporal_layers: 3,
            ksvc: false
        }),
    );

    assert_eq!(
        "S200T3".parse::<ScalabilityMode>(),
        Err(ParseScalabilityModeError::InvalidInput),
    );

    assert_eq!(
        "L4T7_KEY_SHIFT".parse(),
        Ok(ScalabilityMode {
            spatial_layers: 4,
            temporal_layers: 7,
            ksvc: true
        }),
    );
}
