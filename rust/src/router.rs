#[cfg(not(doc))]
pub mod consumer;
#[cfg(not(doc))]
pub mod data_consumer;
#[cfg(not(doc))]
pub mod data_producer;
#[cfg(not(doc))]
pub mod observer;
#[cfg(not(doc))]
pub mod plain_transport;
#[cfg(not(doc))]
pub mod producer;
#[cfg(not(doc))]
pub mod transport;
#[cfg(not(doc))]
pub mod webrtc_transport;

use crate::{ortc, uuid_based_wrapper_type};

use crate::consumer::ConsumerId;
use crate::data_producer::{DataProducer, DataProducerId, WeakDataProducer};
use crate::data_structures::AppData;
use crate::messages::{
    RouterCloseRequest, RouterCreatePlainTransportData, RouterCreatePlainTransportRequest,
    RouterCreateWebrtcTransportData, RouterCreateWebrtcTransportRequest, RouterDumpRequest,
    RouterInternal, TransportInternal,
};
use crate::observer::ObserverId;
use crate::plain_transport::{PlainTransport, PlainTransportOptions};
use crate::producer::{Producer, ProducerId, WeakProducer};
use crate::rtp_parameters::{RtpCapabilities, RtpCapabilitiesFinalized, RtpCodecCapability};
use crate::transport::{TransportGeneric, TransportId};
use crate::webrtc_transport::{WebRtcTransport, WebRtcTransportOptions};
use crate::worker::{Channel, RequestError, Worker};
use async_executor::Executor;
use event_listener_primitives::{Bag, HandlerId};
use log::*;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex};

uuid_based_wrapper_type!(RouterId);

#[derive(Debug, Default)]
pub struct RouterOptions {
    pub media_codecs: Vec<RtpCodecCapability>,
    pub app_data: AppData,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RouterDump {
    pub id: RouterId,
    pub map_consumer_id_producer_id: HashMap<ConsumerId, ProducerId>,
    pub map_data_consumer_id_data_producer_id: HashMap<ConsumerId, ProducerId>,
    pub map_data_producer_id_data_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    pub map_producer_id_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    pub map_producer_id_observer_ids: HashMap<ProducerId, HashSet<ObserverId>>,
    pub rtp_observer_ids: HashSet<ObserverId>,
    pub transport_ids: HashSet<TransportId>,
}

pub enum NewTransport<'a> {
    WebRtc(&'a WebRtcTransport),
    Plain(&'a PlainTransport),
}

#[derive(Default)]
struct Handlers {
    new_transport: Bag<'static, dyn Fn(NewTransport) + Send>,
    closed: Bag<'static, dyn FnOnce() + Send>,
}

struct Inner {
    id: RouterId,
    executor: Arc<Executor<'static>>,
    rtp_capabilities: RtpCapabilitiesFinalized,
    channel: Channel,
    payload_channel: Channel,
    handlers: Handlers,
    app_data: AppData,
    producers: Arc<Mutex<HashMap<ProducerId, WeakProducer>>>,
    data_producers: Arc<Mutex<HashMap<DataProducerId, WeakDataProducer>>>,
    // Make sure worker is not dropped until this router is not dropped
    _worker: Worker,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.handlers.closed.call_once_simple();

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

#[derive(Clone)]
pub struct Router {
    inner: Arc<Inner>,
}

impl Router {
    pub(super) fn new(
        id: RouterId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: Channel,
        rtp_capabilities: RtpCapabilitiesFinalized,
        app_data: AppData,
        worker: Worker,
    ) -> Self {
        debug!("new()");

        let producers = Arc::<Mutex<HashMap<ProducerId, WeakProducer>>>::default();
        let data_producers = Arc::<Mutex<HashMap<DataProducerId, WeakDataProducer>>>::default();
        let handlers = Handlers::default();
        let inner = Arc::new(Inner {
            id,
            executor,
            rtp_capabilities,
            channel,
            payload_channel,
            handlers,
            producers,
            data_producers,
            app_data,
            _worker: worker,
        });

        Self { inner }
    }

    /// Router id.
    pub fn id(&self) -> RouterId {
        self.inner.id
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// RTC capabilities of the Router.
    pub fn rtp_capabilities(&self) -> &RtpCapabilitiesFinalized {
        &self.inner.rtp_capabilities
    }

    /// Dump Router.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<RouterDump, RequestError> {
        debug!("dump()");

        self.inner
            .channel
            .request(RouterDumpRequest {
                internal: RouterInternal {
                    router_id: self.inner.id,
                },
            })
            .await
    }

    /// Create a WebRtcTransport.
    ///
    /// Router will be kept alive as long as at least one transport instance is alive.
    pub async fn create_webrtc_transport(
        &self,
        webrtc_transport_options: WebRtcTransportOptions,
    ) -> Result<WebRtcTransport, RequestError> {
        debug!("create_webrtc_transport()");

        let transport_id = TransportId::new();
        let data = self
            .inner
            .channel
            .request(RouterCreateWebrtcTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreateWebrtcTransportData::from_options(&webrtc_transport_options),
            })
            .await?;

        let transport_fut = WebRtcTransport::new(
            transport_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            data,
            webrtc_transport_options.app_data,
            self.clone(),
        );
        let transport = transport_fut.await;

        self.inner.handlers.new_transport.call(|callback| {
            callback(NewTransport::WebRtc(&transport));
        });

        self.after_transport_creation(&transport);

        Ok(transport)
    }

    /// Create a PlainTransport.
    ///
    /// Router will be kept alive as long as at least one transport instance is alive.
    pub async fn create_plain_transport(
        &self,
        plain_transport_options: PlainTransportOptions,
    ) -> Result<PlainTransport, RequestError> {
        debug!("create_plain_transport()");

        let transport_id = TransportId::new();
        let data = self
            .inner
            .channel
            .request(RouterCreatePlainTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreatePlainTransportData::from_options(&plain_transport_options),
            })
            .await?;

        let transport_fut = PlainTransport::new(
            transport_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            data,
            plain_transport_options.app_data,
            self.clone(),
        );
        let transport = transport_fut.await;

        self.inner.handlers.new_transport.call(|callback| {
            callback(NewTransport::Plain(&transport));
        });

        self.after_transport_creation(&transport);

        Ok(transport)
    }

    /// Check whether the given RTP capabilities can consume the given Producer.
    pub fn can_consume(
        &self,
        producer_id: &ProducerId,
        rtp_capabilities: &RtpCapabilities,
    ) -> bool {
        match self.get_producer(producer_id) {
            Some(producer) => {
                match ortc::can_consume(producer.consumable_rtp_parameters(), rtp_capabilities) {
                    Ok(result) => result,
                    Err(error) => {
                        error!("can_consume() | unexpected error: {}", error);
                        false
                    }
                }
            }
            None => {
                error!(
                    "can_consume() | Producer with id \"{}\" not found",
                    producer_id
                );
                false
            }
        }
    }

    pub fn on_new_transport<F: Fn(NewTransport) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.new_transport.add(Box::new(callback))
    }

