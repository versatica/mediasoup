use super::*;

#[test]
fn parse_scalability_modes() {
    let scalability_mode: ScalabilityMode = "L1T3".parse().unwrap();
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(1).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert!(!scalability_mode.ksvc());

    let scalability_mode: ScalabilityMode = "L3T2_KEY".parse().unwrap();
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(2).unwrap()
    );
    assert!(scalability_mode.ksvc());

    let scalability_mode: ScalabilityMode = "S2T3".parse().unwrap();
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(2).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert!(!scalability_mode.ksvc());

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

    let scalability_mode: ScalabilityMode = "L20T3".parse().unwrap();
    assert!(matches!(scalability_mode, ScalabilityMode::Custom { .. }));
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(20).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert!(!scalability_mode.ksvc());

    assert_eq!(
        "S200T3".parse::<ScalabilityMode>(),
        Err(ParseScalabilityModeError::InvalidInput),
    );

    let scalability_mode: ScalabilityMode = "L4T7_KEY_SHIFT".parse().unwrap();
    assert!(matches!(scalability_mode, ScalabilityMode::Custom { .. }));
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(4).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(7).unwrap()
    );
    assert!(scalability_mode.ksvc());
}

#[test]
fn parse_json_scalability_modes() {
    let scalability_mode: ScalabilityMode = serde_json::from_str("\"L1T3\"").unwrap();
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(1).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert!(!scalability_mode.ksvc());

    let scalability_mode: ScalabilityMode = serde_json::from_str("\"L3T2_KEY\"").unwrap();
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(2).unwrap()
    );
    assert!(scalability_mode.ksvc());

    let scalability_mode: ScalabilityMode = serde_json::from_str("\"S2T3\"").unwrap();
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(2).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert!(!scalability_mode.ksvc());

    assert!(serde_json::from_str::<ScalabilityMode>("\"foo\"").is_err());

    assert!(serde_json::from_str::<ScalabilityMode>("\"ull\"").is_err());

    assert!(serde_json::from_str::<ScalabilityMode>("\"S0T3\"").is_err());

    assert!(serde_json::from_str::<ScalabilityMode>("\"S1T0\"").is_err());

    let scalability_mode: ScalabilityMode = "L20T3".parse().unwrap();
    assert!(matches!(scalability_mode, ScalabilityMode::Custom { .. }));
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(20).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(3).unwrap()
    );
    assert!(!scalability_mode.ksvc());

    assert!(serde_json::from_str::<ScalabilityMode>("\"S200T3\"").is_err());

    let scalability_mode: ScalabilityMode = serde_json::from_str("\"L4T7_KEY_SHIFT\"").unwrap();
    assert!(matches!(scalability_mode, ScalabilityMode::Custom { .. }));
    assert_eq!(
        scalability_mode.spatial_layers(),
        NonZeroU8::new(4).unwrap()
    );
    assert_eq!(
        scalability_mode.temporal_layers(),
        NonZeroU8::new(7).unwrap()
    );
    assert!(scalability_mode.ksvc());
}
