use crate::data_producer::DataProducerId;
use crate::data_structures::AppData;
use crate::messages::{DataConsumerCloseRequest, DataConsumerInternal};
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{Channel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use log::*;
use serde::{Deserialize, Serialize};
use std::mem;
use std::sync::{Arc, Mutex};

uuid_based_wrapper_type!(DataConsumerId);

// TODO: Split into 2 for Direct and others or make an enum
#[derive(Debug)]
pub struct DataConsumerOptions {
    // The id of the DataProducer to consume.
    pub(crate) data_producer_id: DataProducerId,
    /// Just if consuming over SCTP.
    /// Whether data messages must be received in order. If true the messages will be sent reliably.
    /// Defaults to the value in the DataProducer if it has type 'Sctp' or to true if it has type
    /// 'Direct'.
    pub(crate) ordered: Option<bool>,
    /// Just if consuming over SCTP.
    /// When ordered is false indicates the time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    /// Defaults to the value in the DataProducer if it has type 'Sctp' or unset if it has type
    /// 'Direct'.
    pub(crate) max_packet_life_time: Option<u16>,
    /// Just if consuming over SCTP.
    /// When ordered is false indicates the maximum number of times a packet will be retransmitted.
    /// Defaults to the value in the DataProducer if it has type 'Sctp' or unset if it has type
    /// 'Direct'.
    pub(crate) max_retransmits: Option<u16>,
    /// Custom application data.
    pub app_data: AppData,
}

impl DataConsumerOptions {
    /// Inherits parameters of corresponding DataProducer.
    pub fn new_sctp(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// For DirectTransport.
    pub fn new_direct(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: Some(true),
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// Messages will be sent reliably in order.
    pub fn new_sctp_ordered(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// Messages will be sent unreliably with time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    pub fn new_sctp_unordered_with_life_time(
        data_producer_id: DataProducerId,
        max_packet_life_time: u16,
    ) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: Some(max_packet_life_time),
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// Messages will be sent unreliably with a limited number of retransmission attempts.
    pub fn new_sctp_unordered_with_retransmits(
        data_producer_id: DataProducerId,
        max_retransmits: u16,
    ) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: Some(max_retransmits),
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum DataConsumerType {
    Sctp,
    Direct,
}

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    id: DataConsumerId,
    data_producer_id: DataProducerId,
    // r#type: DataConsumerType,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Box<dyn Transport>,
    // Drop subscription to consumer-specific notifications when consumer itself is dropped
    _subscription_handler: SubscriptionHandler,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }

        {
            let channel = self.channel.clone();
            let request = DataConsumerCloseRequest {
                internal: DataConsumerInternal {
                    router_id: self.transport.router_id(),
                    transport_id: self.transport.id(),
                    data_consumer_id: self.id,
                    data_producer_id: self.data_producer_id,
                },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("consumer closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct DataConsumer {
    inner: Arc<Inner>,
}

impl DataConsumer {
    pub(super) async fn new(
        id: DataConsumerId,
        data_producer_id: DataProducerId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
        transport: Box<dyn Transport>,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    // match serde_json::from_value::<Notification>(notification) {
                    //     Ok(notification) => match notification {
                    //         Notification::DataProducerClose => {
                    //             // TODO: Handle this in some meaningful way
                    //         }
                    //         Notification::DataProducerPause => {
                    //             let mut producer_paused = producer_paused.lock().unwrap();
                    //             let was_paused = *paused.lock().unwrap() || *producer_paused;
                    //             *producer_paused = true;
                    //
                    //             if !was_paused {
                    //                 for callback in handlers.pause.lock().unwrap().iter() {
                    //                     callback();
                    //                 }
                    //             }
                    //         }
                    //         Notification::DataProducerResume => {
                    //             let mut producer_paused = producer_paused.lock().unwrap();
                    //             let paused = *paused.lock().unwrap();
                    //             let was_paused = paused || *producer_paused;
                    //             *producer_paused = false;
                    //
                    //             if was_paused && !paused {
                    //                 for callback in handlers.resume.lock().unwrap().iter() {
                    //                     callback();
                    //                 }
                    //             }
                    //         }
                    //         Notification::Score(consumer_score) => {
                    //             *score.lock().unwrap() = consumer_score.clone();
                    //             for callback in handlers.score.lock().unwrap().iter() {
                    //                 callback(&consumer_score);
                    //             }
                    //         }
                    //         Notification::LayersChange(consumer_layers) => {
                    //             *current_layers.lock().unwrap() = Some(consumer_layers.clone());
                    //             for callback in handlers.layers_change.lock().unwrap().iter() {
                    //                 callback(&consumer_layers);
                    //             }
                    //         }
                    //         Notification::Trace(trace_event_data) => {
                    //             for callback in handlers.trace.lock().unwrap().iter() {
                    //                 callback(&trace_event_data);
                    //             }
                    //         }
                    //     },
                    //     Err(error) => {
                    //         error!("Failed to parse notification: {}", error);
                    //     }
                    // }
                })
                .await
                .unwrap()
        };
        // TODO: payload_channel subscription for direct transport

        let inner = Arc::new(Inner {
            id,
            data_producer_id,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
            _subscription_handler: subscription_handler,
        });

        Self { inner }
    }

    /// DataConsumer id.
    pub fn id(&self) -> DataConsumerId {
        self.inner.id
    }

    /// Associated DataProducer id.
    pub fn data_producer_id(&self) -> DataProducerId {
        self.inner.data_producer_id
    }

    // /// DataConsumer type.
    // pub fn r#type(&self) -> DataConsumerType {
    //     self.inner.r#type
    // }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    // /// Dump DataConsumer.
    // #[doc(hidden)]
    // pub async fn dump(&self) -> Result<DataConsumerDump, RequestError> {
    //     debug!("dump()");
    //
    //     self.inner
    //         .channel
    //         .request(DataConsumerDumpRequest {
    //             internal: self.get_internal(),
    //         })
    //         .await
    // }
    //
    // /// Get DataConsumer stats.
    // pub async fn get_stats(&self) -> Result<DataConsumerStats, RequestError> {
    //     debug!("get_stats()");
    //
    //     self.inner
    //         .channel
    //         .request(DataConsumerGetStatsRequest {
    //             internal: self.get_internal(),
    //         })
    //         .await
    // }

    pub fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    fn get_internal(&self) -> DataConsumerInternal {
        DataConsumerInternal {
            router_id: self.inner.transport.router_id(),
            transport_id: self.inner.transport.id(),
            data_consumer_id: self.inner.id,
            data_producer_id: self.inner.data_producer_id,
        }
    }
}
