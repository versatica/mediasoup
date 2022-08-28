use crate::active_speaker_observer::ActiveSpeakerObserverOptions;
use crate::audio_level_observer::AudioLevelObserverOptions;
use crate::consumer::{
    ConsumerDump, ConsumerId, ConsumerLayers, ConsumerScore, ConsumerStats, ConsumerTraceEventType,
    ConsumerType,
};
use crate::data_consumer::{DataConsumerDump, DataConsumerId, DataConsumerStat, DataConsumerType};
use crate::data_producer::{DataProducerDump, DataProducerId, DataProducerStat, DataProducerType};
use crate::data_structures::{
    DtlsParameters, DtlsRole, DtlsState, IceCandidate, IceParameters, IceRole, IceState, ListenIp,
    SctpState, TransportTuple,
};
use crate::direct_transport::DirectTransportOptions;
use crate::ortc::RtpMapping;
use crate::pipe_transport::PipeTransportOptions;
use crate::plain_transport::PlainTransportOptions;
use crate::producer::{
    ProducerDump, ProducerId, ProducerStat, ProducerTraceEventType, ProducerType,
};
use crate::router::{RouterDump, RouterId};
use crate::rtp_observer::RtpObserverId;
use crate::rtp_parameters::{MediaKind, RtpEncodingParameters, RtpParameters};
use crate::sctp_parameters::{NumSctpStreams, SctpParameters, SctpStreamParameters};
use crate::srtp_parameters::{SrtpCryptoSuite, SrtpParameters};
use crate::transport::{TransportId, TransportTraceEventType};
use crate::webrtc_server::{WebRtcServerDump, WebRtcServerId, WebRtcServerListenInfos};
use crate::webrtc_transport::{TransportListenIps, WebRtcTransportListen, WebRtcTransportOptions};
use crate::worker::{WorkerDump, WorkerUpdateSettings};
use parking_lot::Mutex;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::{Debug, Display};
use std::net::IpAddr;
use std::num::NonZeroU16;

pub(crate) trait Request
where
    Self: Debug + Serialize,
{
    type HandlerId: Display;
    type Response: DeserializeOwned;

    /// Request method to call on worker.
    fn as_method(&self) -> &'static str;

    /// Default response to return in case of soft error, such as channel already closed, entity
    /// doesn't exist on worker during closing.
    fn default_for_soft_error() -> Option<Self::Response> {
        None
    }
}

pub(crate) trait Notification: Debug + Serialize {
    type HandlerId: Display;

    /// Request event to call on worker.
    fn as_event(&self) -> &'static str;
}

