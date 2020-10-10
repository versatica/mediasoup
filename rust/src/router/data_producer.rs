use crate::data_structures::AppData;
use crate::messages::{
    DataProducerCloseRequest, DataProducerInternal, ProducerEnableTraceEventRequest,
    ProducerEnableTraceEventRequestData, ProducerGetStatsRequest, ProducerPauseRequest,
    ProducerResumeRequest,
};
use crate::sctp_parameters::SctpStreamParameters;
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{Channel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::mem;
use std::sync::{Arc, Mutex, Weak};

uuid_based_wrapper_type!(DataProducerId);

#[derive(Debug)]
pub struct DataProducerOptions {
    /// DataProducer id (just for Router.pipeToRouter() method).
    /// DataProducer id, should most likely not be specified explicitly, specified by pipe transport
    pub(super) id: Option<DataProducerId>,
    /// SCTP parameters defining how the endpoint is sending the data.
    /// Required if SCTP/DataChannel is used.
    /// Must not be given if the data producer is created on a DirectTransport.
    pub sctp_stream_parameters: Option<SctpStreamParameters>,
    /// A label which can be used to distinguish this DataChannel from others.
    pub label: String,
    // TODO: Should probably not be a string here, maybe not even optional
    /// Name of the sub-protocol used by this DataChannel.
    pub protocol: String,
    /// Custom application data.
    pub app_data: AppData,
}

impl DataProducerOptions {
    pub fn new() -> Self {
        Self {
            id: None,
            sctp_stream_parameters: None,
            label: "".to_string(),
            protocol: "".to_string(),
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum DataProducerType {
    Sctp,
    Direct,
}

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    id: DataProducerId,
    r#type: DataProducerType,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Box<dyn Transport>,
    // Drop subscription to producer-specific notifications when producer itself is dropped
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
            let request = DataProducerCloseRequest {
                internal: DataProducerInternal {
                    router_id: self.transport.router_id(),
                    transport_id: self.transport.id(),
                    data_producer_id: self.id,
                },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("data producer closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct DataProducer {
    inner: Arc<Inner>,
}

impl DataProducer {
    pub(super) async fn new(
        id: DataProducerId,
        r#type: DataProducerType,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
        transport: Box<dyn Transport>,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let subscription_handler = {
            //     let handlers = Arc::clone(&handlers);
            //     let score = Arc::clone(&score);
            //
            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    //             match serde_json::from_value::<Notification>(notification) {
                    //                 Ok(notification) => match notification {
                    //                     Notification::Score(scores) => {
                    //                         *score.lock().unwrap() = scores.clone();
                    //                         for callback in handlers.score.lock().unwrap().iter() {
                    //                             callback(&scores);
                    //                         }
                    //                     }
                    //                     Notification::VideoOrientationChange(video_orientation) => {
                    //                         for callback in
                    //                             handlers.video_orientation_change.lock().unwrap().iter()
                    //                         {
                    //                             callback(video_orientation);
                    //                         }
                    //                     }
                    //                     Notification::Trace(trace_event_data) => {
                    //                         for callback in handlers.trace.lock().unwrap().iter() {
                    //                             callback(&trace_event_data);
                    //                         }
                    //                     }
                    //                 },
                    //                 Err(error) => {
                    //                     error!("Failed to parse notification: {}", error);
                    //                 }
                    //             }
                })
                .await
                .unwrap()
        };

        let inner = Arc::new(Inner {
            id,
            r#type,
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

    /// DataProducer id.
    pub fn id(&self) -> DataProducerId {
        self.inner.id
    }

    /// DataProducer type.
    pub fn r#type(&self) -> DataProducerType {
        self.inner.r#type
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    // /// Dump DataProducer.
    // #[doc(hidden)]
    // pub async fn dump(&self) -> Result<DataProducerDump, RequestError> {
    //     debug!("dump()");
    //
    //     self.inner
    //         .channel
    //         .request(DataProducerDumpRequest {
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

    pub(super) fn downgrade(&self) -> WeakDataProducer {
        WeakDataProducer {
            inner: Arc::downgrade(&self.inner),
        }
    }

    fn get_internal(&self) -> DataProducerInternal {
        DataProducerInternal {
            router_id: self.inner.transport.router_id(),
            transport_id: self.inner.transport.id(),
            data_producer_id: self.inner.id,
        }
    }
}

#[derive(Clone)]
pub(super) struct WeakDataProducer {
    inner: Weak<Inner>,
}

impl WeakDataProducer {
    pub(super) fn upgrade(&self) -> Option<DataProducer> {
        Some(DataProducer {
            inner: self.inner.upgrade()?,
        })
    }
}