    pub fn on_closed<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.closed.add(Box::new(callback))
    }

    fn after_transport_creation<Dump, Stat, T>(&self, transport: &T)
    where
        T: TransportGeneric<Dump, Stat>,
    {
        {
            let producers_weak = Arc::downgrade(&self.inner.producers);
            transport
                .on_new_producer(move |producer| {
                    let producer_id = producer.id();
                    if let Some(producers) = producers_weak.upgrade() {
                        producers
                            .lock()
                            .unwrap()
                            .insert(producer_id, producer.downgrade());
                    }
                    {
                        let producers_weak = producers_weak.clone();
                        producer
                            .on_closed(move || {
                                if let Some(producers) = producers_weak.upgrade() {
                                    producers.lock().unwrap().remove(&producer_id);
                                }
                            })
                            .detach();
                    }
                })
                .detach();
        }
        {
            let data_producers_weak = Arc::downgrade(&self.inner.data_producers);
            transport
                .on_new_data_producer(move |data_producer| {
                    let data_producer_id = data_producer.id();
                    if let Some(data_producers) = data_producers_weak.upgrade() {
                        data_producers
                            .lock()
                            .unwrap()
                            .insert(data_producer_id, data_producer.downgrade());
                    }
                    {
                        let data_producers_weak = data_producers_weak.clone();
                        data_producer
                            .on_closed(move || {
                                if let Some(data_producers) = data_producers_weak.upgrade() {
                                    data_producers.lock().unwrap().remove(&data_producer_id);
                                }
                            })
                            .detach();
                    }
                })
                .detach();
        }
    }

    fn has_producer(&self, producer_id: &ProducerId) -> bool {
        self.inner
            .producers
            .lock()
            .unwrap()
            .contains_key(producer_id)
    }

    fn get_producer(&self, producer_id: &ProducerId) -> Option<Producer> {
        self.inner
            .producers
            .lock()
            .unwrap()
            .get(producer_id)?
            .upgrade()
    }

    fn has_data_producer(&self, data_producer_id: &DataProducerId) -> bool {
        self.inner
            .data_producers
            .lock()
            .unwrap()
            .contains_key(data_producer_id)
    }

    fn get_data_producer(&self, data_producer_id: &DataProducerId) -> Option<DataProducer> {
        self.inner
            .data_producers
            .lock()
            .unwrap()
            .get(data_producer_id)?
            .upgrade()
    }
}
