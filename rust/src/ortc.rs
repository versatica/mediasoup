use crate::rtp_parameters::{RtpCodecParameters, RtpCodecParametersParametersValue, RtpParameters};
use thiserror::Error;

#[derive(Debug, Error)]
pub enum RtpParametersError {
    #[error("invalid codec apt parameter {0}")]
    InvalidAptParameter(String),
}

/// Validates RtpParameters.
pub fn validate_rtp_parameters(rtp_parameters: &RtpParameters) -> Result<(), RtpParametersError> {
    for codec in rtp_parameters.codecs.iter() {
        validate_rtp_codec_parameters(codec)?;
    }

    Ok(())
}

/// Validates RtpCodecParameters. It may modify given data by adding missing
/// fields with default values.
/// It throws if invalid.
fn validate_rtp_codec_parameters(codec: &RtpCodecParameters) -> Result<(), RtpParametersError> {
    let parameters = match codec {
        RtpCodecParameters::Audio { parameters, .. } => parameters,
        RtpCodecParameters::Video { parameters, .. } => parameters,
    };

    for (key, value) in parameters {
        // Specific parameters validation.
        if key.as_str() == "apt" {
            match value {
                RtpCodecParametersParametersValue::Number(_) => {
                    // Good
                }
                RtpCodecParametersParametersValue::String(string) => {
                    return Err(RtpParametersError::InvalidAptParameter(string.clone()));
                }
            }
        }
    }

    Ok(())
}
