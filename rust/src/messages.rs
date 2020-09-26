use crate::consumer::{ConsumerDump, ConsumerLayers, ConsumerScore, ConsumerStats, ConsumerType};
use crate::data_structures::*;
use crate::ortc::RtpMapping;
use crate::producer::{ProducerDump, ProducerStat, ProducerTraceEventType, ProducerType};
use crate::router::RouterDump;
use crate::rtp_parameters::{MediaKind, RtpEncodingParameters, RtpParameters};
use crate::transport::TransportTraceEventType;
use crate::worker::{WorkerDump, WorkerResourceUsage, WorkerUpdateSettings};
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;
use std::marker::PhantomData;

pub(crate) trait Request: Debug + Serialize {
    type Response: DeserializeOwned;

    fn as_method(&self) -> &'static str;
}

macro_rules! request_response {
    (
        $method: literal,
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty, )* },
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
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty, )* },
        $response_struct_name: ident { $( $response_field_name: ident: $response_field_type: ty, )* },
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
        $request_struct_name: ident { $( $request_field_name: ident: $request_field_type: ty, )* },
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

request_response!(
    "router.createWebRtcTransport",
    RouterCreateWebrtcTransportRequest {
        internal: TransportInternal,
        data: RouterCreateWebrtcTransportData,
    },
    WebRtcTransportData,
);

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

// request_response!(
//     TransportProduceDataRequest,
//     "transport.produceData",
//     ;,
//     TransportProduceDataResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     TransportConsumeDataRequest,
//     "transport.consumeData",
//     ;,
//     TransportConsumeDataResponse,
//     {
//         // TODO
//     },
// );
//

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

// request_response!(
//     ConsumerSetPriorityRequest,
//     "consumer.setPriority",
//     ;,
//     ConsumerSetPriorityResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerRequestKeyFrameRequest,
//     "consumer.requestKeyFrame",
//     ;,
//     ConsumerRequestKeyFrameResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerEnableTraceEventRequest,
//     "consumer.enableTraceEvent",
//     ;,
//     ConsumerEnableTraceEventResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataProducerCloseRequest,
//     "dataProducer.close",
//     ;,
//     DataProducerCloseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataProducerDumpRequest,
//     "dataProducer.dump",
//     ;,
//     DataProducerDumpResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataProducerGetStatsRequest,
//     "dataProducer.getStats",
//     ;,
//     DataProducerGetStatsResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     DataConsumerCloseRequest,
//     "dataConsumer.close",
//     ;,
//     DataConsumerCloseResponse,
//     {
//         // TODO
//     },
// );
//
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
