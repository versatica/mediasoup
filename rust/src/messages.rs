use crate::consumer::{
    ConsumerDump, ConsumerId, ConsumerLayers, ConsumerScore, ConsumerStats, ConsumerTraceEventType,
    ConsumerType,
};
use crate::data_consumer::{DataConsumerId, DataConsumerType};
use crate::data_producer::{DataProducerDump, DataProducerId, DataProducerStat};
use crate::data_structures::{
    DtlsParameters, DtlsRole, DtlsState, IceCandidate, IceParameters, IceRole, IceState, SctpState,
    TransportListenIp, TransportTuple,
};
use crate::ortc::RtpMapping;
use crate::producer::{
    ProducerDump, ProducerId, ProducerStat, ProducerTraceEventType, ProducerType,
};
use crate::router::data_producer::DataProducerType;
use crate::router::{RouterDump, RouterId};
use crate::rtp_parameters::{MediaKind, RtpEncodingParameters, RtpParameters};
use crate::sctp_parameters::{NumSctpStreams, SctpParameters, SctpStreamParameters};
use crate::transport::{TransportId, TransportTraceEventType};
use crate::webrtc_transport::{TransportListenIps, WebRtcTransportOptions};
use crate::worker::{WorkerDump, WorkerResourceUsage, WorkerUpdateSettings};
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;
use std::marker::PhantomData;
use std::sync::Mutex;

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

