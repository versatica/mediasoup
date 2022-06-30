use crate::messages::{Notification, Request};
use crate::worker::common::{EventHandlers, SubscriptionTarget, WeakEventHandlers};
use crate::worker::utils::{PreparedPayloadChannelRead, PreparedPayloadChannelWrite};
use crate::worker::{utils, RequestError, SubscriptionHandler};
use atomic_take::AtomicTake;
use log::{debug, error, trace, warn};
use mediasoup_sys::UvAsyncT;
use nohash_hasher::IntMap;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::VecDeque;
use std::fmt::Debug;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};
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

#[derive(Debug, Serialize)]
struct RequestMessage<'a, R> {
    id: u32,
    method: &'static str,
    #[serde(flatten)]
    request: &'a R,
}

struct ResponseError {
    reason: String,
}

type ResponseResult<T> = Result<Option<T>, ResponseError>;

struct RequestDropGuard<'a> {
    id: u32,
    message_with_payload: Arc<AtomicTake<OutgoingMessageRequest>>,
    channel: &'a PayloadChannel,
    removed: bool,
}

impl<'a> Drop for RequestDropGuard<'a> {
    fn drop(&mut self) {
        if self.removed {
            return;
        }

        // Drop pending message from memory
        self.message_with_payload.take();
        // Remove request handler from the container
        if let Some(requests_container) = self.channel.inner.requests_container_weak.upgrade() {
            requests_container.lock().handlers.remove(&self.id);
        }
    }
}

impl<'a> RequestDropGuard<'a> {
    fn remove(mut self) {
        self.removed = true;
    }
}

#[derive(Default)]
struct RequestsContainer {
    next_id: u32,
    handlers: IntMap<u32, async_oneshot::Sender<ResponseResult<Value>>>,
}

struct OutgoingMessageRequest {
    message: Vec<u8>,
    payload: Vec<u8>,
}

enum OutgoingMessage {
    Request(Arc<AtomicTake<OutgoingMessageRequest>>),
    Notification { message: Vec<u8>, payload: Vec<u8> },
}

struct OutgoingMessageBuffer {
    handle: Option<UvAsyncT>,
    messages: VecDeque<OutgoingMessage>,
}

#[derive(Debug, Serialize)]
struct NotificationMessage<'a, N: Serialize> {
    event: &'static str,
    #[serde(flatten)]
    notification: &'a N,
}

#[derive(Debug, Error, Eq, PartialEq)]
pub enum NotificationError {
    #[error("Channel already closed")]
    ChannelClosed,
}

struct Inner {
    outgoing_message_buffer: Arc<Mutex<OutgoingMessageBuffer>>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    requests_container_weak: Weak<Mutex<RequestsContainer>>,
    #[allow(clippy::type_complexity)]
    event_handlers_weak: WeakEventHandlers<Arc<dyn Fn(&[u8], &[u8]) + Send + Sync + 'static>>,
    worker_closed: Arc<AtomicBool>,
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
    pub(super) fn new(
        worker_closed: Arc<AtomicBool>,
    ) -> (
        Self,
        PreparedPayloadChannelRead,
        PreparedPayloadChannelWrite,
    ) {
        let outgoing_message_buffer = Arc::new(Mutex::new(OutgoingMessageBuffer {
            handle: None,
            messages: VecDeque::with_capacity(10),
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

                while let Some(outgoing_message) = outgoing_message_buffer.messages.pop_front() {
                    match outgoing_message {
                        OutgoingMessage::Request(maybe_message) => {
                            // Request might have already been cancelled
                            if let Some(OutgoingMessageRequest { message, payload }) =
                                maybe_message.take()
                            {
                                return Some((message, payload));
                            }
                        }
                        OutgoingMessage::Notification { message, payload } => {
                            return Some((message, payload));
                        }
                    }
                }

                None
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
            worker_closed,
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
        let (result_sender, result_receiver) = async_oneshot::oneshot();

        {
            let requests_container_lock = self
                .inner
                .requests_container_weak
                .upgrade()
                .ok_or(RequestError::ChannelClosed)?;
            let mut requests_container = requests_container_lock.lock();

            id = requests_container.next_id;

            requests_container.next_id = requests_container.next_id.wrapping_add(1);
            requests_container.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{}, id:{}]: {:?}", method, id, request);

        let message = serde_json::to_vec(&RequestMessage {
            id,
            method: request.as_method(),
            request: &request,
        })
        .unwrap();

        let message_with_payload =
            Arc::new(AtomicTake::new(OutgoingMessageRequest { message, payload }));

        {
            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back(OutgoingMessage::Request(Arc::clone(&message_with_payload)));
            if let Some(handle) = outgoing_message_buffer.handle {
                if self.inner.worker_closed.load(Ordering::Acquire) {
                    return Err(RequestError::ChannelClosed);
                }
                unsafe {
                    // Notify worker that there is something to read
                    let ret = mediasoup_sys::uv_async_send(handle);
                    if ret != 0 {
                        error!("uv_async_send call failed with code {}", ret);
                        return Err(RequestError::ChannelClosed);
                    }
                }
            }
        }

        // Drop guard to make sure to drop pending request when future is cancelled
        let request_drop_guard = RequestDropGuard {
            id,
            message_with_payload,
            channel: self,
            removed: false,
        };

        let response_result_fut = result_receiver.await;

        request_drop_guard.remove();

        match response_result_fut.map_err(|_| RequestError::ChannelClosed {})? {
            Ok(data) => {
                debug!("request succeeded [method:{}, id:{}]", method, id);

                // Default will work for `()` response
                serde_json::from_value(data.unwrap_or_default()).map_err(|error| {
                    RequestError::FailedToParse {
                        error: error.to_string(),
                    }
                })
            }
            Err(ResponseError { reason }) => {
                debug!("request failed [method:{}, id:{}]: {}", method, id, reason);

                Err(RequestError::Response { reason })
            }
        }
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

        let message = serde_json::to_vec(&NotificationMessage {
            event: notification.as_event(),
            notification: &notification,
        })
        .unwrap();

        {
            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back(OutgoingMessage::Notification { message, payload });
            if let Some(handle) = outgoing_message_buffer.handle {
                if self.inner.worker_closed.load(Ordering::Acquire) {
                    return Err(NotificationError::ChannelClosed);
                }
                unsafe {
                    // Notify worker that there is something to read
                    let ret = mediasoup_sys::uv_async_send(handle);
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
