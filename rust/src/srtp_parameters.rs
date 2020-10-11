use serde::{Deserialize, Serialize};

/// SRTP parameters.
#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
pub struct SrtpParameters {
    /// Encryption and authentication transforms to be used.
    crypto_suite: SrtpCryptoSuite,
    /// SRTP keying material (master key and salt) in Base64.
    key_base64: String,
}

/// SRTP crypto suite.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
pub enum SrtpCryptoSuite {
    #[serde(rename = "AES_CM_128_HMAC_SHA1_80")]
    AesCm128HmacSha180,
    #[serde(rename = "AES_CM_128_HMAC_SHA1_32")]
    AesCm128HmacSha132,
}

impl Default for SrtpCryptoSuite {
    fn default() -> Self {
        Self::AesCm128HmacSha180
    }
}