macro_rules! request_response {
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident { $( $(#[$request_field_name_attributes: meta])? $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $existing_response_type: ty,
        $default_for_soft_error: expr $(,)?
    ) => {
        #[derive(Debug, Serialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $request_struct_name {
            $(
                $(#[$request_field_name_attributes])*
                pub(crate) $request_field_name: $request_field_type,
            )*
        }

        impl Request for $request_struct_name {
            type HandlerId = $handler_id_type;
            type Response = $existing_response_type;

            fn as_method(&self) -> &'static str {
                $method
            }

            fn default_for_soft_error() -> Option<Self::Response> {
                $default_for_soft_error
            }
        }
    };
    // Call above macro with no default for soft error
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
        $existing_response_type: ty $(,)?
    ) => {
        request_response!(
            $handler_id_type,
            $method,
            $request_struct_name $request_struct_impl,
            $existing_response_type,
            None,
        );
    };
    // Call above macro with unit type as expected response
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
    ) => {
        request_response!(
            $handler_id_type,
            $method,
            $request_struct_name $request_struct_impl,
            (),
            None,
        );
    };
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident { $( $(#[$request_field_name_attributes: meta])? $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $response_struct_name: ident { $( $response_field_name: ident: $response_field_type: ty$(,)? )* },
    ) => {
        #[derive(Debug, Serialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $request_struct_name {
            $(
                $(#[$request_field_name_attributes])*
                pub(crate) $request_field_name: $request_field_type,
            )*
        }

        #[derive(Debug, Deserialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $response_struct_name {
            $( pub(crate) $response_field_name: $response_field_type, )*
        }

        impl Request for $request_struct_name {
            type HandlerId = $handler_id_type;
            type Response = $response_struct_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
}

request_response!(&'static str, "worker.close", WorkerCloseRequest {});

request_response!(
    &'static str,
    "worker.dump",
    WorkerDumpRequest {},
    WorkerDump
);

request_response!(
    &'static str,
    "worker.updateSettings",
    WorkerUpdateSettingsRequest {
        #[serde(flatten)]
        data: WorkerUpdateSettings,
    },
);

request_response!(
    &'static str,
    "worker.createWebRtcServer",
    WorkerCreateWebRtcServerRequest {
        #[serde(rename = "webRtcServerId")]
        webrtc_server_id: WebRtcServerId,
        listen_infos: WebRtcServerListenInfos,
    },
);

request_response!(
    &'static str,
    "worker.closeWebRtcServer",
    WebRtcServerCloseRequest {
        #[serde(rename = "webRtcServerId")]
        webrtc_server_id: WebRtcServerId,
    },
    (),
    Some(()),
);

request_response!(
    WebRtcServerId,
    "webRtcServer.dump",
    WebRtcServerDumpRequest {},
    WebRtcServerDump,
);

request_response!(
    &'static str,
    "worker.createRouter",
    WorkerCreateRouterRequest {
        router_id: RouterId,
    },
);

request_response!(
    &'static str,
    "worker.closeRouter",
    RouterCloseRequest {
        router_id: RouterId,
    },
    (),
    Some(()),
);

request_response!(RouterId, "router.dump", RouterDumpRequest {}, RouterDump);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateDirectTransportData {
    transport_id: TransportId,
    direct: bool,
    max_message_size: usize,
}

impl RouterCreateDirectTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        direct_transport_options: &DirectTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            direct: true,
            max_message_size: direct_transport_options.max_message_size,
        }
    }
}

request_response!(
    RouterId,
    "router.createDirectTransport",
    RouterCreateDirectTransportRequest {
        #[serde(flatten)]
        data: RouterCreateDirectTransportData,
    },
    RouterCreateDirectTransportResponse {},
);