macro_rules! request_response_generic {
    (
        $method: literal,
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $generic_response: ident,
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_struct_name<$generic_response>
        where
            $generic_response: Debug + DeserializeOwned,
        {
            $( pub(crate) $request_field_name: $request_field_type, )*
            #[serde(skip)]
            pub(crate) phantom_data: PhantomData<$generic_response>,
        }

        impl<$generic_response: Debug + DeserializeOwned> Request for $request_struct_name<$generic_response> {
            type Response = $generic_response;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
}

request_response!("worker.dump", WorkerDumpRequest {}, WorkerDump);

request_response!(
    "worker.getResourceUsage",
    WorkerGetResourceRequest {},
    WorkerResourceUsage,
);

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
pub(crate) struct RouterCreateWebrtcTransportData {
    listen_ips: TransportListenIps,
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
    pub(crate) listen_ip: TransportListenIp,
    pub(crate) rtcp_mux: bool,
    pub(crate) comedia: bool,
    pub(crate) enable_sctp: bool,
    pub(crate) num_sctp_streams: NumSctpStreams,
    pub(crate) max_sctp_message_size: u32,
    pub(crate) sctp_send_buffer_size: u32,
    pub(crate) enable_srtp: bool,
    pub(crate) srtp_crypto_suite: String,
    is_data_channel: bool,
}

impl RouterCreatePlainTransportData {
    pub(crate) fn new(listen_ip: TransportListenIp) -> Self {
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

// request_response!(
//     "router.createPlainTransport",
//     RouterCreatePlainTransportRequest {
//         internal: TransportInternal,
//         data: RouterCreatePlainTransportData,
//     },
//     RouterCreatePlainTransportResponse {
//         // TODO
//     },
// );

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreatePipeTransportData {
    pub(crate) listen_ip: TransportListenIp,
    pub(crate) enable_sctp: bool,
    pub(crate) num_sctp_streams: NumSctpStreams,
    pub(crate) max_sctp_message_size: u32,
    pub(crate) sctp_send_buffer_size: u32,
    pub(crate) enable_rtx: bool,
    pub(crate) enable_srtp: bool,
    is_data_channel: bool,
}

impl RouterCreatePipeTransportData {
    pub(crate) fn new(listen_ip: TransportListenIp) -> Self {
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

// request_response!(
//     "router.createPipeTransport",
//     RouterCreatePipeTransportRequest {
//         internal: TransportInternal,
//         data: RouterCreatePipeTransportData,
//     },
//     RouterCreatePipeTransportResponse {
//         // TODO
//     },
// );

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateDirectTransportData {
    pub(crate) direct: bool,
    pub(crate) max_message_size: u32,
}

impl Default for RouterCreateDirectTransportData {
    fn default() -> Self {
        Self {
            direct: true,
            max_message_size: 262144,
        }
    }
}

// request_response!(
//     "router.createDirectTransport",
//     RouterCreateDirectTransportRequest {
//         internal: TransportInternal,
//         data: RouterCreateDirectTransportData,
//     },
//     RouterCreateDirectTransportResponse {
//         // TODO
//     },
// );

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateAudioLevelObserverData {
    pub(crate) max_entries: u16,
    pub(crate) threshold: i8,
    pub(crate) interval: u16,
}

impl Default for RouterCreateAudioLevelObserverData {
    fn default() -> Self {
        Self {
            max_entries: 1,
            threshold: -80,
            interval: 1000,
        }
    }
}

// request_response!(
//     "router.createAudioLevelObserver",
//     RouterCreateAudioLevelObserverRequest {
//         internal: RouterCreateAudioLevelObserverInternal,
//         data: RouterCreateAudioLevelObserverData,
//     },
//     RouterCreateAudioLevelObserverResponse {
//         // TODO
//     },
// );

request_response!(
    "transport.close",
    TransportCloseRequest {
        internal: TransportInternal,
    },
);

request_response_generic!(
    "transport.dump",
    TransportDumpRequest {
        internal: TransportInternal,
    },
    Dump,
);

request_response_generic!(
    "transport.getStats",
    TransportGetStatsRequest {
        internal: TransportInternal,
    },
    Stats,
);

#[derive(Debug, Serialize)]
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
pub(crate) struct TransportProduceRequestData {
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
        data: TransportProduceRequestData,
    },
    TransportProduceResponse {
        r#type: ProducerType,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportConsumeRequestData {
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
        data: TransportConsumeRequestData,
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
pub(crate) struct TransportProduceDataRequestData {
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
        data: TransportProduceDataRequestData,
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
pub(crate) struct TransportConsumeDataRequestData {
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
        data: TransportConsumeDataRequestData,
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
pub(crate) struct TransportEnableTraceEventRequestData {
    pub(crate) types: Vec<TransportTraceEventType>,
}

request_response!(
    "transport.enableTraceEvent",
    TransportEnableTraceEventRequest {
        internal: TransportInternal,
        data: TransportEnableTraceEventRequestData,
    },
);

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
pub(crate) struct ProducerEnableTraceEventRequestData {
    pub(crate) types: Vec<ProducerTraceEventType>,
}

request_response!(
    "producer.enableTraceEvent",
    ProducerEnableTraceEventRequest {
        internal: ProducerInternal,
        data: ProducerEnableTraceEventRequestData,
    },
);

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
pub(crate) struct ConsumerSetPriorityRequestData {
    pub(crate) priority: u8,
}

request_response!(
    "consumer.setPriority",
    ConsumerSetPriorityRequest {
        internal: ConsumerInternal,
        data: ConsumerSetPriorityRequestData,
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
pub(crate) struct ConsumerEnableTraceEventRequestData {
    pub(crate) types: Vec<ConsumerTraceEventType>,
}

request_response!(
    "producer.enableTraceEvent",
    ConsumerEnableTraceEventRequest {
        internal: ConsumerInternal,
        data: ConsumerEnableTraceEventRequestData,
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

request_response!(
    "dataConsumer.close",
    DataConsumerCloseRequest {
        internal: DataConsumerInternal
    },
);

// request_response!(
//     DataConsumerDumpRequest,
//     "dataConsumer.dump",
//     ;,
//     DataConsumerDumpResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataConsumerGetStatsRequest,
//     "dataConsumer.getStats",
//     ;,
//     DataConsumerGetStatsResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataConsumerGetBufferedAmountRequest,
//     "dataConsumer.getBufferedAmount",
//     ;,
//     DataConsumerGetBufferedAmountResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataConsumerSetBufferedAmountLowThresholdRequest,
//     "dataConsumer.setBufferedAmountLowThreshold",
//     ;,
//     DataConsumerSetBufferedAmountLowThresholdResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     RtpObserverCloseRequest,
//     "rtpObserver.close",
//     ;,
//     RtpObserverCloseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     RtpObserverPauseRequest,
//     "rtpObserver.pause",
//     ;,
//     RtpObserverPauseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     RtpObserverResumeRequest,
//     "rtpObserver.resume",
//     ;,
//     RtpObserverResumeResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     RtpObserverAddProducerRequest,
//     "rtpObserver.addProducer",
//     ;,
//     RtpObserverAddProducerResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     RtpObserverRemoveProducerRequest,
//     "rtpObserver.removeProducer",
//     ;,
//     RtpObserverRemoveProducerResponse,
//     {
//         // TODO
//     },
// );
