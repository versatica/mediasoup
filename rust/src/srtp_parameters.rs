//! Collection of SRTP-related data structures that are used to specify SRTP encryption/decryption
//! parameters.

use crate::fbs::transport;
use serde::{Deserialize, Serialize};
use std::str::FromStr;

/// SRTP parameters.
#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct SrtpParameters {
    /// Encryption and authentication transforms to be used.
    pub crypto_suite: SrtpCryptoSuite,
    /// SRTP keying material (master key and salt) in Base64.
    pub key_base64: String,
}

impl SrtpParameters {
    pub(crate) fn from_fbs(tuple: &transport::SrtpParameters) -> Self {
        Self {
            crypto_suite: tuple.crypto_suite.parse().unwrap(),
            key_base64: String::from(tuple.key_base64.as_str()),
        }
    }

    pub(crate) fn to_fbs(&self) -> transport::SrtpParameters {
        transport::SrtpParameters {
            crypto_suite: self.crypto_suite.to_string(),
            key_base64: String::from(self.key_base64.as_str()),
        }
    }
}

/// SRTP crypto suite.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
pub enum SrtpCryptoSuite {
    /// AEAD_AES_256_GCM
    #[serde(rename = "AEAD_AES_256_GCM")]
    AeadAes256Gcm,
    /// AEAD_AES_128_GCM
    #[serde(rename = "AEAD_AES_128_GCM")]
    AeadAes128Gcm,
    /// AES_CM_128_HMAC_SHA1_80
    #[serde(rename = "AES_CM_128_HMAC_SHA1_80")]
    AesCm128HmacSha180,
    /// AES_CM_128_HMAC_SHA1_32
    #[serde(rename = "AES_CM_128_HMAC_SHA1_32")]
    AesCm128HmacSha132,
}

impl Default for SrtpCryptoSuite {
    fn default() -> Self {
        Self::AesCm128HmacSha180
    }
}

// TODO: Remove once SrtpCryptoSuite is defined in fbs.
impl ToString for SrtpCryptoSuite {
    fn to_string(&self) -> String {
        match self {
            Self::AeadAes256Gcm => String::from("AEAD_AES_256_GCM"),
            Self::AeadAes128Gcm => String::from("AEAD_AES_128_GCM"),
            Self::AesCm128HmacSha180 => String::from("AES_CM_128_HMAC_SHA1_80"),
            Self::AesCm128HmacSha132 => String::from("AES_CM_128_HMAC_SHA1_32"),
        }
    }
}

/// Error that caused [`SrtpCryptoSuite`] parsing error.
#[derive(Debug, Eq, PartialEq)]
pub struct ParseCryptoSuiteError;

// TODO: Remove once SrtpCryptoSuite is defined in fbs.
impl FromStr for SrtpCryptoSuite {
    type Err = ParseCryptoSuiteError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "AEAD_AES_256_GCM" => Ok(Self::AeadAes256Gcm),
            "AEAD_AES_128_GCM" => Ok(Self::AeadAes128Gcm),
            "AES_CM_128_HMAC_SHA1_80" => Ok(Self::AesCm128HmacSha180),
            "AES_CM_128_HMAC_SHA1_32" => Ok(Self::AesCm128HmacSha132),
            _ => Err(ParseCryptoSuiteError),
        }
    }
}
