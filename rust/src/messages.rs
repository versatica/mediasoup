use crate::data_structures::*;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;
use uuid::Uuid;

pub(crate) trait Request: Debug + Serialize {
    type Response: DeserializeOwned;

    fn as_method(&self) -> &'static str;
}

macro_rules! request_response {
    (
        $request_name: ident,
        $response_name: ident,
        $method: literal,
        $request_impl: tt,
        $response_impl: tt,
    ) => {
        #[derive(Debug, Serialize)]
        pub struct $request_name $request_impl

        #[derive(Debug, Deserialize)]
        #[serde(rename_all = "camelCase")]
        pub struct $response_name $response_impl

        impl Request for $request_name {
            type Response = $response_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
    (
        $request_name: ident,
        $response_name: ident,
        $method: literal,
        $request_impl: tt,
    ) => {
        #[derive(Debug, Serialize)]
        pub struct $request_name $request_impl

        impl Request for $request_name {
            type Response = $response_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
}

request_response!(
    WorkerDumpRequest,
    WorkerDumpResponse,
    "worker.dump",
    ;,
    {
        pub pid: u32,
        pub router_ids: Vec<Uuid>,
    },
);

request_response!(
    WorkerGetResourceRequest,
    WorkerResourceUsage,
    "worker.getResourceUsage",
    ;,
);

request_response!(
    WorkerUpdateSettingsRequest,
    WorkerUpdateSettingsResponse,
    "worker.updateSettings",
    {
        pub(crate) data: WorkerUpdateSettingsData,
    },
    {
        // TODO
    },
);

request_response!(
    WorkerCreateRouterRequest,
    WorkerCreateRouterResponse,
    "worker.createRouter",
    {
        pub(crate) internal: RouterInternal,
    },
    {
        // TODO
    },
);

request_response!(
    RouterCloseRequest,
    RouterCloseResponse,
    "router.close",
    {
        pub(crate) internal: RouterInternal,
    },
    {
        // TODO
    },
);

request_response!(
    RouterDumpRequest,
    RouterDumpResponse,
    "router.dump",
    {
        pub(crate) internal: RouterInternal,
    },
    {
        // TODO
    },
);

request_response!(
    RouterCreateWebrtcTransportRequest,
    RouterCreateWebrtcTransportResponse,
    "router.createWebRtcTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreateWebrtcTransportData,
    },
    {
        // TODO
    },
);

request_response!(
    RouterCreatePlainTransportRequest,
    RouterCreatePlainTransportResponse,
    "router.createPlainTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreatePlainTransportData,
    },
    {
        // TODO
    },
);

request_response!(
    RouterCreatePipeTransportRequest,
    RouterCreatePipeTransportResponse,
    "router.createPipeTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreatePipeTransportData,
    },
    {
        // TODO
    },
);

request_response!(
    RouterCreateDirectTransportRequest,
    RouterCreateDirectTransportResponse,
    "router.createDirectTransport",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: RouterCreateDirectTransportData,
    },
    {
        // TODO
    },
);

request_response!(
    RouterCreateAudioLevelObserverRequest,
    RouterCreateAudioLevelObserverResponse,
    "router.createAudioLevelObserver",
    {
        pub(crate) internal: RouterCreateAudioLevelObserverInternal,
        pub(crate) data: RouterCreateAudioLevelObserverData,
    },
    {
        // TODO
    },
);

request_response!(
    TransportCloseRequest,
    TransportCloseResponse,
    "transport.close",
    {
        pub(crate) internal: TransportInternal,
    },
    {
        // TODO
    },
);

request_response!(
    TransportDumpRequest,
    TransportDumpResponse,
    "transport.dump",
    {
        pub(crate) internal: TransportInternal,
    },
    {
        // TODO
    },
);

request_response!(
    TransportGetStatsRequest,
    TransportGetStatsResponse,
    "transport.getStats",
    {
        pub(crate) internal: TransportInternal,
    },
    {
        // TODO
    },
);

request_response!(
    TransportConnectRequest,
    TransportConnectResponse,
    "transport.connect",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: TransportConnectData,
    },
    {
        // TODO
    },
);

request_response!(
    TransportSetMaxIncomingBitrateRequest,
    TransportSetMaxIncomingBitrateResponse,
    "transport.setMaxIncomingBitrate",
    {
        pub(crate) internal: TransportInternal,
        pub(crate) data: TransportSetMaxIncomingBitrateData,
    },
    {
        // TODO
    },
);

// TODO: Detail remaining methods, I got bored for now
request_response!(
    TransportRestartIceRequest,
    TransportRestartIceResponse,
    "transport.restartIce",
    {
        pub(crate) internal: TransportInternal,
    },
    {
        // TODO
    },
);

