//! SRTP parameters.

use crate::fbs::{FromFbs, ToFbs};
use mediasoup_sys::fbs::srtp_parameters;
use mediasoup_types::srtp_parameters::*;

impl FromFbs for SrtpParameters {
    type FbsType = srtp_parameters::SrtpParameters;

    fn from_fbs(tuple: &Self::FbsType) -> Self {
        Self {
            crypto_suite: SrtpCryptoSuite::from_fbs(&tuple.crypto_suite),
            key_base64: String::from(tuple.key_base64.as_str()),
        }
    }
}

impl ToFbs for SrtpParameters {
    type FbsType = srtp_parameters::SrtpParameters;

    fn to_fbs(&self) -> Self::FbsType {
        srtp_parameters::SrtpParameters {
            crypto_suite: SrtpCryptoSuite::to_fbs(&self.crypto_suite),
            key_base64: String::from(self.key_base64.as_str()),
        }
    }
}

impl FromFbs for SrtpCryptoSuite {
    type FbsType = srtp_parameters::SrtpCryptoSuite;

    fn from_fbs(crypto_suite: &Self::FbsType) -> Self {
        match crypto_suite {
            srtp_parameters::SrtpCryptoSuite::AeadAes256Gcm => Self::AeadAes256Gcm,
            srtp_parameters::SrtpCryptoSuite::AeadAes128Gcm => Self::AeadAes128Gcm,
            srtp_parameters::SrtpCryptoSuite::AesCm128HmacSha180 => Self::AesCm128HmacSha180,
            srtp_parameters::SrtpCryptoSuite::AesCm128HmacSha132 => Self::AesCm128HmacSha132,
        }
    }
}

impl ToFbs for SrtpCryptoSuite {
    type FbsType = srtp_parameters::SrtpCryptoSuite;

    fn to_fbs(&self) -> Self::FbsType {
        match self {
            Self::AeadAes256Gcm => srtp_parameters::SrtpCryptoSuite::AeadAes256Gcm,
            Self::AeadAes128Gcm => srtp_parameters::SrtpCryptoSuite::AeadAes128Gcm,
            Self::AesCm128HmacSha180 => srtp_parameters::SrtpCryptoSuite::AesCm128HmacSha180,
            Self::AesCm128HmacSha132 => srtp_parameters::SrtpCryptoSuite::AesCm128HmacSha132,
        }
    }
}
