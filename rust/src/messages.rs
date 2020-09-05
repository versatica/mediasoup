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
        $request_name: ident,
        $method: literal,
        $request_impl: tt,
        $response_name: ident,
        $response_impl: tt,
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_name $request_impl

        #[derive(Debug, Deserialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $response_name $response_impl

        impl Request for $request_name {
            type Response = $response_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
    (
        $request_name: ident,
        $method: literal,
        $request_impl: tt,
        $response_name: ident,
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_name $request_impl

        impl Request for $request_name {
            type Response = $response_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
    (
        $request_name: ident,
        $method: literal,
        $request_impl: tt,
        $response_impl: ty,
    ) => {
        #[derive(Debug, Serialize)]
        pub(crate) struct $request_name $request_impl

        impl Request for $request_name {
            type Response = $response_impl;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
}

request_response!(
    WorkerDumpRequest,
    "worker.dump",
    ;,
    WorkerDump,
);

request_response!(
    WorkerGetResourceRequest,
    "worker.getResourceUsage",
    ;,
    WorkerResourceUsage,
);

request_response!(
    WorkerUpdateSettingsRequest,
    "worker.updateSettings",
    {
        pub(crate) data: WorkerUpdateSettings,
    },
    (),
);

request_response!(
    WorkerCreateRouterRequest,
    "worker.createRouter",
    {
        pub(crate) internal: RouterInternal,
    },
    (),
);

request_response!(
    RouterCloseRequest,
    "router.close",
    {
        pub(crate) internal: RouterInternal,
    },
    RouterCloseResponse,
    {
        // TODO
    },
);

request_response!(
    RouterDumpRequest,
    "router.dump",
    {
        pub(crate) internal: RouterInternal,
    },
    RouterDumpResponse,
    {
        // TODO
    },
);

request_response!(
    RouterCreateWebrtcTransportRequest,
    "router.createWebRtcTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreateWebrtcTransportData,
    },
    RouterCreateWebrtcTransportResponse,
    {
        // TODO
    },
);

request_response!(
    RouterCreatePlainTransportRequest,
    "router.createPlainTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreatePlainTransportData,
    },
    RouterCreatePlainTransportResponse,
    {
        // TODO
    },
);

request_response!(
    RouterCreatePipeTransportRequest,
    "router.createPipeTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreatePipeTransportData,
    },
    RouterCreatePipeTransportResponse,
    {
        // TODO
    },
);

request_response!(
    RouterCreateDirectTransportRequest,
    "router.createDirectTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreateDirectTransportData,
    },
    RouterCreateDirectTransportResponse,
    {
        // TODO
    },
);

request_response!(
    RouterCreateAudioLevelObserverRequest,
    "router.createAudioLevelObserver",
    {
        pub(crate) internal: RouterCreateAudioLevelObserverInternal,
        pub(crate) data: RouterCreateAudioLevelObserverData,
    },
    RouterCreateAudioLevelObserverResponse,
    {
        // TODO
    },
);

request_response!(
    TransportCloseRequest,
    "transport.close",
    {
        pub(crate) internal: TransportInternal,
    },
    TransportCloseResponse,
    {
        // TODO
    },
);

request_response!(
    TransportDumpRequest,
    "transport.dump",
    {
        pub(crate) internal: TransportInternal,
    },
    TransportDumpResponse,
    {
        // TODO
    },
);

request_response!(
    TransportGetStatsRequest,
    "transport.getStats",
    {
        pub(crate) internal: TransportInternal,
    },
    TransportGetStatsResponse,
    {
        // TODO
    },
);

request_response!(
    TransportConnectRequest,
    "transport.connect",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: TransportConnectData,
    },
    TransportConnectResponse,
    {
        // TODO
    },
);

request_response!(
    TransportSetMaxIncomingBitrateRequest,
    "transport.setMaxIncomingBitrate",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: TransportSetMaxIncomingBitrateData,
    },
    TransportSetMaxIncomingBitrateResponse,
    {
        // TODO
    },
);

// TODO: Detail remaining methods, I got bored for now
request_response!(
    TransportRestartIceRequest,
    "transport.restartIce",
    {
        pub(crate) internal: TransportInternal,
    },
    TransportRestartIceResponse,
    {
        // TODO
    },
);

request_response!(
    TransportProduceRequest,
    "transport.produce",
    ;,
    TransportProduceResponse,
    {
        // TODO
    },
);

request_response!(
    TransportConsumeRequest,
    "transport.consume",
    ;,
    TransportConsumeResponse,
    {
        // TODO
    },
);

request_response!(
    TransportProduceDataRequest,
    "transport.produceData",
    ;,
    TransportProduceDataResponse,
    {
        // TODO
    },
);