request_response!(
    TransportProduceRequest,
    TransportProduceResponse,
    "transport.produce",
    ;,
    {
        // TODO
    },
);

request_response!(
    TransportConsumeRequest,
    TransportConsumeResponse,
    "transport.consume",
    ;,
    {
        // TODO
    },
);

request_response!(
    TransportProduceDataRequest,
    TransportProduceDataResponse,
    "transport.produceData",
    ;,
    {
        // TODO
    },
);

request_response!(
    TransportConsumeDataRequest,
    TransportConsumeDataResponse,
    "transport.consumeData",
    ;,
    {
        // TODO
    },
);

request_response!(
    TransportEnableTraceEventRequest,
    TransportEnableTraceEventResponse,
    "transport.enableTraceEvent",
    ;,
    {
        // TODO
    },
);

request_response!(
    ProducerCloseRequest,
    ProducerCloseResponse,
    "producer.close",
    ;,
    {
        // TODO
    },
);

request_response!(
    ProducerDumpRequest,
    ProducerDumpResponse,
    "producer.dump",
    ;,
    {
        // TODO
    },
);

request_response!(
    ProducerGetStatsRequest,
    ProducerGetStatsResponse,
    "producer.getStats",
    ;,
    {
        // TODO
    },
);

request_response!(
    ProducerPauseRequest,
    ProducerPauseResponse,
    "producer.pause",
    ;,
    {
        // TODO
    },
);

request_response!(
    ProducerResumeRequest,
    ProducerResumeResponse,
    "producer.resume",
    ;,
    {
        // TODO
    },
);

request_response!(
    ProducerEnableTraceEventRequest,
    ProducerEnableTraceEventResponse,
    "producer.enableTraceEvent",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerCloseRequest,
    ConsumerCloseResponse,
    "consumer.close",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerDumpRequest,
    ConsumerDumpResponse,
    "consumer.dump",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerGetStatsRequest,
    ConsumerGetStatsResponse,
    "consumer.getStats",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerPauseRequest,
    ConsumerPauseResponse,
    "consumer.pause",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerResumeRequest,
    ConsumerResumeResponse,
    "consumer.resume",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerSetPreferredLayersRequest,
    ConsumerSetPreferredLayersResponse,
    "consumer.setPreferredLayers",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerSetPriorityRequest,
    ConsumerSetPriorityResponse,
    "consumer.setPriority",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerRequestKeyFrameRequest,
    ConsumerRequestKeyFrameResponse,
    "consumer.requestKeyFrame",
    ;,
    {
        // TODO
    },
);

request_response!(
    ConsumerEnableTraceEventRequest,
    ConsumerEnableTraceEventResponse,
    "consumer.enableTraceEvent",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataProducerCloseRequest,
    DataProducerCloseResponse,
    "dataProducer.close",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataProducerDumpRequest,
    DataProducerDumpResponse,
    "dataProducer.dump",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataProducerGetStatsRequest,
    DataProducerGetStatsResponse,
    "dataProducer.getStats",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataConsumerCloseRequest,
    DataConsumerCloseResponse,
    "dataConsumer.close",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataConsumerDumpRequest,
    DataConsumerDumpResponse,
    "dataConsumer.dump",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataConsumerGetStatsRequest,
    DataConsumerGetStatsResponse,
    "dataConsumer.getStats",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataConsumerGetBufferedAmountRequest,
    DataConsumerGetBufferedAmountResponse,
    "dataConsumer.getBufferedAmount",
    ;,
    {
        // TODO
    },
);

request_response!(
    DataConsumerSetBufferedAmountLowThresholdRequest,
    DataConsumerSetBufferedAmountLowThresholdResponse,
    "dataConsumer.setBufferedAmountLowThreshold",
    ;,
    {
        // TODO
    },
);

request_response!(
    RtpObserverCloseRequest,
    RtpObserverCloseResponse,
    "rtpObserver.close",
    ;,
    {
        // TODO
    },
);

request_response!(
    RtpObserverPauseRequest,
    RtpObserverPauseResponse,
    "rtpObserver.pause",
    ;,
    {
        // TODO
    },
);

request_response!(
    RtpObserverResumeRequest,
    RtpObserverResumeResponse,
    "rtpObserver.resume",
    ;,
    {
        // TODO
    },
);

request_response!(
    RtpObserverAddProducerRequest,
    RtpObserverAddProducerResponse,
    "rtpObserver.addProducer",
    ;,
    {
        // TODO
    },
);

request_response!(
    RtpObserverRemoveProducerRequest,
    RtpObserverRemoveProducerResponse,
    "rtpObserver.removeProducer",
    ;,
    {
        // TODO
    },
);
