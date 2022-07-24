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
use std::fmt::Debug;
use std::net::IpAddr;
use std::num::NonZeroU16;

pub(crate) trait Request: Debug + Serialize {
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
    /// Request event to call on worker.
    fn as_event(&self) -> &'static str;
}

macro_rules! request_response {
    (
        $method: literal,
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $existing_response_type: ty,
        $default_for_soft_error: expr $(,)?
    ) => {
        #[derive(Debug, Serialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $request_struct_name {
            $( pub(crate) $request_field_name: $request_field_type, )*
        }

        impl Request for $request_struct_name {
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
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
        $existing_response_type: ty $(,)?
    ) => {
        request_response!(
            $method,
            $request_struct_name $request_struct_impl,
            $existing_response_type,
            None,
        );
    };
    // Call above macro with unit type as expected response
    (
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
    ) => {
        request_response!(
            $method,
            $request_struct_name $request_struct_impl,
            (),
            None,
        );
    };
    (
        $method: literal,
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $response_struct_name: ident { $( $response_field_name: ident: $response_field_type: ty$(,)? )* },
    ) => {
        #[derive(Debug, Serialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $request_struct_name {
            $( pub(crate) $request_field_name: $request_field_type, )*
        }

        #[derive(Debug, Deserialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $response_struct_name {
            $( pub(crate) $response_field_name: $response_field_type, )*
        }

        impl Request for $request_struct_name {
            type Response = $response_struct_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
}

request_response!("worker.close", WorkerCloseRequest {});

request_response!("worker.dump", WorkerDumpRequest {}, WorkerDump);