#[derive(Debug, Serialize)]
#[serde(untagged)]
enum RouterCreateWebrtcTransportListen {
    #[serde(rename_all = "camelCase")]
    Individual {
        listen_ips: TransportListenIps,
        #[serde(skip_serializing_if = "Option::is_none")]
        port: Option<u16>,
    },
    Server {
        #[serde(rename = "webRtcServerId")]
        webrtc_server_id: WebRtcServerId,
    },
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateWebrtcTransportRequest {
    transport_id: TransportId,
    #[serde(flatten)]
    listen: RouterCreateWebrtcTransportListen,
    enable_udp: bool,
    enable_tcp: bool,
    prefer_udp: bool,
    prefer_tcp: bool,
    initial_available_outgoing_bitrate: u32,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    is_data_channel: bool,
}

impl RouterCreateWebrtcTransportRequest {
    pub(crate) fn from_options(
        transport_id: TransportId,
        webrtc_transport_options: &WebRtcTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            listen: match &webrtc_transport_options.listen {
                WebRtcTransportListen::Individual { listen_ips, port } => {
                    RouterCreateWebrtcTransportListen::Individual {
                        listen_ips: listen_ips.clone(),
                        port: *port,
                    }
                }
                WebRtcTransportListen::Server { webrtc_server } => {
                    RouterCreateWebrtcTransportListen::Server {
                        webrtc_server_id: webrtc_server.id(),
                    }
                }
            },
            enable_udp: webrtc_transport_options.enable_udp,
            enable_tcp: webrtc_transport_options.enable_tcp,
            prefer_udp: webrtc_transport_options.prefer_udp,
            prefer_tcp: webrtc_transport_options.prefer_tcp,
            initial_available_outgoing_bitrate: webrtc_transport_options
                .initial_available_outgoing_bitrate,
            enable_sctp: webrtc_transport_options.enable_sctp,
            num_sctp_streams: webrtc_transport_options.num_sctp_streams,
            max_sctp_message_size: webrtc_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: webrtc_transport_options.sctp_send_buffer_size,
            is_data_channel: true,
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct WebRtcTransportData {
    pub(crate) ice_role: IceRole,
    pub(crate) ice_parameters: IceParameters,
    pub(crate) ice_candidates: Vec<IceCandidate>,
    pub(crate) ice_state: Mutex<IceState>,
    pub(crate) ice_selected_tuple: Mutex<Option<TransportTuple>>,
    pub(crate) dtls_parameters: Mutex<DtlsParameters>,
    pub(crate) dtls_state: Mutex<DtlsState>,
    pub(crate) dtls_remote_cert: Mutex<Option<String>>,
    pub(crate) sctp_parameters: Option<SctpParameters>,
    pub(crate) sctp_state: Mutex<Option<SctpState>>,
}

impl Request for RouterCreateWebrtcTransportRequest {
    type HandlerId = RouterId;
    type Response = WebRtcTransportData;

    fn as_method(&self) -> &'static str {
        match &self.listen {
            RouterCreateWebrtcTransportListen::Individual { .. } => "router.createWebRtcTransport",
            RouterCreateWebrtcTransportListen::Server { .. } => {
                "router.createWebRtcTransportWithServer"
            }
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreatePlainTransportData {
    transport_id: TransportId,
    listen_ip: ListenIp,
    #[serde(skip_serializing_if = "Option::is_none")]
    port: Option<u16>,
    rtcp_mux: bool,
    comedia: bool,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    enable_srtp: bool,
    srtp_crypto_suite: SrtpCryptoSuite,
    is_data_channel: bool,
}

impl RouterCreatePlainTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        plain_transport_options: &PlainTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            listen_ip: plain_transport_options.listen_ip,
            port: plain_transport_options.port,
            rtcp_mux: plain_transport_options.rtcp_mux,
            comedia: plain_transport_options.comedia,
            enable_sctp: plain_transport_options.enable_sctp,
            num_sctp_streams: plain_transport_options.num_sctp_streams,
            max_sctp_message_size: plain_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: plain_transport_options.sctp_send_buffer_size,
            enable_srtp: plain_transport_options.enable_srtp,
            srtp_crypto_suite: plain_transport_options.srtp_crypto_suite,
            is_data_channel: false,
        }
    }
}

request_response!(
    RouterId,
    "router.createPlainTransport",
    RouterCreatePlainTransportRequest {
        #[serde(flatten)]
        data: RouterCreatePlainTransportData,
    },
    PlainTransportData {
        // The following fields are present, but unused
        // rtcp_mux: bool,
        // comedia: bool,
        tuple: Mutex<TransportTuple>,
        rtcp_tuple: Mutex<Option<TransportTuple>>,
        sctp_parameters: Option<SctpParameters>,
        sctp_state: Mutex<Option<SctpState>>,
        srtp_parameters: Mutex<Option<SrtpParameters>>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreatePipeTransportData {
    transport_id: TransportId,
    listen_ip: ListenIp,
    #[serde(skip_serializing_if = "Option::is_none")]
    port: Option<u16>,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    enable_rtx: bool,
    enable_srtp: bool,
    is_data_channel: bool,
}

impl RouterCreatePipeTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        pipe_transport_options: &PipeTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            listen_ip: pipe_transport_options.listen_ip,
            port: pipe_transport_options.port,
            enable_sctp: pipe_transport_options.enable_sctp,
            num_sctp_streams: pipe_transport_options.num_sctp_streams,
            max_sctp_message_size: pipe_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: pipe_transport_options.sctp_send_buffer_size,
            enable_rtx: pipe_transport_options.enable_rtx,
            enable_srtp: pipe_transport_options.enable_srtp,
            is_data_channel: false,
        }
    }
}

request_response!(
    RouterId,
    "router.createPipeTransport",
    RouterCreatePipeTransportRequest {
        #[serde(flatten)]
        data: RouterCreatePipeTransportData,
    },
    PipeTransportData {
        tuple: Mutex<TransportTuple>,
        sctp_parameters: Option<SctpParameters>,
        sctp_state: Mutex<Option<SctpState>>,
        rtx: bool,
        srtp_parameters: Mutex<Option<SrtpParameters>>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateAudioLevelObserverData {
    rtp_observer_id: RtpObserverId,
    max_entries: NonZeroU16,
    threshold: i8,
    interval: u16,
}

impl RouterCreateAudioLevelObserverData {
    pub(crate) fn from_options(
        rtp_observer_id: RtpObserverId,
        audio_level_observer_options: &AudioLevelObserverOptions,
    ) -> Self {
        Self {
            rtp_observer_id,
            max_entries: audio_level_observer_options.max_entries,
            threshold: audio_level_observer_options.threshold,
            interval: audio_level_observer_options.interval,
        }
    }
}

request_response!(
    RouterId,
    "router.createAudioLevelObserver",
    RouterCreateAudioLevelObserverRequest {
        #[serde(flatten)]
        data: RouterCreateAudioLevelObserverData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateActiveSpeakerObserverData {
    rtp_observer_id: RtpObserverId,
    interval: u16,
}

impl RouterCreateActiveSpeakerObserverData {
    pub(crate) fn from_options(
        rtp_observer_id: RtpObserverId,
        active_speaker_observer_options: &ActiveSpeakerObserverOptions,
    ) -> Self {
        Self {
            rtp_observer_id,
            interval: active_speaker_observer_options.interval,
        }
    }
}

request_response!(
    RouterId,
    "router.createActiveSpeakerObserver",
    RouterCreateActiveSpeakerObserverRequest {
        #[serde(flatten)]
        data: RouterCreateActiveSpeakerObserverData,
    },
);

request_response!(
    RouterId,
    "router.closeTransport",
    TransportCloseRequest {
        transport_id: TransportId,
    },
    (),
    Some(()),
);

request_response!(
    TransportId,
    "transport.dump",
    TransportDumpRequest {},
    Value,
);

request_response!(
    TransportId,
    "transport.getStats",
    TransportGetStatsRequest {},
    Value,
);

request_response!(
    TransportId,
    "transport.connect",
    TransportConnectWebRtcRequest {
        dtls_parameters: DtlsParameters,
    },
    TransportConnectResponseWebRtc {
        dtls_local_role: DtlsRole,
    },
);

request_response!(
    TransportId,
    "transport.connect",
    TransportConnectPipeRequest {
        ip: IpAddr,
        port: u16,
        #[serde(skip_serializing_if = "Option::is_none")]
        srtp_parameters: Option<SrtpParameters>,
    },
    TransportConnectResponsePipe {
        tuple: TransportTuple,
    },
);

request_response!(
    TransportId,
    "transport.connect",
    TransportConnectPlainRequest {
        #[serde(skip_serializing_if = "Option::is_none")]
        ip: Option<IpAddr>,
        #[serde(skip_serializing_if = "Option::is_none")]
        port: Option<u16>,
        #[serde(skip_serializing_if = "Option::is_none")]
        rtcp_port: Option<u16>,
        #[serde(skip_serializing_if = "Option::is_none")]
        srtp_parameters: Option<SrtpParameters>,
    },
    TransportConnectResponsePlain {
        tuple: Option<TransportTuple>,
        rtcp_tuple: Option<TransportTuple>,
        srtp_parameters: Option<SrtpParameters>,
    },
);

request_response!(
    TransportId,
    "transport.setMaxIncomingBitrate",
    TransportSetMaxIncomingBitrateRequest { bitrate: u32 },
);

request_response!(
    TransportId,
    "transport.setMaxOutgoingBitrate",
    TransportSetMaxOutgoingBitrateRequest { bitrate: u32 },
);

request_response!(
    TransportId,
    "transport.restartIce",
    TransportRestartIceRequest {},
    TransportRestartIceResponse {
        ice_parameters: IceParameters,
    },
);

request_response!(
    TransportId,
    "transport.produce",
    TransportProduceRequest {
        producer_id: ProducerId,
        kind: MediaKind,
        rtp_parameters: RtpParameters,
        rtp_mapping: RtpMapping,
        key_frame_request_delay: u32,
        paused: bool,
    },
    TransportProduceResponse {
        r#type: ProducerType,
    },
);

request_response!(
    TransportId,
    "transport.consume",
    TransportConsumeRequest {
        consumer_id: ConsumerId,
        producer_id: ProducerId,
        kind: MediaKind,
        rtp_parameters: RtpParameters,
        r#type: ConsumerType,
        consumable_rtp_encodings: Vec<RtpEncodingParameters>,
        paused: bool,
        preferred_layers: Option<ConsumerLayers>,
        ignore_dtx: bool,
    },
    TransportConsumeResponse {
        paused: bool,
        producer_paused: bool,
        score: ConsumerScore,
        preferred_layers: Option<ConsumerLayers>,
    },
);

request_response!(
    TransportId,
    "transport.produceData",
    TransportProduceDataRequest {
        data_producer_id: DataProducerId,
        r#type: DataProducerType,
        #[serde(skip_serializing_if = "Option::is_none")]
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
    },
    TransportProduceDataResponse {
        r#type: DataProducerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
    },
);

request_response!(
    TransportId,
    "transport.consumeData",
    TransportConsumeDataRequest {
        data_consumer_id: DataConsumerId,
        data_producer_id: DataProducerId,
        r#type: DataConsumerType,
        #[serde(skip_serializing_if = "Option::is_none")]
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
    },
    TransportConsumeDataResponse {
        r#type: DataConsumerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
    },
);

request_response!(
    TransportId,
    "transport.enableTraceEvent",
    TransportEnableTraceEventRequest {
        types: Vec<TransportTraceEventType>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportSendRtcpNotification {}

impl Notification for TransportSendRtcpNotification {
    type HandlerId = TransportId;

    fn as_event(&self) -> &'static str {
        "transport.sendRtcp"
    }
}

request_response!(
    TransportId,
    "transport.closeProducer",
    ProducerCloseRequest {
        producer_id: ProducerId,
    },
    (),
    Some(()),
);

request_response!(
    ProducerId,
    "producer.dump",
    ProducerDumpRequest {},
    ProducerDump
);

request_response!(
    ProducerId,
    "producer.getStats",
    ProducerGetStatsRequest {},
    Vec<ProducerStat>,
);

request_response!(ProducerId, "producer.pause", ProducerPauseRequest {});

request_response!(ProducerId, "producer.resume", ProducerResumeRequest {});

request_response!(
    ProducerId,
    "producer.enableTraceEvent",
    ProducerEnableTraceEventRequest {
        types: Vec<ProducerTraceEventType>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ProducerSendNotification {}

impl Notification for ProducerSendNotification {
    type HandlerId = ProducerId;

    fn as_event(&self) -> &'static str {
        "producer.send"
    }
}

request_response!(
    TransportId,
    "transport.closeConsumer",
    ConsumerCloseRequest {
        consumer_id: ConsumerId,
    },
    (),
    Some(()),
);

request_response!(
    ConsumerId,
    "consumer.dump",
    ConsumerDumpRequest {},
    ConsumerDump,
);

request_response!(
    ConsumerId,
    "consumer.getStats",
    ConsumerGetStatsRequest {},
    ConsumerStats,
);

request_response!(ConsumerId, "consumer.pause", ConsumerPauseRequest {},);

request_response!(ConsumerId, "consumer.resume", ConsumerResumeRequest {},);

request_response!(
    ConsumerId,
    "consumer.setPreferredLayers",
    ConsumerSetPreferredLayersRequest {
        #[serde(flatten)]
        data: ConsumerLayers,
    },
    Option<ConsumerLayers>,
);

request_response!(
    ConsumerId,
    "consumer.setPriority",
    ConsumerSetPriorityRequest { priority: u8 },
    ConsumerSetPriorityResponse { priority: u8 },
);

request_response!(
    ConsumerId,
    "consumer.requestKeyFrame",
    ConsumerRequestKeyFrameRequest {},
);

request_response!(
    ConsumerId,
    "consumer.enableTraceEvent",
    ConsumerEnableTraceEventRequest {
        types: Vec<ConsumerTraceEventType>,
    },
);

request_response!(
    TransportId,
    "transport.closeDataProducer",
    DataProducerCloseRequest {
        data_producer_id: DataProducerId,
    },
    (),
    Some(()),
);

request_response!(
    DataProducerId,
    "dataProducer.dump",
    DataProducerDumpRequest {},
    DataProducerDump,
);

request_response!(
    DataProducerId,
    "dataProducer.getStats",
    DataProducerGetStatsRequest {},
    Vec<DataProducerStat>,
);

#[derive(Debug, Copy, Clone, Serialize)]
#[serde(into = "u32")]
pub(crate) struct DataProducerSendNotification {
    #[serde(flatten)]
    pub(crate) ppid: u32,
}

impl From<DataProducerSendNotification> for u32 {
    fn from(notification: DataProducerSendNotification) -> Self {
        notification.ppid
    }
}

impl Notification for DataProducerSendNotification {
    type HandlerId = DataProducerId;

    fn as_event(&self) -> &'static str {
        "dataProducer.send"
    }
}

request_response!(
    TransportId,
    "transport.closeDataConsumer",
    DataConsumerCloseRequest {
        data_consumer_id: DataConsumerId,
    },
    (),
    Some(()),
);

request_response!(
    DataConsumerId,
    "dataConsumer.dump",
    DataConsumerDumpRequest {},
    DataConsumerDump,
);

request_response!(
    DataConsumerId,
    "dataConsumer.getStats",
    DataConsumerGetStatsRequest {},
    Vec<DataConsumerStat>,
);

request_response!(
    DataConsumerId,
    "dataConsumer.getBufferedAmount",
    DataConsumerGetBufferedAmountRequest {},
    DataConsumerGetBufferedAmountResponse {
        buffered_amount: u32,
    },
);

request_response!(
    DataConsumerId,
    "dataConsumer.setBufferedAmountLowThreshold",
    DataConsumerSetBufferedAmountLowThresholdRequest { threshold: u32 },
);

#[derive(Debug, Copy, Clone, Serialize)]
#[serde(into = "u32")]
pub(crate) struct DataConsumerSendRequest {
    pub(crate) ppid: u32,
}

impl Request for DataConsumerSendRequest {
    type HandlerId = DataConsumerId;
    type Response = ();

    fn as_method(&self) -> &'static str {
        "dataConsumer.send"
    }
}

impl From<DataConsumerSendRequest> for u32 {
    fn from(request: DataConsumerSendRequest) -> Self {
        request.ppid
    }
}

request_response!(
    RouterId,
    "router.closeRtpObserver",
    RtpObserverCloseRequest {
        rtp_observer_id: RtpObserverId,
    },
    (),
    Some(()),
);

request_response!(
    RtpObserverId,
    "rtpObserver.pause",
    RtpObserverPauseRequest {},
);

request_response!(
    RtpObserverId,
    "rtpObserver.resume",
    RtpObserverResumeRequest {},
);

request_response!(
    RtpObserverId,
    "rtpObserver.addProducer",
    RtpObserverAddProducerRequest {
        producer_id: ProducerId,
    },
);

request_response!(
    RtpObserverId,
    "rtpObserver.removeProducer",
    RtpObserverRemoveProducerRequest {
        producer_id: ProducerId,
    },
);
