use serde::{Serialize, Serializer};

#[derive(Debug, Copy, Clone)]
pub enum WorkerLogLevel {
    Debug,
    Warn,
    Error,
    None,
}

impl Default for WorkerLogLevel {
    fn default() -> Self {
        Self::Error
    }
}

impl Serialize for WorkerLogLevel {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(self.as_str())
    }
}

impl WorkerLogLevel {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::Debug => "debug",
            Self::Warn => "warn",
            Self::Error => "error",
            Self::None => "none",
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub enum WorkerLogTag {
    Info,
    Ice,
    Dtls,
    Rtp,
    Srtp,
    Rtcp,
    Rtx,
    Bwe,
    Score,
    Simulcast,
    Svc,
    Sctp,
    Message,
}

impl Serialize for WorkerLogTag {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(self.as_str())
    }
}

impl WorkerLogTag {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::Info => "info",
            Self::Ice => "ice",
            Self::Dtls => "dtls",
            Self::Rtp => "rtp",
            Self::Srtp => "srtp",
            Self::Rtcp => "rtcp",
            Self::Rtx => "rtx",
            Self::Bwe => "bwe",
            Self::Score => "score",
            Self::Simulcast => "simulcast",
            Self::Svc => "svc",
            Self::Sctp => "sctp",
            Self::Message => "message",
        }
    }
}