request_response!(
    "worker.updateSettings",
    WorkerUpdateSettingsRequest {
        data: WorkerUpdateSettings,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct WorkerCreateWebRtcServerData {
    #[serde(rename = "webRtcServerId")]
    pub(crate) webrtc_server_id: WebRtcServerId,
    pub(crate) listen_infos: WebRtcServerListenInfos,
}

request_response!(
    "worker.createWebRtcServer",
    WorkerCreateWebRtcServerRequest {
        data: WorkerCreateWebRtcServerData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct WebRtcServerCloseRequestData {
    #[serde(rename = "webRtcServerId")]
    pub(crate) webrtc_server_id: WebRtcServerId,
}

request_response!(
    "worker.closeWebRtcServer",
    WebRtcServerCloseRequest {
        data: WebRtcServerCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "webRtcServer.dump",
    WebRtcServerDumpRequest {
        handler_id: WebRtcServerId,
    },
    WebRtcServerDump,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct WorkerCreateRouterRequestData {
    pub(crate) router_id: RouterId,
}

request_response!(
    "worker.createRouter",
    WorkerCreateRouterRequest {
        data: WorkerCreateRouterRequestData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCloseRequestData {
    pub(crate) router_id: RouterId,
}

request_response!(
    "worker.closeRouter",
    RouterCloseRequest {
        data: RouterCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "router.dump",
    RouterDumpRequest {
        handler_id: RouterId,
    },
    RouterDump,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateDirectTransportData {
    pub(crate) transport_id: TransportId,
    pub(crate) direct: bool,
    pub(crate) max_message_size: usize,
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
    "router.createDirectTransport",
    RouterCreateDirectTransportRequest {
        handler_id: RouterId,
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
pub(crate) struct RouterCreateWebrtcTransportData {
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

impl RouterCreateWebrtcTransportData {
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

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateWebrtcTransportRequest {
    pub(crate) handler_id: RouterId,
    pub(crate) data: RouterCreateWebrtcTransportData,
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
    type Response = WebRtcTransportData;

    fn as_method(&self) -> &'static str {
        match &self.data.listen {
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
    "router.createPlainTransport",
    RouterCreatePlainTransportRequest {
        handler_id: RouterId,
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
    "router.createPipeTransport",
    RouterCreatePipeTransportRequest {
        handler_id: RouterId,
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
    pub(crate) rtp_observer_id: RtpObserverId,
    pub(crate) max_entries: NonZeroU16,
    pub(crate) threshold: i8,
    pub(crate) interval: u16,
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
    "router.createAudioLevelObserver",
    RouterCreateAudioLevelObserverRequest {
        handler_id: RouterId,
        data: RouterCreateAudioLevelObserverData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateActiveSpeakerObserverData {
    pub(crate) rtp_observer_id: RtpObserverId,
    pub(crate) interval: u16,
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
    "router.createActiveSpeakerObserver",
    RouterCreateActiveSpeakerObserverRequest {
        handler_id: RouterId,
        data: RouterCreateActiveSpeakerObserverData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportCloseRequestData {
    pub(crate) transport_id: TransportId,
}

request_response!(
    "router.closeTransport",
    TransportCloseRequest {
        handler_id: RouterId,
        data: TransportCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "transport.dump",
    TransportDumpRequest {
        handler_id: TransportId,
    },
    Value,
);

request_response!(
    "transport.getStats",
    TransportGetStatsRequest {
        handler_id: TransportId,
    },
    Value,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConnectRequestWebRtcData {
    pub(crate) dtls_parameters: DtlsParameters,
}

request_response!(
    "transport.connect",
    TransportConnectWebRtcRequest {
        handler_id: TransportId,
        data: TransportConnectRequestWebRtcData,
    },
    TransportConnectResponseWebRtc {
        dtls_local_role: DtlsRole,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConnectRequestPipeData {
    pub(crate) ip: IpAddr,
    pub(crate) port: u16,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) srtp_parameters: Option<SrtpParameters>,
}

request_response!(
    "transport.connect",
    TransportConnectPipeRequest {
        handler_id: TransportId,
        data: TransportConnectRequestPipeData,
    },
    TransportConnectResponsePipe {
        tuple: TransportTuple,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConnectRequestPlainData {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) ip: Option<IpAddr>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) port: Option<u16>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) rtcp_port: Option<u16>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) srtp_parameters: Option<SrtpParameters>,
}

request_response!(
    "transport.connect",
    TransportConnectPlainRequest {
        handler_id: TransportId,
        data: TransportConnectRequestPlainData,
    },
    TransportConnectResponsePlain {
        tuple: Option<TransportTuple>,
        rtcp_tuple: Option<TransportTuple>,
        srtp_parameters: Option<SrtpParameters>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportSetMaxIncomingBitrateData {
    pub(crate) bitrate: u32,
}

request_response!(
    "transport.setMaxIncomingBitrate",
    TransportSetMaxIncomingBitrateRequest {
        handler_id: TransportId,
        data: TransportSetMaxIncomingBitrateData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportSetMaxOutgoingBitrateData {
    pub(crate) bitrate: u32,
}

request_response!(
    "transport.setMaxOutgoingBitrate",
    TransportSetMaxOutgoingBitrateRequest {
        handler_id: TransportId,
        data: TransportSetMaxOutgoingBitrateData,
    },
);

request_response!(
    "transport.restartIce",
    TransportRestartIceRequest {
        handler_id: TransportId,
    },
    TransportRestartIceResponse {
        ice_parameters: IceParameters,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportProduceData {
    pub(crate) producer_id: ProducerId,
    pub(crate) kind: MediaKind,
    pub(crate) rtp_parameters: RtpParameters,
    pub(crate) rtp_mapping: RtpMapping,
    pub(crate) key_frame_request_delay: u32,
    pub(crate) paused: bool,
}

request_response!(
    "transport.produce",
    TransportProduceRequest {
        handler_id: TransportId,
        data: TransportProduceData,
    },
    TransportProduceResponse {
        r#type: ProducerType,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConsumeData {
    pub(crate) consumer_id: ConsumerId,
    pub(crate) producer_id: ProducerId,
    pub(crate) kind: MediaKind,
    pub(crate) rtp_parameters: RtpParameters,
    pub(crate) r#type: ConsumerType,
    pub(crate) consumable_rtp_encodings: Vec<RtpEncodingParameters>,
    pub(crate) paused: bool,
    pub(crate) preferred_layers: Option<ConsumerLayers>,
    pub(crate) ignore_dtx: bool,
}

request_response!(
    "transport.consume",
    TransportConsumeRequest {
        handler_id: TransportId,
        data: TransportConsumeData,
    },
    TransportConsumeResponse {
        paused: bool,
        producer_paused: bool,
        score: ConsumerScore,
        preferred_layers: Option<ConsumerLayers>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportProduceDataData {
    pub(crate) data_producer_id: DataProducerId,
    pub(crate) r#type: DataProducerType,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) sctp_stream_parameters: Option<SctpStreamParameters>,
    pub(crate) label: String,
    pub(crate) protocol: String,
}

request_response!(
    "transport.produceData",
    TransportProduceDataRequest {
        handler_id: TransportId,
        data: TransportProduceDataData,
    },
    TransportProduceDataResponse {
        r#type: DataProducerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConsumeDataData {
    pub(crate) data_consumer_id: DataConsumerId,
    pub(crate) data_producer_id: DataProducerId,
    pub(crate) r#type: DataConsumerType,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) sctp_stream_parameters: Option<SctpStreamParameters>,
    pub(crate) label: String,
    pub(crate) protocol: String,
}

request_response!(
    "transport.consumeData",
    TransportConsumeDataRequest {
        handler_id: TransportId,
        data: TransportConsumeDataData,
    },
    TransportConsumeDataResponse {
        r#type: DataConsumerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportEnableTraceEventData {
    pub(crate) types: Vec<TransportTraceEventType>,
}

request_response!(
    "transport.enableTraceEvent",
    TransportEnableTraceEventRequest {
        handler_id: TransportId,
        data: TransportEnableTraceEventData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportSendRtcpNotification {
    pub(crate) handler_id: TransportId,
}

impl Notification for TransportSendRtcpNotification {
    fn as_event(&self) -> &'static str {
        "transport.sendRtcp"
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ProducerCloseRequestData {
    pub(crate) producer_id: ProducerId,
}

request_response!(
    "transport.closeProducer",
    ProducerCloseRequest {
        handler_id: TransportId,
        data: ProducerCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "producer.dump",
    ProducerDumpRequest {
        handler_id: ProducerId,
    },
    ProducerDump
);

request_response!(
    "producer.getStats",
    ProducerGetStatsRequest {
        handler_id: ProducerId,
    },
    Vec<ProducerStat>,
);

request_response!(
    "producer.pause",
    ProducerPauseRequest {
        handler_id: ProducerId,
    },
);

request_response!(
    "producer.resume",
    ProducerResumeRequest {
        handler_id: ProducerId,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ProducerEnableTraceEventData {
    pub(crate) types: Vec<ProducerTraceEventType>,
}

request_response!(
    "producer.enableTraceEvent",
    ProducerEnableTraceEventRequest {
        handler_id: ProducerId,
        data: ProducerEnableTraceEventData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ProducerSendNotification {
    pub(crate) handler_id: ProducerId,
}

impl Notification for ProducerSendNotification {
    fn as_event(&self) -> &'static str {
        "producer.send"
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ConsumerCloseRequestData {
    pub(crate) consumer_id: ConsumerId,
}

request_response!(
    "transport.closeConsumer",
    ConsumerCloseRequest {
        handler_id: TransportId,
        data: ConsumerCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "consumer.dump",
    ConsumerDumpRequest {
        handler_id: ConsumerId,
    },
    ConsumerDump,
);

request_response!(
    "consumer.getStats",
    ConsumerGetStatsRequest {
        handler_id: ConsumerId,
    },
    ConsumerStats,
);

request_response!(
    "consumer.pause",
    ConsumerPauseRequest {
        handler_id: ConsumerId,
    },
);

request_response!(
    "consumer.resume",
    ConsumerResumeRequest {
        handler_id: ConsumerId,
    },
);

request_response!(
    "consumer.setPreferredLayers",
    ConsumerSetPreferredLayersRequest {
        handler_id: ConsumerId,
        data: ConsumerLayers,
    },
    Option<ConsumerLayers>,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ConsumerSetPriorityData {
    pub(crate) priority: u8,
}

request_response!(
    "consumer.setPriority",
    ConsumerSetPriorityRequest {
        handler_id: ConsumerId,
        data: ConsumerSetPriorityData,
    },
    ConsumerSetPriorityResponse { priority: u8 },
);

request_response!(
    "consumer.requestKeyFrame",
    ConsumerRequestKeyFrameRequest {
        handler_id: ConsumerId,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ConsumerEnableTraceEventData {
    pub(crate) types: Vec<ConsumerTraceEventType>,
}

request_response!(
    "consumer.enableTraceEvent",
    ConsumerEnableTraceEventRequest {
        handler_id: ConsumerId,
        data: ConsumerEnableTraceEventData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataProducerCloseRequestData {
    pub(crate) data_producer_id: DataProducerId,
}

request_response!(
    "transport.closeDataProducer",
    DataProducerCloseRequest {
        handler_id: TransportId,
        data: DataProducerCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "dataProducer.dump",
    DataProducerDumpRequest {
        handler_id: DataProducerId,
    },
    DataProducerDump,
);

request_response!(
    "dataProducer.getStats",
    DataProducerGetStatsRequest {
        handler_id: DataProducerId,
    },
    Vec<DataProducerStat>,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataProducerSendData {
    pub(crate) ppid: u32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataProducerSendNotification {
    pub(crate) handler_id: DataProducerId,
    pub(crate) data: DataProducerSendData,
}

impl Notification for DataProducerSendNotification {
    fn as_event(&self) -> &'static str {
        "dataProducer.send"
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataConsumerCloseRequestData {
    pub(crate) data_consumer_id: DataConsumerId,
}

request_response!(
    "transport.closeDataConsumer",
    DataConsumerCloseRequest {
        handler_id: TransportId,
        data: DataConsumerCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "dataConsumer.dump",
    DataConsumerDumpRequest {
        handler_id: DataConsumerId,
    },
    DataConsumerDump,
);

request_response!(
    "dataConsumer.getStats",
    DataConsumerGetStatsRequest {
        handler_id: DataConsumerId,
    },
    Vec<DataConsumerStat>,
);

request_response!(
    "dataConsumer.getBufferedAmount",
    DataConsumerGetBufferedAmountRequest {
        handler_id: DataConsumerId,
    },
    DataConsumerGetBufferedAmountResponse {
        buffered_amount: u32,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataConsumerSetBufferedAmountLowThresholdData {
    pub(crate) threshold: u32,
}

request_response!(
    "dataConsumer.setBufferedAmountLowThreshold",
    DataConsumerSetBufferedAmountLowThresholdRequest {
        handler_id: DataConsumerId,
        data: DataConsumerSetBufferedAmountLowThresholdData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataConsumerSendRequestData {
    pub(crate) ppid: u32,
}

request_response!(
    "dataConsumer.send",
    DataConsumerSendRequest {
        handler_id: DataConsumerId,
        data: DataConsumerSendRequestData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RtpObserverCloseRequestData {
    pub(crate) rtp_observer_id: RtpObserverId,
}

request_response!(
    "router.closeRtpObserver",
    RtpObserverCloseRequest {
        handler_id: RouterId,
        data: RtpObserverCloseRequestData,
    },
    (),
    Some(()),
);

request_response!(
    "rtpObserver.pause",
    RtpObserverPauseRequest {
        handler_id: RtpObserverId,
    },
);

request_response!(
    "rtpObserver.resume",
    RtpObserverResumeRequest {
        handler_id: RtpObserverId,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RtpObserverAddRemoveProducerRequestData {
    pub(crate) producer_id: ProducerId,
}

request_response!(
    "rtpObserver.addProducer",
    RtpObserverAddProducerRequest {
        handler_id: RtpObserverId,
        data: RtpObserverAddRemoveProducerRequestData,
    },
);

request_response!(
    "rtpObserver.removeProducer",
    RtpObserverRemoveProducerRequest {
        handler_id: RtpObserverId,
        data: RtpObserverAddRemoveProducerRequestData,
    },
);