request_response!(
    TransportConsumeDataRequest,
    "transport.consumeData",
    ;,
    TransportConsumeDataResponse,
    {
        // TODO
    },
);

request_response!(
    TransportEnableTraceEventRequest,
    "transport.enableTraceEvent",
    ;,
    TransportEnableTraceEventResponse,
    {
        // TODO
    },
);

request_response!(
    ProducerCloseRequest,
    "producer.close",
    ;,
    ProducerCloseResponse,
    {
        // TODO
    },
);

request_response!(
    ProducerDumpRequest,
    "producer.dump",
    ;,
    ProducerDumpResponse,
    {
        // TODO
    },
);

request_response!(
    ProducerGetStatsRequest,
    "producer.getStats",
    ;,
    ProducerGetStatsResponse,
    {
        // TODO
    },
);

request_response!(
    ProducerPauseRequest,
    "producer.pause",
    ;,
    ProducerPauseResponse,
    {
        // TODO
    },
);

request_response!(
    ProducerResumeRequest,
    "producer.resume",
    ;,
    ProducerResumeResponse,
    {
        // TODO
    },
);

request_response!(
    ProducerEnableTraceEventRequest,
    "producer.enableTraceEvent",
    ;,
    ProducerEnableTraceEventResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerCloseRequest,
    "consumer.close",
    ;,
    ConsumerCloseResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerDumpRequest,
    "consumer.dump",
    ;,
    ConsumerDumpResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerGetStatsRequest,
    "consumer.getStats",
    ;,
    ConsumerGetStatsResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerPauseRequest,
    "consumer.pause",
    ;,
    ConsumerPauseResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerResumeRequest,
    "consumer.resume",
    ;,
    ConsumerResumeResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerSetPreferredLayersRequest,
    "consumer.setPreferredLayers",
    ;,
    ConsumerSetPreferredLayersResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerSetPriorityRequest,
    "consumer.setPriority",
    ;,
    ConsumerSetPriorityResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerRequestKeyFrameRequest,
    "consumer.requestKeyFrame",
    ;,
    ConsumerRequestKeyFrameResponse,
    {
        // TODO
    },
);

request_response!(
    ConsumerEnableTraceEventRequest,
    "consumer.enableTraceEvent",
    ;,
    ConsumerEnableTraceEventResponse,
    {
        // TODO
    },
);

request_response!(
    DataProducerCloseRequest,
    "dataProducer.close",
    ;,
    DataProducerCloseResponse,
    {
        // TODO
    },
);

request_response!(
    DataProducerDumpRequest,
    "dataProducer.dump",
    ;,
    DataProducerDumpResponse,
    {
        // TODO
    },
);

request_response!(
    DataProducerGetStatsRequest,
    "dataProducer.getStats",
    ;,
    DataProducerGetStatsResponse,
    {
        // TODO
    },
);

request_response!(
    DataConsumerCloseRequest,
    "dataConsumer.close",
    ;,
    DataConsumerCloseResponse,
    {
        // TODO
    },
);

request_response!(
    DataConsumerDumpRequest,
    "dataConsumer.dump",
    ;,
    DataConsumerDumpResponse,
    {
        // TODO
    },
);

request_response!(
    DataConsumerGetStatsRequest,
    "dataConsumer.getStats",
    ;,
    DataConsumerGetStatsResponse,
    {
        // TODO
    },
);

request_response!(
    DataConsumerGetBufferedAmountRequest,
    "dataConsumer.getBufferedAmount",
    ;,
    DataConsumerGetBufferedAmountResponse,
    {
        // TODO
    },
);

request_response!(
    DataConsumerSetBufferedAmountLowThresholdRequest,
    "dataConsumer.setBufferedAmountLowThreshold",
    ;,
    DataConsumerSetBufferedAmountLowThresholdResponse,
    {
        // TODO
    },
);

request_response!(
    RtpObserverCloseRequest,
    "rtpObserver.close",
    ;,
    RtpObserverCloseResponse,
    {
        // TODO
    },
);

request_response!(
    RtpObserverPauseRequest,
    "rtpObserver.pause",
    ;,
    RtpObserverPauseResponse,
    {
        // TODO
    },
);

request_response!(
    RtpObserverResumeRequest,
    "rtpObserver.resume",
    ;,
    RtpObserverResumeResponse,
    {
        // TODO
    },
);

request_response!(
    RtpObserverAddProducerRequest,
    "rtpObserver.addProducer",
    ;,
    RtpObserverAddProducerResponse,
    {
        // TODO
    },
);

request_response!(
    RtpObserverRemoveProducerRequest,
    "rtpObserver.removeProducer",
    ;,
    RtpObserverRemoveProducerResponse,
    {
        // TODO
    },
);
