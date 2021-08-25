use crate::active_speaker_observer::ActiveSpeakerObserverOptions;
use crate::audio_level_observer::AudioLevelObserverOptions;
use crate::consumer::{
    ConsumerDump, ConsumerId, ConsumerLayers, ConsumerScore, ConsumerStats, ConsumerTraceEventType,
    ConsumerType,
};
use crate::data_consumer::{DataConsumerDump, DataConsumerId, DataConsumerStat, DataConsumerType};
use crate::data_producer::{DataProducerDump, DataProducerId, DataProducerStat, DataProducerType};
use crate::data_structures::{
    DtlsParameters, DtlsRole, DtlsState, IceCandidate, IceParameters, IceRole, IceState, SctpState,
    TransportListenIp, TransportTuple,
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
use crate::webrtc_transport::{TransportListenIps, WebRtcTransportOptions};
use crate::worker::{WorkerDump, WorkerUpdateSettings};
use parking_lot::Mutex;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::Debug;
use std::net::IpAddr;
use std::num::NonZeroU16;

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterInternal {
    pub(crate) router_id: RouterId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportInternal {
    pub(crate) router_id: RouterId,
    pub(crate) transport_id: TransportId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RtpObserverInternal {
    pub(crate) router_id: RouterId,
    pub(crate) rtp_observer_id: RtpObserverId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ProducerInternal {
    pub(crate) router_id: RouterId,
    pub(crate) transport_id: TransportId,
    pub(crate) producer_id: ProducerId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ConsumerInternal {
    pub(crate) router_id: RouterId,
    pub(crate) transport_id: TransportId,
    pub(crate) consumer_id: ConsumerId,
    pub(crate) producer_id: ProducerId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataProducerInternal {
    pub(crate) router_id: RouterId,
    pub(crate) transport_id: TransportId,
    pub(crate) data_producer_id: DataProducerId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataConsumerInternal {
    pub(crate) router_id: RouterId,
    pub(crate) transport_id: TransportId,
    pub(crate) data_producer_id: DataProducerId,
    pub(crate) data_consumer_id: DataConsumerId,
}

pub(crate) trait Request: Debug + Serialize {
    type Response: DeserializeOwned;

    fn as_method(&self) -> &'static str;
}

pub(crate) trait Notification: Debug + Serialize {
    fn as_event(&self) -> &'static str;
}

macro_rules! request_response {
    (
        $method: literal,
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $existing_response_type: ty $(,)?
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_struct_name {
            $( pub(crate) $request_field_name: $request_field_type, )*
        }

        impl Request for $request_struct_name {
            type Response = $existing_response_type;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
    // Call above macro with unit type as expected response
    (
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
    ) => {
        request_response!($method, $request_struct_name $request_struct_impl, ());
    };
    (
        $method: literal,
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $response_struct_name: ident { $( $response_field_name: ident: $response_field_type: ty$(,)? )* },
    ) => {
        #[derive(Debug, Serialize)]
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

request_response!(
    "worker.createRouter",
    WorkerCreateRouterRequest {
        internal: RouterInternal,
    },
);

request_response!(
    "router.close",
    RouterCloseRequest {
        internal: RouterInternal,
    },
);

request_response!(
    "router.dump",
    RouterDumpRequest {
        internal: RouterInternal,
    },
    RouterDump,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateDirectTransportData {
    pub(crate) direct: bool,
    pub(crate) max_message_size: usize,
}

impl RouterCreateDirectTransportData {
    pub(crate) fn from_options(direct_transport_options: &DirectTransportOptions) -> Self {
        Self {
            direct: true,
            max_message_size: direct_transport_options.max_message_size,
        }
    }
}

request_response!(
    "router.createDirectTransport",
    RouterCreateDirectTransportRequest {
        internal: TransportInternal,
        data: RouterCreateDirectTransportData,
    },
    RouterCreateDirectTransportResponse {},
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateWebrtcTransportData {
    listen_ips: TransportListenIps,
    #[serde(skip_serializing_if = "Option::is_none")]
    port: Option<u16>,
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
    pub(crate) fn from_options(webrtc_transport_options: &WebRtcTransportOptions) -> Self {
        Self {
            listen_ips: webrtc_transport_options.listen_ips.clone(),
            port: webrtc_transport_options.port,
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

request_response!(
    "router.createWebRtcTransport",
    RouterCreateWebrtcTransportRequest {
        internal: TransportInternal,
        data: RouterCreateWebrtcTransportData,
    },
    WebRtcTransportData {
        ice_role: IceRole,
        ice_parameters: IceParameters,
        ice_candidates: Vec<IceCandidate>,
        ice_state: Mutex<IceState>,
        ice_selected_tuple: Mutex<Option<TransportTuple>>,
        dtls_parameters: Mutex<DtlsParameters>,
        dtls_state: Mutex<DtlsState>,
        dtls_remote_cert: Mutex<Option<String>>,
        sctp_parameters: Option<SctpParameters>,
        sctp_state: Mutex<Option<SctpState>>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreatePlainTransportData {
    listen_ip: TransportListenIp,
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
    pub(crate) fn from_options(plain_transport_options: &PlainTransportOptions) -> Self {
        Self {
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
        internal: TransportInternal,
        data: RouterCreatePlainTransportData,
    },
    PlainTransportData {
        rtcp_mux: bool,
        comedia: bool,
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
    listen_ip: TransportListenIp,
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
    pub(crate) fn from_options(pipe_transport_options: &PipeTransportOptions) -> Self {
        Self {
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
        internal: TransportInternal,
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
    pub(crate) max_entries: NonZeroU16,
    pub(crate) threshold: i8,
    pub(crate) interval: u16,
}

impl RouterCreateAudioLevelObserverData {
    pub(crate) fn from_options(audio_level_observer_options: &AudioLevelObserverOptions) -> Self {
        Self {
            max_entries: audio_level_observer_options.max_entries,
            threshold: audio_level_observer_options.threshold,
            interval: audio_level_observer_options.interval,
        }
    }
}

request_response!(
    "router.createAudioLevelObserver",
    RouterCreateAudioLevelObserverRequest {
        internal: RtpObserverInternal,
        data: RouterCreateAudioLevelObserverData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateActiveSpeakerObserverData {
    pub(crate) interval: u16,
}

impl RouterCreateActiveSpeakerObserverData {
    pub(crate) fn from_options(
        active_speaker_observer_options: &ActiveSpeakerObserverOptions,
    ) -> Self {
        Self {
            interval: active_speaker_observer_options.interval,
        }
    }
}

request_response!(
    "router.createActiveSpeakerObserver",
    RouterCreateActiveSpeakerObserverRequest {
        internal: RtpObserverInternal,
        data: RouterCreateActiveSpeakerObserverData,
    },
);

request_response!(
    "transport.close",
    TransportCloseRequest {
        internal: TransportInternal,
    },
);

request_response!(
    "transport.dump",
    TransportDumpRequest {
        internal: TransportInternal,
    },
    Value,
);

request_response!(
    "transport.getStats",
    TransportGetStatsRequest {
        internal: TransportInternal,
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
    TransportConnectRequestWebRtc {
        internal: TransportInternal,
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
    TransportConnectRequestPipe {
        internal: TransportInternal,
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
    TransportConnectRequestPlain {
        internal: TransportInternal,
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
        internal: TransportInternal,
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
        internal: TransportInternal,
        data: TransportSetMaxOutgoingBitrateData,
    },
);

request_response!(
    "transport.restartIce",
    TransportRestartIceRequest {
        internal: TransportInternal,
    },
    TransportRestartIceResponse {
        ice_parameters: IceParameters,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportProduceData {
    pub(crate) kind: MediaKind,
    pub(crate) rtp_parameters: RtpParameters,
    pub(crate) rtp_mapping: RtpMapping,
    pub(crate) key_frame_request_delay: u32,
    pub(crate) paused: bool,
}

request_response!(
    "transport.produce",
    TransportProduceRequest {
        internal: ProducerInternal,
        data: TransportProduceData,
    },
    TransportProduceResponse {
        r#type: ProducerType,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConsumeData {
    pub(crate) kind: MediaKind,
    pub(crate) rtp_parameters: RtpParameters,
    pub(crate) r#type: ConsumerType,
    pub(crate) consumable_rtp_encodings: Vec<RtpEncodingParameters>,
    pub(crate) paused: bool,
    pub(crate) preferred_layers: Option<ConsumerLayers>,
}

request_response!(
    "transport.consume",
    TransportConsumeRequest {
        internal: ConsumerInternal,
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
    pub(crate) r#type: DataProducerType,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) sctp_stream_parameters: Option<SctpStreamParameters>,
    pub(crate) label: String,
    pub(crate) protocol: String,
}

request_response!(
    "transport.produceData",
    TransportProduceDataRequest {
        internal: DataProducerInternal,
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
    pub(crate) r#type: DataConsumerType,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub(crate) sctp_stream_parameters: Option<SctpStreamParameters>,
    pub(crate) label: String,
    pub(crate) protocol: String,
}

request_response!(
    "transport.consumeData",
    TransportConsumeDataRequest {
        internal: DataConsumerInternal,
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
        internal: TransportInternal,
        data: TransportEnableTraceEventData,
    },
);

#[derive(Debug, Serialize)]
pub(crate) struct TransportSendRtcpNotification {
    pub(crate) internal: TransportInternal,
}

impl Notification for TransportSendRtcpNotification {
    fn as_event(&self) -> &'static str {
        "transport.sendRtcp"
    }
}

request_response!(
    "producer.close",
    ProducerCloseRequest {
        internal: ProducerInternal,
    },
);

request_response!(
    "producer.dump",
    ProducerDumpRequest {
        internal: ProducerInternal,
    },
    ProducerDump
);

request_response!(
    "producer.getStats",
    ProducerGetStatsRequest {
        internal: ProducerInternal,
    },
    Vec<ProducerStat>,
);

request_response!(
    "producer.pause",
    ProducerPauseRequest {
        internal: ProducerInternal,
    },
);

request_response!(
    "producer.resume",
    ProducerResumeRequest {
        internal: ProducerInternal,
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
        internal: ProducerInternal,
        data: ProducerEnableTraceEventData,
    },
);

#[derive(Debug, Serialize)]
pub(crate) struct ProducerSendNotification {
    pub(crate) internal: ProducerInternal,
}

impl Notification for ProducerSendNotification {
    fn as_event(&self) -> &'static str {
        "producer.send"
    }
}

request_response!(
    "consumer.close",
    ConsumerCloseRequest {
        internal: ConsumerInternal,
    },
);

request_response!(
    "consumer.dump",
    ConsumerDumpRequest {
        internal: ConsumerInternal,
    },
    ConsumerDump,
);

request_response!(
    "consumer.getStats",
    ConsumerGetStatsRequest {
        internal: ConsumerInternal,
    },
    ConsumerStats,
);

request_response!(
    "consumer.pause",
    ConsumerPauseRequest {
        internal: ConsumerInternal,
    },
);

request_response!(
    "consumer.resume",
    ConsumerResumeRequest {
        internal: ConsumerInternal,
    },
);

request_response!(
    "consumer.setPreferredLayers",
    ConsumerSetPreferredLayersRequest {
        internal: ConsumerInternal,
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
        internal: ConsumerInternal,
        data: ConsumerSetPriorityData,
    },
    ConsumerSetPriorityResponse { priority: u8 },
);

request_response!(
    "consumer.requestKeyFrame",
    ConsumerRequestKeyFrameRequest {
        internal: ConsumerInternal,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ConsumerEnableTraceEventData {
    pub(crate) types: Vec<ConsumerTraceEventType>,
}

request_response!(
    "producer.enableTraceEvent",
    ConsumerEnableTraceEventRequest {
        internal: ConsumerInternal,
        data: ConsumerEnableTraceEventData,
    },
);

request_response!(
    "dataProducer.close",
    DataProducerCloseRequest {
        internal: DataProducerInternal,
    },
);

request_response!(
    "dataProducer.dump",
    DataProducerDumpRequest {
        internal: DataProducerInternal,
    },
    DataProducerDump,
);

request_response!(
    "dataProducer.getStats",
    DataProducerGetStatsRequest {
        internal: DataProducerInternal,
    },
    Vec<DataProducerStat>,
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct DataProducerSendData {
    pub(crate) ppid: u32,
}

#[derive(Debug, Serialize)]
pub(crate) struct DataProducerSendNotification {
    pub(crate) internal: DataProducerInternal,
    pub(crate) data: DataProducerSendData,
}

impl Notification for DataProducerSendNotification {
    fn as_event(&self) -> &'static str {
        "dataProducer.send"
    }
}

request_response!(
    "dataConsumer.close",
    DataConsumerCloseRequest {
        internal: DataConsumerInternal
    },
);

request_response!(
    "dataConsumer.dump",
    DataConsumerDumpRequest {
        internal: DataConsumerInternal,
    },
    DataConsumerDump,
);

request_response!(
    "dataConsumer.getStats",
    DataConsumerGetStatsRequest {
        internal: DataConsumerInternal,
    },
    Vec<DataConsumerStat>,
);

request_response!(
    "dataConsumer.getBufferedAmount",
    DataConsumerGetBufferedAmountRequest {
        internal: DataConsumerInternal,
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
        internal: DataConsumerInternal,
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
        internal: DataConsumerInternal,
        data: DataConsumerSendRequestData,
    },
);

request_response!(
    "rtpObserver.close",
    RtpObserverCloseRequest {
        internal: RtpObserverInternal,
    },
);

request_response!(
    "rtpObserver.pause",
    RtpObserverPauseRequest {
        internal: RtpObserverInternal,
    },
);

request_response!(
    "rtpObserver.resume",
    RtpObserverResumeRequest {
        internal: RtpObserverInternal,
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
        internal: RtpObserverInternal,
        data: RtpObserverAddRemoveProducerRequestData,
    },
);

request_response!(
    "rtpObserver.removeProducer",
    RtpObserverRemoveProducerRequest {
        internal: RtpObserverInternal,
        data: RtpObserverAddRemoveProducerRequestData,
    },
);

#[derive(Debug, Serialize)]
#[serde(untagged)]
pub(crate) enum MetaRequest {
    WorkerCloseRequest(WorkerCloseRequest),
    WorkerDumpRequest(WorkerDumpRequest),
    WorkerUpdateSettingsRequest(WorkerUpdateSettingsRequest),
    WorkerCreateRouterRequest(WorkerCreateRouterRequest),
    RouterCloseRequest(RouterCloseRequest),
    RouterDumpRequest(RouterDumpRequest),
    RouterCreateDirectTransportRequest(RouterCreateDirectTransportRequest),
    RouterCreateWebrtcTransportRequest(RouterCreateWebrtcTransportRequest),
    RouterCreatePlainTransportRequest(RouterCreatePlainTransportRequest),
    RouterCreatePipeTransportRequest(RouterCreatePipeTransportRequest),
    RouterCreateAudioLevelObserverRequest(RouterCreateAudioLevelObserverRequest),
    RouterCreateActiveSpeakerObserverRequest(RouterCreateActiveSpeakerObserverRequest),
    TransportCloseRequest(TransportCloseRequest),
    TransportDumpRequest(TransportDumpRequest),
    TransportGetStatsRequest(TransportGetStatsRequest),
    TransportConnectRequestWebRtc(TransportConnectRequestWebRtc),
    TransportConnectRequestPipe(TransportConnectRequestPipe),
    TransportConnectRequestPlain(TransportConnectRequestPlain),
    TransportSetMaxIncomingBitrateRequest(TransportSetMaxIncomingBitrateRequest),
    TransportSetMaxOutgoingBitrateRequest(TransportSetMaxOutgoingBitrateRequest),
    TransportRestartIceRequest(TransportRestartIceRequest),
    TransportProduceRequest(TransportProduceRequest),
    TransportConsumeRequest(TransportConsumeRequest),
    TransportProduceDataRequest(TransportProduceDataRequest),
    TransportConsumeDataRequest(TransportConsumeDataRequest),
    TransportEnableTraceEventRequest(TransportEnableTraceEventRequest),
    ProducerCloseRequest(ProducerCloseRequest),
    ProducerDumpRequest(ProducerDumpRequest),
    ProducerGetStatsRequest(ProducerGetStatsRequest),
    ProducerPauseRequest(ProducerPauseRequest),
    ProducerResumeRequest(ProducerResumeRequest),
    ProducerEnableTraceEventRequest(ProducerEnableTraceEventRequest),
    ConsumerCloseRequest(ConsumerCloseRequest),
    ConsumerDumpRequest(ConsumerDumpRequest),
    ConsumerGetStatsRequest(ConsumerGetStatsRequest),
    ConsumerPauseRequest(ConsumerPauseRequest),
    ConsumerResumeRequest(ConsumerResumeRequest),
    ConsumerSetPreferredLayersRequest(ConsumerSetPreferredLayersRequest),
    ConsumerSetPriorityRequest(ConsumerSetPriorityRequest),
    ConsumerRequestKeyFrameRequest(ConsumerRequestKeyFrameRequest),
    ConsumerEnableTraceEventRequest(ConsumerEnableTraceEventRequest),
    DataProducerCloseRequest(DataProducerCloseRequest),
    DataProducerDumpRequest(DataProducerDumpRequest),
    DataProducerGetStatsRequest(DataProducerGetStatsRequest),
    DataConsumerCloseRequest(DataConsumerCloseRequest),
    DataConsumerDumpRequest(DataConsumerDumpRequest),
    DataConsumerGetStatsRequest(DataConsumerGetStatsRequest),
    DataConsumerGetBufferedAmountRequest(DataConsumerGetBufferedAmountRequest),
    DataConsumerSetBufferedAmountLowThresholdRequest(
        DataConsumerSetBufferedAmountLowThresholdRequest,
    ),
    DataConsumerSendRequest(DataConsumerSendRequest),
    RtpObserverCloseRequest(RtpObserverCloseRequest),
    RtpObserverPauseRequest(RtpObserverPauseRequest),
    RtpObserverResumeRequest(RtpObserverResumeRequest),
    RtpObserverAddProducerRequest(RtpObserverAddProducerRequest),
    RtpObserverRemoveProducerRequest(RtpObserverRemoveProducerRequest),
}

impl From<WorkerCloseRequest> for MetaRequest {
    fn from(r: WorkerCloseRequest) -> Self {
        Self::WorkerCloseRequest(r)
    }
}

impl From<WorkerDumpRequest> for MetaRequest {
    fn from(r: WorkerDumpRequest) -> Self {
        Self::WorkerDumpRequest(r)
    }
}

impl From<WorkerUpdateSettingsRequest> for MetaRequest {
    fn from(r: WorkerUpdateSettingsRequest) -> Self {
        Self::WorkerUpdateSettingsRequest(r)
    }
}

impl From<WorkerCreateRouterRequest> for MetaRequest {
    fn from(r: WorkerCreateRouterRequest) -> Self {
        Self::WorkerCreateRouterRequest(r)
    }
}

impl From<RouterCloseRequest> for MetaRequest {
    fn from(r: RouterCloseRequest) -> Self {
        Self::RouterCloseRequest(r)
    }
}

impl From<RouterDumpRequest> for MetaRequest {
    fn from(r: RouterDumpRequest) -> Self {
        Self::RouterDumpRequest(r)
    }
}

impl From<RouterCreateDirectTransportRequest> for MetaRequest {
    fn from(r: RouterCreateDirectTransportRequest) -> Self {
        Self::RouterCreateDirectTransportRequest(r)
    }
}

impl From<RouterCreateWebrtcTransportRequest> for MetaRequest {
    fn from(r: RouterCreateWebrtcTransportRequest) -> Self {
        Self::RouterCreateWebrtcTransportRequest(r)
    }
}

impl From<RouterCreatePlainTransportRequest> for MetaRequest {
    fn from(r: RouterCreatePlainTransportRequest) -> Self {
        Self::RouterCreatePlainTransportRequest(r)
    }
}

impl From<RouterCreatePipeTransportRequest> for MetaRequest {
    fn from(r: RouterCreatePipeTransportRequest) -> Self {
        Self::RouterCreatePipeTransportRequest(r)
    }
}

impl From<RouterCreateAudioLevelObserverRequest> for MetaRequest {
    fn from(r: RouterCreateAudioLevelObserverRequest) -> Self {
        Self::RouterCreateAudioLevelObserverRequest(r)
    }
}

impl From<RouterCreateActiveSpeakerObserverRequest> for MetaRequest {
    fn from(r: RouterCreateActiveSpeakerObserverRequest) -> Self {
        Self::RouterCreateActiveSpeakerObserverRequest(r)
    }
}

impl From<TransportCloseRequest> for MetaRequest {
    fn from(r: TransportCloseRequest) -> Self {
        Self::TransportCloseRequest(r)
    }
}

impl From<TransportDumpRequest> for MetaRequest {
    fn from(r: TransportDumpRequest) -> Self {
        Self::TransportDumpRequest(r)
    }
}

impl From<TransportGetStatsRequest> for MetaRequest {
    fn from(r: TransportGetStatsRequest) -> Self {
        Self::TransportGetStatsRequest(r)
    }
}

impl From<TransportConnectRequestWebRtc> for MetaRequest {
    fn from(r: TransportConnectRequestWebRtc) -> Self {
        Self::TransportConnectRequestWebRtc(r)
    }
}

impl From<TransportConnectRequestPipe> for MetaRequest {
    fn from(r: TransportConnectRequestPipe) -> Self {
        Self::TransportConnectRequestPipe(r)
    }
}

impl From<TransportConnectRequestPlain> for MetaRequest {
    fn from(r: TransportConnectRequestPlain) -> Self {
        Self::TransportConnectRequestPlain(r)
    }
}

impl From<TransportSetMaxIncomingBitrateRequest> for MetaRequest {
    fn from(r: TransportSetMaxIncomingBitrateRequest) -> Self {
        Self::TransportSetMaxIncomingBitrateRequest(r)
    }
}

impl From<TransportSetMaxOutgoingBitrateRequest> for MetaRequest {
    fn from(r: TransportSetMaxOutgoingBitrateRequest) -> Self {
        Self::TransportSetMaxOutgoingBitrateRequest(r)
    }
}

impl From<TransportRestartIceRequest> for MetaRequest {
    fn from(r: TransportRestartIceRequest) -> Self {
        Self::TransportRestartIceRequest(r)
    }
}

impl From<TransportProduceRequest> for MetaRequest {
    fn from(r: TransportProduceRequest) -> Self {
        Self::TransportProduceRequest(r)
    }
}

impl From<TransportConsumeRequest> for MetaRequest {
    fn from(r: TransportConsumeRequest) -> Self {
        Self::TransportConsumeRequest(r)
    }
}

impl From<TransportProduceDataRequest> for MetaRequest {
    fn from(r: TransportProduceDataRequest) -> Self {
        Self::TransportProduceDataRequest(r)
    }
}

impl From<TransportConsumeDataRequest> for MetaRequest {
    fn from(r: TransportConsumeDataRequest) -> Self {
        Self::TransportConsumeDataRequest(r)
    }
}

impl From<TransportEnableTraceEventRequest> for MetaRequest {
    fn from(r: TransportEnableTraceEventRequest) -> Self {
        Self::TransportEnableTraceEventRequest(r)
    }
}

impl From<ProducerCloseRequest> for MetaRequest {
    fn from(r: ProducerCloseRequest) -> Self {
        Self::ProducerCloseRequest(r)
    }
}

impl From<ProducerDumpRequest> for MetaRequest {
    fn from(r: ProducerDumpRequest) -> Self {
        Self::ProducerDumpRequest(r)
    }
}

impl From<ProducerGetStatsRequest> for MetaRequest {
    fn from(r: ProducerGetStatsRequest) -> Self {
        Self::ProducerGetStatsRequest(r)
    }
}

impl From<ProducerPauseRequest> for MetaRequest {
    fn from(r: ProducerPauseRequest) -> Self {
        Self::ProducerPauseRequest(r)
    }
}

impl From<ProducerResumeRequest> for MetaRequest {
    fn from(r: ProducerResumeRequest) -> Self {
        Self::ProducerResumeRequest(r)
    }
}

impl From<ProducerEnableTraceEventRequest> for MetaRequest {
    fn from(r: ProducerEnableTraceEventRequest) -> Self {
        Self::ProducerEnableTraceEventRequest(r)
    }
}

impl From<ConsumerCloseRequest> for MetaRequest {
    fn from(r: ConsumerCloseRequest) -> Self {
        Self::ConsumerCloseRequest(r)
    }
}

impl From<ConsumerDumpRequest> for MetaRequest {
    fn from(r: ConsumerDumpRequest) -> Self {
        Self::ConsumerDumpRequest(r)
    }
}

impl From<ConsumerGetStatsRequest> for MetaRequest {
    fn from(r: ConsumerGetStatsRequest) -> Self {
        Self::ConsumerGetStatsRequest(r)
    }
}

impl From<ConsumerPauseRequest> for MetaRequest {
    fn from(r: ConsumerPauseRequest) -> Self {
        Self::ConsumerPauseRequest(r)
    }
}

impl From<ConsumerResumeRequest> for MetaRequest {
    fn from(r: ConsumerResumeRequest) -> Self {
        Self::ConsumerResumeRequest(r)
    }
}

impl From<ConsumerSetPreferredLayersRequest> for MetaRequest {
    fn from(r: ConsumerSetPreferredLayersRequest) -> Self {
        Self::ConsumerSetPreferredLayersRequest(r)
    }
}

impl From<ConsumerSetPriorityRequest> for MetaRequest {
    fn from(r: ConsumerSetPriorityRequest) -> Self {
        Self::ConsumerSetPriorityRequest(r)
    }
}

impl From<ConsumerRequestKeyFrameRequest> for MetaRequest {
    fn from(r: ConsumerRequestKeyFrameRequest) -> Self {
        Self::ConsumerRequestKeyFrameRequest(r)
    }
}

impl From<ConsumerEnableTraceEventRequest> for MetaRequest {
    fn from(r: ConsumerEnableTraceEventRequest) -> Self {
        Self::ConsumerEnableTraceEventRequest(r)
    }
}

impl From<DataProducerCloseRequest> for MetaRequest {
    fn from(r: DataProducerCloseRequest) -> Self {
        Self::DataProducerCloseRequest(r)
    }
}

impl From<DataProducerDumpRequest> for MetaRequest {
    fn from(r: DataProducerDumpRequest) -> Self {
        Self::DataProducerDumpRequest(r)
    }
}

impl From<DataProducerGetStatsRequest> for MetaRequest {
    fn from(r: DataProducerGetStatsRequest) -> Self {
        Self::DataProducerGetStatsRequest(r)
    }
}

impl From<DataConsumerCloseRequest> for MetaRequest {
    fn from(r: DataConsumerCloseRequest) -> Self {
        Self::DataConsumerCloseRequest(r)
    }
}

impl From<DataConsumerDumpRequest> for MetaRequest {
    fn from(r: DataConsumerDumpRequest) -> Self {
        Self::DataConsumerDumpRequest(r)
    }
}

impl From<DataConsumerGetStatsRequest> for MetaRequest {
    fn from(r: DataConsumerGetStatsRequest) -> Self {
        Self::DataConsumerGetStatsRequest(r)
    }
}

impl From<DataConsumerGetBufferedAmountRequest> for MetaRequest {
    fn from(r: DataConsumerGetBufferedAmountRequest) -> Self {
        Self::DataConsumerGetBufferedAmountRequest(r)
    }
}

impl From<DataConsumerSetBufferedAmountLowThresholdRequest> for MetaRequest {
    fn from(r: DataConsumerSetBufferedAmountLowThresholdRequest) -> Self {
        Self::DataConsumerSetBufferedAmountLowThresholdRequest(r)
    }
}

impl From<DataConsumerSendRequest> for MetaRequest {
    fn from(r: DataConsumerSendRequest) -> Self {
        Self::DataConsumerSendRequest(r)
    }
}

impl From<RtpObserverCloseRequest> for MetaRequest {
    fn from(r: RtpObserverCloseRequest) -> Self {
        Self::RtpObserverCloseRequest(r)
    }
}

impl From<RtpObserverPauseRequest> for MetaRequest {
    fn from(r: RtpObserverPauseRequest) -> Self {
        Self::RtpObserverPauseRequest(r)
    }
}

impl From<RtpObserverResumeRequest> for MetaRequest {
    fn from(r: RtpObserverResumeRequest) -> Self {
        Self::RtpObserverResumeRequest(r)
    }
}

impl From<RtpObserverAddProducerRequest> for MetaRequest {
    fn from(r: RtpObserverAddProducerRequest) -> Self {
        Self::RtpObserverAddProducerRequest(r)
    }
}

impl From<RtpObserverRemoveProducerRequest> for MetaRequest {
    fn from(r: RtpObserverRemoveProducerRequest) -> Self {
        Self::RtpObserverRemoveProducerRequest(r)
    }
}

impl MetaRequest {
    pub(crate) fn as_method(&self) -> &'static str {
        match self {
            MetaRequest::WorkerCloseRequest(m) => m.as_method(),
            MetaRequest::WorkerDumpRequest(m) => m.as_method(),
            MetaRequest::WorkerUpdateSettingsRequest(m) => m.as_method(),
            MetaRequest::WorkerCreateRouterRequest(m) => m.as_method(),
            MetaRequest::RouterCloseRequest(m) => m.as_method(),
            MetaRequest::RouterDumpRequest(m) => m.as_method(),
            MetaRequest::RouterCreateDirectTransportRequest(m) => m.as_method(),
            MetaRequest::RouterCreateWebrtcTransportRequest(m) => m.as_method(),
            MetaRequest::RouterCreatePlainTransportRequest(m) => m.as_method(),
            MetaRequest::RouterCreatePipeTransportRequest(m) => m.as_method(),
            MetaRequest::RouterCreateAudioLevelObserverRequest(m) => m.as_method(),
            MetaRequest::RouterCreateActiveSpeakerObserverRequest(m) => m.as_method(),
            MetaRequest::TransportCloseRequest(m) => m.as_method(),
            MetaRequest::TransportDumpRequest(m) => m.as_method(),
            MetaRequest::TransportGetStatsRequest(m) => m.as_method(),
            MetaRequest::TransportConnectRequestWebRtc(m) => m.as_method(),
            MetaRequest::TransportConnectRequestPipe(m) => m.as_method(),
            MetaRequest::TransportConnectRequestPlain(m) => m.as_method(),
            MetaRequest::TransportSetMaxIncomingBitrateRequest(m) => m.as_method(),
            MetaRequest::TransportSetMaxOutgoingBitrateRequest(m) => m.as_method(),
            MetaRequest::TransportRestartIceRequest(m) => m.as_method(),
            MetaRequest::TransportProduceRequest(m) => m.as_method(),
            MetaRequest::TransportConsumeRequest(m) => m.as_method(),
            MetaRequest::TransportProduceDataRequest(m) => m.as_method(),
            MetaRequest::TransportConsumeDataRequest(m) => m.as_method(),
            MetaRequest::TransportEnableTraceEventRequest(m) => m.as_method(),
            MetaRequest::ProducerCloseRequest(m) => m.as_method(),
            MetaRequest::ProducerDumpRequest(m) => m.as_method(),
            MetaRequest::ProducerGetStatsRequest(m) => m.as_method(),
            MetaRequest::ProducerPauseRequest(m) => m.as_method(),
            MetaRequest::ProducerResumeRequest(m) => m.as_method(),
            MetaRequest::ProducerEnableTraceEventRequest(m) => m.as_method(),
            MetaRequest::ConsumerCloseRequest(m) => m.as_method(),
            MetaRequest::ConsumerDumpRequest(m) => m.as_method(),
            MetaRequest::ConsumerGetStatsRequest(m) => m.as_method(),
            MetaRequest::ConsumerPauseRequest(m) => m.as_method(),
            MetaRequest::ConsumerResumeRequest(m) => m.as_method(),
            MetaRequest::ConsumerSetPreferredLayersRequest(m) => m.as_method(),
            MetaRequest::ConsumerSetPriorityRequest(m) => m.as_method(),
            MetaRequest::ConsumerRequestKeyFrameRequest(m) => m.as_method(),
            MetaRequest::ConsumerEnableTraceEventRequest(m) => m.as_method(),
            MetaRequest::DataProducerCloseRequest(m) => m.as_method(),
            MetaRequest::DataProducerDumpRequest(m) => m.as_method(),
            MetaRequest::DataProducerGetStatsRequest(m) => m.as_method(),
            MetaRequest::DataConsumerCloseRequest(m) => m.as_method(),
            MetaRequest::DataConsumerDumpRequest(m) => m.as_method(),
            MetaRequest::DataConsumerGetStatsRequest(m) => m.as_method(),
            MetaRequest::DataConsumerGetBufferedAmountRequest(m) => m.as_method(),
            MetaRequest::DataConsumerSetBufferedAmountLowThresholdRequest(m) => m.as_method(),
            MetaRequest::DataConsumerSendRequest(m) => m.as_method(),
            MetaRequest::RtpObserverCloseRequest(m) => m.as_method(),
            MetaRequest::RtpObserverPauseRequest(m) => m.as_method(),
            MetaRequest::RtpObserverResumeRequest(m) => m.as_method(),
            MetaRequest::RtpObserverAddProducerRequest(m) => m.as_method(),
            MetaRequest::RtpObserverRemoveProducerRequest(m) => m.as_method(),
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(untagged)]
pub(crate) enum MetaNotification {
    TransportSendRtcpNotification(TransportSendRtcpNotification),
    ProducerSendNotification(ProducerSendNotification),
    DataProducerSendNotification(DataProducerSendNotification),
}

impl From<TransportSendRtcpNotification> for MetaNotification {
    fn from(n: TransportSendRtcpNotification) -> Self {
        Self::TransportSendRtcpNotification(n)
    }
}
impl From<ProducerSendNotification> for MetaNotification {
    fn from(n: ProducerSendNotification) -> Self {
        Self::ProducerSendNotification(n)
    }
}
impl From<DataProducerSendNotification> for MetaNotification {
    fn from(n: DataProducerSendNotification) -> Self {
        Self::DataProducerSendNotification(n)
    }
}

impl MetaNotification {
    pub(crate) fn as_event(&self) -> &'static str {
        match self {
            Self::TransportSendRtcpNotification(n) => n.as_event(),
            Self::ProducerSendNotification(n) => n.as_event(),
            Self::DataProducerSendNotification(n) => n.as_event(),
        }
    }
}
