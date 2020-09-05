use crate::data_structures::{
    AppData, ConsumerId, ObserverId, ProducerId, RouterId, RouterInternal, TransportId,
};
use crate::messages::{RouterCloseRequest, RouterDumpRequest};
use crate::ortc::RtpCapabilities;
use crate::worker::{Channel, RequestError};
use async_executor::Executor;
use log::debug;
use log::error;
use serde::Deserialize;
use std::collections::{HashMap, HashSet};
use std::mem;
use std::sync::{Arc, Mutex};

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RouterDumpResponse {
    id: RouterId,
    map_consumer_id_producer_id: HashMap<ConsumerId, ProducerId>,
    map_data_consumer_id_data_producer_id: HashMap<ConsumerId, ProducerId>,
    map_data_producer_id_data_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    map_producer_id_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    map_producer_id_observer_ids: HashMap<ProducerId, HashSet<ObserverId>>,
    rtp_observer_ids: HashSet<ObserverId>,
    transport_ids: HashSet<TransportId>,
}

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce()>>>,
}

pub struct Router {
    id: RouterId,
    executor: Arc<Executor>,
    rtp_capabilities: RtpCapabilities,
    channel: Channel,
    payload_channel: Channel,
    handlers: Handlers,
    app_data: AppData,
}

impl Drop for Router {
    fn drop(&mut self) {
        debug!("drop()");

        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }

        {
            let channel = self.channel.clone();
            let request = RouterCloseRequest {
                internal: RouterInternal { router_id: self.id },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("router closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

impl Router {
    // TODO: Ideally we'd want `pub(in super::worker)`, but it doesn't work
    pub(super) fn new(
        id: RouterId,
        executor: Arc<Executor>,
        rtp_capabilities: RtpCapabilities,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
    ) -> Self {
        debug!("new()");

        let handlers = Handlers::default();
        Self {
            id,
            executor,
            rtp_capabilities,
            channel,
            payload_channel,
            handlers,
            app_data,
        }
    }

    /// Router id.
    pub fn id(&self) -> RouterId {
        self.id
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.app_data
    }

    /// RTC capabilities of the Router.
    pub fn rtp_capabilities(&self) -> RtpCapabilities {
        self.rtp_capabilities.clone()
    }

    /// Dump Router.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<RouterDumpResponse, RequestError> {
        debug!("dump()");

        self.channel
            .request(RouterDumpRequest {
                internal: RouterInternal { router_id: self.id },
            })
            .await
    }

    pub fn connect_closed<F: FnOnce() + 'static>(&self, callback: F) {
        self.handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
