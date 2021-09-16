use crate::messages::{Notification, Request};
use crate::worker::common::{EventHandlers, SubscriptionTarget, WeakEventHandlers};
use crate::worker::utils::{PreparedPayloadChannelRead, PreparedPayloadChannelWrite};
use crate::worker::{utils, RequestError, SubscriptionHandler};
use futures_lite::future;
use log::{debug, error, trace, warn};
use mediasoup_sys::UvAsyncT;
use nohash_hasher::IntMap;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::VecDeque;
use std::fmt::Debug;
use std::sync::{Arc, Weak};
use std::time::Duration;
use thiserror::Error;

#[derive(Debug)]
pub(super) enum InternalMessage {
    /// Unknown data
    UnexpectedData(Vec<u8>),
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum PayloadChannelReceiveMessage {
    #[serde(rename_all = "camelCase")]
    Notification { target_id: SubscriptionTarget },
    ResponseSuccess {
        id: u32,
        // The following field is present, unused, but needed for differentiating successful
        // response from error case
        #[allow(dead_code)]
        accepted: bool,
        data: Option<Value>,
    },
    ResponseError {
        id: u32,
        // The following field is present, but unused
        // error: Value,
        reason: String,
    },
    /// Unknown data
    #[serde(skip)]
    Internal(InternalMessage),
}

fn deserialize_message(bytes: &[u8]) -> PayloadChannelReceiveMessage {
    match serde_json::from_slice(bytes) {
        Ok(message) => message,
        Err(error) => {
            warn!("Failed to deserialize message: {}", error);

            PayloadChannelReceiveMessage::Internal(InternalMessage::UnexpectedData(Vec::from(
                bytes,
            )))
        }
    }
}

#[derive(Debug, Error, Eq, PartialEq)]
pub enum NotificationError {
    #[error("Channel already closed")]
    ChannelClosed,
}

struct ResponseError {
    reason: String,
}

type Response<T> = Result<Option<T>, ResponseError>;

#[derive(Default)]
struct RequestsContainer {
    next_id: u32,
    handlers: IntMap<u32, async_oneshot::Sender<Response<Value>>>,
}

struct OutgoingMessageBuffer {
    handle: Option<UvAsyncT>,
    messages: VecDeque<(Vec<u8>, Vec<u8>)>,
}

struct Inner {
    outgoing_message_buffer: Arc<Mutex<OutgoingMessageBuffer>>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    requests_container_weak: Weak<Mutex<RequestsContainer>>,
    #[allow(clippy::type_complexity)]
    event_handlers_weak: WeakEventHandlers<Arc<dyn Fn(&[u8], &[u8]) + Send + Sync + 'static>>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        self.internal_message_receiver.close();
    }
}

#[derive(Clone)]
pub(crate) struct PayloadChannel {
    inner: Arc<Inner>,
}

impl PayloadChannel {
    pub(super) fn new() -> (
        Self,
        PreparedPayloadChannelRead,
        PreparedPayloadChannelWrite,
    ) {
        let outgoing_message_buffer = Arc::new(Mutex::new(OutgoingMessageBuffer {
            handle: None,
            messages: VecDeque::<(Vec<u8>, Vec<u8>)>::with_capacity(10),
        }));
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();
        let requests_container_weak = Arc::downgrade(&requests_container);
        let event_handlers = EventHandlers::new();
        let event_handlers_weak = event_handlers.downgrade();

        let prepared_payload_channel_read = utils::prepare_payload_channel_read_fn({
            let outgoing_message_buffer = Arc::clone(&outgoing_message_buffer);

            move |handle| {
                let mut outgoing_message_buffer = outgoing_message_buffer.lock();
                if outgoing_message_buffer.handle.is_none() {
                    outgoing_message_buffer.handle.replace(handle);
                }

                outgoing_message_buffer.messages.pop_front()
            }
        });

        let (internal_message_sender, internal_message_receiver) = async_channel::bounded(1);

        let prepared_payload_channel_write =
            utils::prepare_payload_channel_write_fn(move |message, payload| {
                trace!("received raw message: {}", String::from_utf8_lossy(message));

                match deserialize_message(message) {
                    PayloadChannelReceiveMessage::Notification { target_id } => {
                        trace!("received notification payload of {} bytes", payload.len());

                        event_handlers.call_callbacks_with_two_values(&target_id, message, payload);
                    }
                    PayloadChannelReceiveMessage::ResponseSuccess { id, data, .. } => {
                        let sender = requests_container.lock().handlers.remove(&id);
                        if let Some(mut sender) = sender {
                            let _ = sender.send(Ok(data));
                        } else {
                            warn!(
                                "received success response does not match any sent request [id:{}]",
                                id,
                            );
                        }
                    }
                    PayloadChannelReceiveMessage::ResponseError { id, reason } => {
                        let sender = requests_container.lock().handlers.remove(&id);
                        if let Some(mut sender) = sender {
                            let _ = sender.send(Err(ResponseError { reason }));
                        } else {
                            warn!(
                                "received error response does not match any sent request [id:{}]",
                                id,
                            );
                        }
                    }
                    PayloadChannelReceiveMessage::Internal(internal_message) => {
                        let _ = internal_message_sender.try_send(internal_message);
                    }
                }
            });

        let inner = Arc::new(Inner {
            outgoing_message_buffer,
            internal_message_receiver,
            requests_container_weak,
            event_handlers_weak,
        });

        (
            Self { inner },
            prepared_payload_channel_read,
            prepared_payload_channel_write,
        )
    }

