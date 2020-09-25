use once_cell::sync::OnceCell;
use regex::Regex;

static SCALABILITY_MODE_REGEX: OnceCell<Regex> = OnceCell::new();

fn get_scalability_mode_regex_regex() -> &'static Regex {
    &SCALABILITY_MODE_REGEX
        .get_or_init(|| Regex::new(r"^[LS]([1-9]\\d?)T([1-9]\\d?)(_KEY)?").unwrap())
}

pub struct ScalabilityMode {
    pub spatial_layers: u8,
    pub temporal_layers: u8,
    pub ksvc: bool,
}

// TODO: No tests for this now, may not work properly
pub fn parse(scalability_mode: Option<&str>) -> ScalabilityMode {
    match scalability_mode
        .map(|scalability_mode| get_scalability_mode_regex_regex().captures(scalability_mode))
        .flatten()
    {
        Some(captures) => ScalabilityMode {
            spatial_layers: captures.get(1).unwrap().as_str().parse().unwrap(),
            temporal_layers: captures.get(2).unwrap().as_str().parse().unwrap(),
            ksvc: captures.get(3).is_some(),
        },
        None => ScalabilityMode {
            spatial_layers: 1,
            temporal_layers: 1,
            ksvc: false,
        },
    }
}
