use crate::data_structures::AppData;
use crate::messages::{
    RtpObserverAddProducerRequest, RtpObserverAddRemoveProducerRequestInternal,
    RtpObserverCloseRequest, RtpObserverInternal, RtpObserverPauseRequest,
    RtpObserverRemoveProducerRequest, RtpObserverResumeRequest,
};
use crate::producer::{Producer, ProducerId};
use crate::router::Router;
use crate::rtp_observer::{RtpObserver, RtpObserverAddProducerOptions, RtpObserverId};
use crate::worker::{Channel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_mutex::Mutex as AsyncMutex;
use async_trait::async_trait;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use log::{debug, error};
use parking_lot::Mutex as SyncMutex;
use serde::Deserialize;
use std::num::NonZeroU16;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};

#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct AudioLevelObserverOptions {
    /// Maximum number of entries in the 'volumes' event.
    /// Default 1.
    pub max_entries: NonZeroU16,
    /// Minimum average volume (in dBvo from -127 to 0) for entries in the 'volumes' event.
    /// Default -80.
    pub threshold: i8,
    /// Interval in ms for checking audio volumes.
    /// Default 1000.
    pub interval: u16,
    /// Custom application data.
    pub app_data: AppData,
}

impl Default for AudioLevelObserverOptions {
    fn default() -> Self {
        Self {
            max_entries: NonZeroU16::new(1).unwrap(),
            threshold: -80,
            interval: 1000,
            app_data: AppData::default(),
        }
    }
}

pub struct AudioLevelObserverVolume {
    /// The audio producer instance.
    pub producer: Producer,
    /// The average volume (in dBvo from -127 to 0) of the audio producer in the last interval.
    pub volume: i8,
}

#[derive(Default)]
struct Handlers {
    volumes: Bag<Box<dyn Fn(&Vec<AudioLevelObserverVolume>) + Send + Sync>>,
    silence: Bag<Box<dyn Fn() + Send + Sync>>,
    pause: Bag<Box<dyn Fn() + Send + Sync>>,
    resume: Bag<Box<dyn Fn() + Send + Sync>>,
    add_producer: Bag<Box<dyn Fn(&Producer) + Send + Sync>>,
    remove_producer: Bag<Box<dyn Fn(&Producer) + Send + Sync>>,
    router_close: BagOnce<Box<dyn FnOnce() + Send>>,
    close: BagOnce<Box<dyn FnOnce() + Send>>,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct VolumeNotification {
    producer_id: ProducerId,
    volume: i8,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    Volumes(Vec<VolumeNotification>),
    Silence,
}

struct Inner {
    id: RtpObserverId,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    handlers: Arc<Handlers>,
    paused: AtomicBool,
    app_data: AppData,
    // Make sure router is not dropped until this audio level observer is not dropped
    router: Router,
    closed: AtomicBool,
    // Drop subscription to audio level observer-specific notifications when observer itself is
    // dropped
    _subscription_handler: SubscriptionHandler,
    _on_router_close_handler: AsyncMutex<HandlerId>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.close();
    }
}

impl Inner {
    fn close(&self) {
        if !self.closed.swap(true, Ordering::SeqCst) {
            debug!("close()");

            self.handlers.close.call_simple();

            {
                let channel = self.channel.clone();
                let request = RtpObserverCloseRequest {
                    internal: RtpObserverInternal {
                        router_id: self.router.id(),
                        rtp_observer_id: self.id,
                    },
                };
                let router = self.router.clone();
                self.executor
                    .spawn(async move {
                        if let Err(error) = channel.request(request).await {
                            error!("audio level observer closing failed on drop: {}", error);
                        }

                        drop(router);
                    })
                    .detach();
            }
        }
    }
}

#[derive(Clone)]
pub struct AudioLevelObserver {
    inner: Arc<Inner>,
}

#[async_trait(?Send)]
impl RtpObserver for AudioLevelObserver {
    /// RtpObserver id.
    fn id(&self) -> RtpObserverId {
        self.inner.id
    }

    /// Whether the RtpObserver is paused.
    fn paused(&self) -> bool {
        self.inner.paused.load(Ordering::SeqCst)
    }

    /// App custom data.
    fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    fn closed(&self) -> bool {
        self.inner.closed.load(Ordering::SeqCst)
    }

    /// Pause the RtpObserver.
    async fn pause(&self) -> Result<(), RequestError> {
        debug!("pause()");

        self.inner
            .channel
            .request(RtpObserverPauseRequest {
                internal: self.get_internal(),
            })
            .await?;

        let was_paused = self.inner.paused.swap(true, Ordering::SeqCst);

        if !was_paused {
            self.inner.handlers.pause.call_simple();
        }

        Ok(())
    }

