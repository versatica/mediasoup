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

pub struct DataConsumerOptions {
    // TODO
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
