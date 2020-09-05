use crate::data_structures::*;
use crate::worker::{WorkerDump, WorkerResourceUsage, WorkerUpdateSettings};
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;

pub(crate) trait Request: Debug + Serialize {
    type Response: DeserializeOwned;

    fn as_method(&self) -> &'static str;
}

macro_rules! request_response {
    (
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt,
        $existing_response_type: ty $(,)?
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_struct_name $request_struct_impl

        impl Request for $request_struct_name {
            type Response = $existing_response_type;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
    (
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
    ) => {
        // Call above macro with unit type as expected response
        request_response!($method, $request_struct_name $request_struct_impl, ());
    };
    (
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt,
        $response_struct_name: ident $response_struct_impl: tt,
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_struct_name $request_struct_impl

        #[derive(Debug, Deserialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $response_struct_name $response_struct_impl

        impl Request for $request_struct_name {
            type Response = $response_struct_name;

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
        pub(crate) data: WorkerUpdateSettings,
    },
);

request_response!(
    "worker.createRouter",
    WorkerCreateRouterRequest {
        pub(crate) internal: RouterInternal,
    },
);

request_response!(
    "router.close",
    RouterCloseRequest {
        pub(crate) internal: RouterInternal,
    },
);

request_response!(
    "router.dump",
    RouterDumpRequest {
        pub(crate) internal: RouterInternal,
    },
    RouterDumpResponse {
        // TODO
    },
);

request_response!(
    "router.createWebRtcTransport",
    RouterCreateWebrtcTransportRequest {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreateWebrtcTransportData,
    },
    RouterCreateWebrtcTransportResponse {
        // TODO
    },
);

request_response!(
    "router.createPlainTransport",
    RouterCreatePlainTransportRequest {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreatePlainTransportData,
    },
    RouterCreatePlainTransportResponse {
        // TODO
    },
);

request_response!(
    "router.createPipeTransport",
    RouterCreatePipeTransportRequest {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreatePipeTransportData,
    },
    RouterCreatePipeTransportResponse {
        // TODO
    },
);

request_response!(
    "router.createDirectTransport",
    RouterCreateDirectTransportRequest {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreateDirectTransportData,
    },
    RouterCreateDirectTransportResponse {
        // TODO
    },
);

request_response!(
    "router.createAudioLevelObserver",
    RouterCreateAudioLevelObserverRequest {
        pub(crate) internal: RouterCreateAudioLevelObserverInternal,
        pub(crate) data: RouterCreateAudioLevelObserverData,
    },
    RouterCreateAudioLevelObserverResponse {
        // TODO
    },
);

request_response!(
    "transport.close",
    TransportCloseRequest {
        pub(crate) internal: TransportInternal,
    },
    TransportCloseResponse {
        // TODO
    },
);

request_response!(
    "transport.dump",
    TransportDumpRequest {
        pub(crate) internal: TransportInternal,
    },
    TransportDumpResponse {
        // TODO
    },
);

request_response!(
    "transport.getStats",
    TransportGetStatsRequest {
        pub(crate) internal: TransportInternal,
    },
    TransportGetStatsResponse {
        // TODO
    },
);

request_response!(
    "transport.connect",
    TransportConnectRequest {
        pub(crate) internal: TransportInternal,
        pub(crate) data: TransportConnectData,
    },
    TransportConnectResponse {
        // TODO
    },
);

request_response!(
    "transport.setMaxIncomingBitrate",
    TransportSetMaxIncomingBitrateRequest {
        pub(crate) internal: TransportInternal,
        pub(crate) data: TransportSetMaxIncomingBitrateData,
    },
    TransportSetMaxIncomingBitrateResponse {
        // TODO
    },
);

// TODO: Detail remaining methods, I got bored for now
request_response!(
    "transport.restartIce",
    TransportRestartIceRequest {
        pub(crate) internal: TransportInternal,
    },
    TransportRestartIceResponse {
        // TODO
    },
);

// request_response!(
//     TransportProduceRequest,
//     "transport.produce",
//     ;,
//     TransportProduceResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     TransportConsumeRequest,
//     "transport.consume",
//     ;,
//     TransportConsumeResponse,
//     {
//         // TODO
//     },
// );
//
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
// request_response!(
//     TransportEnableTraceEventRequest,
//     "transport.enableTraceEvent",
//     ;,
//     TransportEnableTraceEventResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ProducerCloseRequest,
//     "producer.close",
//     ;,
//     ProducerCloseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ProducerDumpRequest,
//     "producer.dump",
//     ;,
//     ProducerDumpResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ProducerGetStatsRequest,
//     "producer.getStats",
//     ;,
//     ProducerGetStatsResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ProducerPauseRequest,
//     "producer.pause",
//     ;,
//     ProducerPauseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ProducerResumeRequest,
//     "producer.resume",
//     ;,
//     ProducerResumeResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ProducerEnableTraceEventRequest,
//     "producer.enableTraceEvent",
//     ;,
//     ProducerEnableTraceEventResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerCloseRequest,
//     "consumer.close",
//     ;,
//     ConsumerCloseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerDumpRequest,
//     "consumer.dump",
//     ;,
//     ConsumerDumpResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerGetStatsRequest,
//     "consumer.getStats",
//     ;,
//     ConsumerGetStatsResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerPauseRequest,
//     "consumer.pause",
//     ;,
//     ConsumerPauseResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerResumeRequest,
//     "consumer.resume",
//     ;,
//     ConsumerResumeResponse,
//     {
//         // TODO
//     },
// );
//
// request_response!(
//     ConsumerSetPreferredLayersRequest,
//     "consumer.setPreferredLayers",
//     ;,
//     ConsumerSetPreferredLayersResponse,
//     {
//         // TODO
//     },
// );
//
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