    /// Resume the RtpObserver.
    async fn resume(&self) -> Result<(), RequestError> {
        debug!("resume()");

        self.inner
            .channel
            .request(RtpObserverResumeRequest {
                internal: self.get_internal(),
            })
            .await?;

        let was_paused = self.inner.paused.swap(false, Ordering::SeqCst);

        if !was_paused {
            self.inner.handlers.resume.call_simple();
        }

        Ok(())
    }

    /// Add a Producer to the RtpObserver.
    async fn add_producer(
        &self,
        RtpObserverAddProducerOptions { producer_id }: RtpObserverAddProducerOptions,
    ) -> Result<(), RequestError> {
        let producer = match self.inner.router.get_producer(&producer_id) {
            Some(producer) => producer,
            None => {
                return Ok(());
            }
        };
        self.inner
            .channel
            .request(RtpObserverAddProducerRequest {
                internal: RtpObserverAddRemoveProducerRequestInternal {
                    router_id: self.inner.router.id(),
                    rtp_observer_id: self.inner.id,
                    producer_id,
                },
            })
            .await?;

        self.inner.handlers.add_producer.call(|callback| {
            callback(&producer);
        });

        Ok(())
    }

    /// Remove a Producer from the RtpObserver.
    async fn remove_producer(&self, producer_id: ProducerId) -> Result<(), RequestError> {
        let producer = match self.inner.router.get_producer(&producer_id) {
            Some(producer) => producer,
            None => {
                return Ok(());
            }
        };
        self.inner
            .channel
            .request(RtpObserverRemoveProducerRequest {
                internal: RtpObserverAddRemoveProducerRequestInternal {
                    router_id: self.inner.router.id(),
                    rtp_observer_id: self.inner.id,
                    producer_id,
                },
            })
            .await?;

        self.inner.handlers.remove_producer.call(|callback| {
            callback(&producer);
        });

        Ok(())
    }
}

impl AudioLevelObserver {
    pub(super) async fn new(
        id: RtpObserverId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        app_data: AppData,
        router: Router,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let paused = AtomicBool::new(false);

        let subscription_handler = {
            let router = router.clone();
            let handlers = Arc::clone(&handlers);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::Volumes(volumes) => {
                                let volumes = volumes
                                    .into_iter()
                                    .filter_map(|notification| {
                                        let VolumeNotification {
                                            producer_id,
                                            volume,
                                        } = notification;
                                        router.get_producer(&producer_id).map(|producer| {
                                            AudioLevelObserverVolume { producer, volume }
                                        })
                                    })
                                    .collect();

                                handlers.volumes.call(|callback| {
                                    callback(&volumes);
                                });
                            }
                            Notification::Silence => {
                                handlers.silence.call_simple();
                            }
                        },
                        Err(error) => {
                            error!("Failed to parse notification: {}", error);
                        }
                    }
                })
                .await
        };

        let inner_weak = Arc::<SyncMutex<Option<Weak<Inner>>>>::default();
        let on_router_close_handler = router.on_close({
            let inner_weak = Arc::clone(&inner_weak);

            move || {
                if let Some(inner) = inner_weak
                    .lock()
                    .as_ref()
                    .and_then(|weak_inner| weak_inner.upgrade())
                {
                    inner.handlers.router_close.call_simple();
                    inner.close();
                }
            }
        });
        let inner = Arc::new(Inner {
            id,
            executor,
            channel,
            handlers,
            paused,
            app_data,
            router,
            closed: AtomicBool::new(false),
            _subscription_handler: subscription_handler,
            _on_router_close_handler: AsyncMutex::new(on_router_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        Self { inner }
    }

    pub fn on_volumes<F: Fn(&Vec<AudioLevelObserverVolume>) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.volumes.add(Box::new(callback))
    }

    pub fn on_silence<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.silence.add(Box::new(callback))
    }

    pub fn on_pause<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.pause.add(Box::new(callback))
    }

    pub fn on_resume<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.resume.add(Box::new(callback))
    }

    pub fn on_add_producer<F: Fn(&Producer) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.add_producer.add(Box::new(callback))
    }

    pub fn on_remove_producer<F: Fn(&Producer) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.remove_producer.add(Box::new(callback))
    }

    pub fn on_router_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.router_close.add(Box::new(callback))
    }

    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.close.add(Box::new(callback))
    }

    fn get_internal(&self) -> RtpObserverInternal {
        RtpObserverInternal {
            router_id: self.inner.router.id(),
            rtp_observer_id: self.inner.id,
        }
    }
}