    pub(super) fn get_internal_message_receiver(&self) -> async_channel::Receiver<InternalMessage> {
        self.inner.internal_message_receiver.clone()
    }

    pub(crate) async fn request<R>(
        &self,
        request: R,
        payload: Vec<u8>,
    ) -> Result<R::Response, RequestError>
    where
        R: Request,
    {
        let method = request.as_method();

        let id;
        let queue_len;
        let (result_sender, result_receiver) = async_oneshot::oneshot();

        {
            let requests_container_lock = self
                .inner
                .requests_container_weak
                .upgrade()
                .ok_or(RequestError::ChannelClosed)?;
            let mut requests_container = requests_container_lock.lock();

            id = requests_container.next_id;
            queue_len = requests_container.handlers.len();

            requests_container.next_id = requests_container.next_id.wrapping_add(1);
            requests_container.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{}, id:{}]: {:?}", method, id, request);

        {
            #[derive(Debug, Serialize)]
            struct RequestMessagePrivate<'a, R> {
                id: u32,
                method: &'static str,
                #[serde(flatten)]
                request: &'a R,
            }

            let message = serde_json::to_vec(&RequestMessagePrivate {
                id,
                method: request.as_method(),
                request: &request,
            })
            .unwrap();

            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back((message, payload));
            if let Some(handle) = &outgoing_message_buffer.handle {
                unsafe {
                    // Notify worker that there is something to read
                    let ret = mediasoup_sys::uv_async_send(*handle);
                    if ret != 0 {
                        error!("uv_async_send call failed with code {}", ret);
                        return Err(RequestError::ChannelClosed);
                    }
                }
            }
        }

        let result = future::or(
            async move {
                result_receiver
                    .await
                    .map_err(|_| RequestError::ChannelClosed {})
            },
            async move {
                async_io::Timer::after(Duration::from_millis(
                    (1000.0 * (15.0 + (0.1 * queue_len as f64))).round() as u64,
                ))
                .await;

                if let Some(requests_container) = self.inner.requests_container_weak.upgrade() {
                    requests_container.lock().handlers.remove(&id);
                }

                Err(RequestError::TimedOut)
            },
        )
        .await?;

        let data = match result {
            Ok(data) => {
                debug!("request succeeded [method:{}, id:{}]", method, id);
                data
            }
            Err(ResponseError { reason }) => {
                debug!("request failed [method:{}, id:{}]: {}", method, id, reason);

                return Err(RequestError::Response { reason });
            }
        };

        // Default will work for `()` response
        serde_json::from_value(data.unwrap_or_default()).map_err(|error| {
            RequestError::FailedToParse {
                error: error.to_string(),
            }
        })
    }

    pub(crate) fn notify<N>(
        &self,
        notification: N,
        payload: Vec<u8>,
    ) -> Result<(), NotificationError>
    where
        N: Notification,
    {
        debug!("notify() [event:{}]", notification.as_event());

        {
            #[derive(Debug, Serialize)]
            struct NotificationMessagePrivate<'a, N: Serialize> {
                event: &'static str,
                #[serde(flatten)]
                notification: &'a N,
            }

            let message = serde_json::to_vec(&NotificationMessagePrivate {
                event: notification.as_event(),
                notification: &notification,
            })
            .unwrap();

            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back((message, payload));
            if let Some(handle) = &outgoing_message_buffer.handle {
                unsafe {
                    // Notify worker that there is something to read
                    let ret = mediasoup_sys::uv_async_send(*handle);
                    if ret != 0 {
                        error!("uv_async_send call failed with code {}", ret);
                        return Err(NotificationError::ChannelClosed);
                    }
                }
            }
        }

        Ok(())
    }

    pub(crate) fn subscribe_to_notifications<F>(
        &self,
        target_id: SubscriptionTarget,
        callback: F,
    ) -> Option<SubscriptionHandler>
    where
        F: Fn(&[u8], &[u8]) + Send + Sync + 'static,
    {
        self.inner
            .event_handlers_weak
            .upgrade()
            .map(|event_handlers| event_handlers.add(target_id, Arc::new(callback)))
    }
}
