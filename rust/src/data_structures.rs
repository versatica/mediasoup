use serde::{Deserialize, Serialize, Serializer};
use uuid::Uuid;

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WorkerDumpResponse {
    pub pid: u32,
    pub router_ids: Vec<Uuid>,
}

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

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct WorkerUpdateSettingsData {
    pub log_level: WorkerLogLevel,
    pub log_tags: Vec<WorkerLogTag>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterInternal {
    pub router_id: Uuid,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportInternal {
    pub router_id: Uuid,
    pub transport_id: Uuid,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportListenIp {
    pub ip: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub announced_ip: Option<String>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub struct NumSctpStreams {
    pub os: u16,
    pub mis: u16,
}

impl Default for NumSctpStreams {
    fn default() -> Self {
        Self {
            os: 1024,
            mis: 1024,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreateWebrtcTransportData {
    pub listen_ips: Vec<TransportListenIp>,
    pub enable_udp: bool,
    pub enable_tcp: bool,
    pub prefer_udp: bool,
    pub prefer_tcp: bool,
    pub initial_available_outgoing_bitrate: u32,
    pub enable_sctp: bool,
    pub num_sctp_streams: NumSctpStreams,
    pub max_sctp_message_size: u32,
    pub sctp_send_buffer_size: u32,
    is_data_channel: bool,
}

impl RouterCreateWebrtcTransportData {
    pub fn new(listen_ips: Vec<TransportListenIp>) -> Self {
        Self {
            listen_ips,
            enable_udp: true,
            enable_tcp: false,
            prefer_udp: false,
            prefer_tcp: false,
            initial_available_outgoing_bitrate: 600_000,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262144,
            sctp_send_buffer_size: 262144,
            is_data_channel: true,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreatePlainTransportData {
    pub listen_ip: TransportListenIp,
    pub rtcp_mux: bool,
    pub comedia: bool,
    pub enable_sctp: bool,
    pub num_sctp_streams: NumSctpStreams,
    pub max_sctp_message_size: u32,
    pub sctp_send_buffer_size: u32,
    pub enable_srtp: bool,
    pub srtp_crypto_suite: String,
    is_data_channel: bool,
}

impl RouterCreatePlainTransportData {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self {
            listen_ip,
            rtcp_mux: true,
            comedia: false,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262144,
            sctp_send_buffer_size: 262144,
            enable_srtp: false,
            srtp_crypto_suite: "AES_CM_128_HMAC_SHA1_80".to_string(),
            is_data_channel: false,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreatePipeTransportData {
    pub listen_ip: TransportListenIp,
    pub enable_sctp: bool,
    pub num_sctp_streams: NumSctpStreams,
    pub max_sctp_message_size: u32,
    pub sctp_send_buffer_size: u32,
    pub enable_rtx: bool,
    pub enable_srtp: bool,
    is_data_channel: bool,
}

impl RouterCreatePipeTransportData {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self {
            listen_ip,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 268435456,
            sctp_send_buffer_size: 268435456,
            enable_rtx: false,
            enable_srtp: false,
            is_data_channel: false,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreateDirectTransportData {
    pub direct: bool,
    pub max_message_size: u32,
}

impl RouterCreateDirectTransportData {
    pub fn new() -> Self {
        Self {
            direct: true,
            max_message_size: 262144,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreateAudioLevelObserverInternal {
    pub router_id: Uuid,
    pub rtp_observer_id: Uuid,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreateAudioLevelObserverData {
    pub max_entries: u16,
    pub threshold: i8,
    pub interval: u16,
}

impl RouterCreateAudioLevelObserverData {
    pub fn new() -> Self {
        Self {
            max_entries: 1,
            threshold: -80,
            interval: 1000,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "lowercase", untagged)]
pub enum DtlsRole {
    Auto,
    Client,
    Server,
}

#[derive(Debug, Serialize)]
pub struct DtlsFingerprint {
    pub algorithm: String,
    pub value: String,
}

impl Default for DtlsRole {
    fn default() -> Self {
        Self::Auto
    }
}

#[derive(Debug, Serialize)]
pub struct DtlsParameters {
    pub role: DtlsRole,
    pub fingerprints: Vec<DtlsFingerprint>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportConnectData {
    pub dtls_parameters: DtlsParameters,
}

impl TransportConnectData {
    pub fn new(dtls_parameters: DtlsParameters) -> Self {
        Self { dtls_parameters }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportSetMaxIncomingBitrateData {
    pub bitrate: u64,
}

impl TransportSetMaxIncomingBitrateData {
    pub fn new(bitrate: u64) -> Self {
        Self { bitrate }
    }
}

#[derive(Debug, Serialize)]
#[serde(untagged)]
pub enum RequestMessage {
    WorkerDump,
    WorkerGetResourceUsage,
    WorkerUpdateSettings {
        data: WorkerUpdateSettingsData,
    },
    WorkerCreateRouter {
        internal: RouterInternal,
    },
    RouterClose {
        internal: RouterInternal,
    },
    RouterDump {
        internal: RouterInternal,
    },
    RouterCreateWebrtcTransport {
        internal: TransportInternal,
        data: RouterCreateWebrtcTransportData,
    },
    RouterCreatePlainTransport {
        internal: TransportInternal,
        data: RouterCreatePlainTransportData,
    },
    RouterCreatePipeTransport {
        internal: TransportInternal,
        data: RouterCreatePipeTransportData,
    },
    RouterCreateDirectTransport {
        internal: TransportInternal,
        data: RouterCreateDirectTransportData,
    },
    RouterCreateAudioLevelObserver {
        internal: RouterCreateAudioLevelObserverInternal,
        data: RouterCreateAudioLevelObserverData,
    },
    TransportClose {
        internal: TransportInternal,
    },
    TransportDump {
        internal: TransportInternal,
    },
    TransportGetStats {
        internal: TransportInternal,
    },
    TransportConnect {
        internal: TransportInternal,
        data: TransportConnectData,
    },
    TransportSetMaxIncomingBitrate {
        internal: TransportInternal,
        data: TransportSetMaxIncomingBitrateData,
    },
    // TODO: Detail remaining methods, I got bored for now
    TransportRestartIce {
        internal: TransportInternal,
    },
    TransportProduce,
    TransportConsume,
    TransportProduceData,
    TransportConsumeData,
    TransportEnableTraceEvent,
    ProducerClose,
    ProducerDump,
    ProducerGetStats,
    ProducerPause,
    ProducerResume,
    ProducerEnableTraceEvent,
    ConsumerClose,
    ConsumerDump,
    ConsumerGetStats,
    ConsumerPause,
    ConsumerResume,
    ConsumerSetPreferredLayers,
    ConsumerSetPriority,
    ConsumerRequestKeyFrame,
    ConsumerEnableTraceEvent,
    DataProducerClose,
    DataProducerDump,
    DataProducerGetStats,
    DataConsumerClose,
    DataConsumerDump,
    DataConsumerGetStats,
    DataConsumerGetBufferedAmount,
    DataConsumerSetBufferedAmountLowThreshold,
    RtpObserverClose,
    RtpObserverPause,
    RtpObserverResume,
    RtpObserverAddProducer,
    RtpObserverRemoveProducer,
}

impl RequestMessage {
    pub(crate) fn as_method(&self) -> &'static str {
        match self {
            Self::WorkerDump => "worker.dump",
            Self::WorkerGetResourceUsage => "worker.getResourceUsage",
            Self::WorkerUpdateSettings { .. } => "worker.updateSettings",
            Self::WorkerCreateRouter { .. } => "worker.createRouter",
            Self::RouterClose { .. } => "router.close",
            Self::RouterDump { .. } => "router.dump",
            Self::RouterCreateWebrtcTransport { .. } => "router.createWebRtcTransport",
            Self::RouterCreatePlainTransport { .. } => "router.createPlainTransport",
            Self::RouterCreatePipeTransport { .. } => "router.createPipeTransport",
            Self::RouterCreateDirectTransport { .. } => "router.createDirectTransport",
            Self::RouterCreateAudioLevelObserver { .. } => "router.createAudioLevelObserver",
            Self::TransportClose { .. } => "transport.close",
            Self::TransportDump { .. } => "transport.dump",
            Self::TransportGetStats { .. } => "transport.getStats",
            Self::TransportConnect { .. } => "transport.connect",
            Self::TransportSetMaxIncomingBitrate { .. } => "transport.setMaxIncomingBitrate",
            Self::TransportRestartIce { .. } => "transport.restartIce",
            Self::TransportProduce => "transport.produce",
            Self::TransportConsume => "transport.consume",
            Self::TransportProduceData => "transport.produceData",
            Self::TransportConsumeData => "transport.consumeData",
            Self::TransportEnableTraceEvent => "transport.enableTraceEvent",
            Self::ProducerClose => "producer.close",
            Self::ProducerDump => "producer.dump",
            Self::ProducerGetStats => "producer.getStats",
            Self::ProducerPause => "producer.pause",
            Self::ProducerResume => "producer.resume",
            Self::ProducerEnableTraceEvent => "producer.enableTraceEvent",
            Self::ConsumerClose => "consumer.close",
            Self::ConsumerDump => "consumer.dump",
            Self::ConsumerGetStats => "consumer.getStats",
            Self::ConsumerPause => "consumer.pause",
            Self::ConsumerResume => "consumer.resume",
            Self::ConsumerSetPreferredLayers => "consumer.setPreferredLayers",
            Self::ConsumerSetPriority => "consumer.setPriority",
            Self::ConsumerRequestKeyFrame => "consumer.requestKeyFrame",
            Self::ConsumerEnableTraceEvent => "consumer.enableTraceEvent",
            Self::DataProducerClose => "dataProducer.close",
            Self::DataProducerDump => "dataProducer.dump",
            Self::DataProducerGetStats => "dataProducer.getStats",
            Self::DataConsumerClose => "dataConsumer.close",
            Self::DataConsumerDump => "dataConsumer.dump",
            Self::DataConsumerGetStats => "dataConsumer.getStats",
            Self::DataConsumerGetBufferedAmount => "dataConsumer.getBufferedAmount",
            Self::DataConsumerSetBufferedAmountLowThreshold => {
                "dataConsumer.setBufferedAmountLowThreshold"
            }
            Self::RtpObserverClose => "rtpObserver.close",
            Self::RtpObserverPause => "rtpObserver.pause",
            Self::RtpObserverResume => "rtpObserver.resume",
            Self::RtpObserverAddProducer => "rtpObserver.addProducer",
            Self::RtpObserverRemoveProducer => "rtpObserver.removeProducer",
        }
    }
}
