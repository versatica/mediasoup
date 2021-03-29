//! An audio level observer monitors the volume of the selected audio producers.

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
use async_trait::async_trait;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use log::{debug, error};
use parking_lot::Mutex;
use serde::Deserialize;
use std::num::NonZeroU16;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};

/// AudioLevelObserver options
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

/// Represents volume of one audio producer.
#[derive(Clone)]
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
    _subscription_handler: Option<SubscriptionHandler>,
    _on_router_close_handler: Mutex<HandlerId>,
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

/// An audio level observer monitors the volume of the selected audio producers.
///
/// It just handles audio producers (if [`AudioLevelObserver::add_producer()`] is called with a
/// video producer it will fail).
///
/// Audio levels are read from an RTP header extension. No decoding of audio data is done. See
/// [RFC6464](https://tools.ietf.org/html/rfc6464) for more information.
#[derive(Clone)]
pub struct AudioLevelObserver {
    inner: Arc<Inner>,
}

#[async_trait(?Send)]
impl RtpObserver for AudioLevelObserver {
    fn id(&self) -> RtpObserverId {
        self.inner.id
    }

    fn paused(&self) -> bool {
        self.inner.paused.load(Ordering::SeqCst)
    }

    fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    fn closed(&self) -> bool {
        self.inner.closed.load(Ordering::SeqCst)
    }

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

    fn on_pause(&self, callback: Box<dyn Fn() + Send + Sync + 'static>) -> HandlerId {
        self.inner.handlers.pause.add(Box::new(callback))
    }

    fn on_resume(&self, callback: Box<dyn Fn() + Send + Sync + 'static>) -> HandlerId {
        self.inner.handlers.resume.add(Box::new(callback))
    }

    fn on_add_producer(
        &self,
        callback: Box<dyn Fn(&Producer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.add_producer.add(Box::new(callback))
    }

    fn on_remove_producer(
        &self,
        callback: Box<dyn Fn(&Producer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.remove_producer.add(Box::new(callback))
    }

    fn on_router_close(&self, callback: Box<dyn FnOnce() + Send + 'static>) -> HandlerId {
        self.inner.handlers.router_close.add(Box::new(callback))
    }

    fn on_close(&self, callback: Box<dyn FnOnce() + Send + 'static>) -> HandlerId {
        let handler_id = self.inner.handlers.close.add(Box::new(callback));
        if self.inner.closed.load(Ordering::Relaxed) {
            self.inner.handlers.close.call_simple();
        }
        handler_id
    }
}

impl AudioLevelObserver {
    pub(super) fn new(
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

            channel.subscribe_to_notifications(id.into(), move |notification| {
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
        };

        let inner_weak = Arc::<Mutex<Option<Weak<Inner>>>>::default();
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
            _on_router_close_handler: Mutex::new(on_router_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        Self { inner }
    }

    /// Callback is called at most every interval (see [`AudioLevelObserverOptions`]).
    ///
    /// Audio volumes entries ordered by volume (louder ones go first).
    pub fn on_volumes<F: Fn(&Vec<AudioLevelObserverVolume>) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.volumes.add(Box::new(callback))
    }

    /// Callback is called when no one of the producers in this RTP observer is generating audio with a volume
    /// beyond the given threshold.
    pub fn on_silence<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.silence.add(Box::new(callback))
    }

    /// Downgrade `AudioLevelObserver` to [`WeakAudioLevelObserver`] instance.
    pub fn downgrade(&self) -> WeakAudioLevelObserver {
        WeakAudioLevelObserver {
            inner: Arc::downgrade(&self.inner),
        }
    }

    fn get_internal(&self) -> RtpObserverInternal {
        RtpObserverInternal {
            router_id: self.inner.router.id(),
            rtp_observer_id: self.inner.id,
        }
    }
}

/// [`WeakAudioLevelObserver`] doesn't own audio level observer instance on mediasoup-worker and
/// will not prevent one from being destroyed once last instance of regular [`AudioLevelObserver`]
/// is dropped.
///
/// [`WeakAudioLevelObserver`] vs [`AudioLevelObserver`] is similar to [`Weak`] vs [`Arc`].
#[derive(Clone)]
pub struct WeakAudioLevelObserver {
    inner: Weak<Inner>,
}

impl WeakAudioLevelObserver {
    /// Attempts to upgrade `WeakAudioLevelObserver` to [`AudioLevelObserver`] if last instance of one wasn't
    /// dropped yet.
    pub fn upgrade(&self) -> Option<AudioLevelObserver> {
        let inner = self.inner.upgrade()?;

        Some(AudioLevelObserver { inner })
    }
}
