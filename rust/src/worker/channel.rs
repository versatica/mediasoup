use crate::messages::{Request, WorkerCloseRequest};
use crate::worker::common::{EventHandlers, SubscriptionTarget, WeakEventHandlers};
use crate::worker::utils;
use crate::worker::utils::{PreparedChannelRead, PreparedChannelWrite};
use crate::worker::{RequestError, SubscriptionHandler};
use atomic_take::AtomicTake;
use hash_hasher::HashedMap;
use log::{debug, error, trace, warn};
use lru::LruCache;
use mediasoup_sys::UvAsyncT;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::any::TypeId;
use std::collections::VecDeque;
use std::fmt::Debug;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};

#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub(super) enum InternalMessage {
    /// Debug log
    #[serde(skip)]
    Debug(String),
    /// Warn log
    #[serde(skip)]
    Warn(String),
    /// Error log
    #[serde(skip)]
    Error(String),
    /// Dump log
    #[serde(skip)]
    Dump(String),
    /// Unknown
    #[serde(skip)]
    Unexpected(Vec<u8>),
}

pub(crate) struct BufferMessagesGuard {
    target_id: SubscriptionTarget,
    buffered_notifications_for: Arc<Mutex<HashedMap<SubscriptionTarget, Vec<Vec<u8>>>>>,
    event_handlers_weak: WeakEventHandlers<Arc<dyn Fn(&[u8]) + Send + Sync + 'static>>,
}

impl Drop for BufferMessagesGuard {
    fn drop(&mut self) {
        let mut buffered_notifications_for = self.buffered_notifications_for.lock();
        if let Some(notifications) = buffered_notifications_for.remove(&self.target_id) {
            if let Some(event_handlers) = self.event_handlers_weak.upgrade() {
                for notification in notifications {
                    event_handlers.call_callbacks_with_single_value(&self.target_id, &notification);
                }
            }
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum ChannelReceiveMessage {
    #[serde(rename_all = "camelCase")]
    Notification {
        target_id: SubscriptionTarget,
    },
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
    Event(InternalMessage),
}

fn deserialize_message(bytes: &[u8]) -> ChannelReceiveMessage {
    match bytes[0] {
        // JSON message
        b'{' => serde_json::from_slice(bytes).unwrap(),
        // Debug log
        b'D' => ChannelReceiveMessage::Event(InternalMessage::Debug(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Warn log
        b'W' => ChannelReceiveMessage::Event(InternalMessage::Warn(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Error log
        b'E' => ChannelReceiveMessage::Event(InternalMessage::Error(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Dump log
        b'X' => ChannelReceiveMessage::Event(InternalMessage::Dump(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Unknown
        _ => ChannelReceiveMessage::Event(InternalMessage::Unexpected(Vec::from(bytes))),
    }
}

#[derive(Debug, Serialize)]
struct RequestMessage<'a, R: Serialize> {
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
    message: Arc<AtomicTake<Vec<u8>>>,
    channel: &'a Channel,
    removed: bool,
}

impl<'a> Drop for RequestDropGuard<'a> {
    fn drop(&mut self) {
        if self.removed {
            return;
        }

        // Drop pending message from memory
        self.message.take();
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
    handlers: HashedMap<u32, async_oneshot::Sender<ResponseResult<Value>>>,
}

struct OutgoingMessageBuffer {
    handle: Option<UvAsyncT>,
    messages: VecDeque<Arc<AtomicTake<Vec<u8>>>>,
}

struct Inner {
    outgoing_message_buffer: Arc<Mutex<OutgoingMessageBuffer>>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    requests_container_weak: Weak<Mutex<RequestsContainer>>,
    buffered_notifications_for: Arc<Mutex<HashedMap<SubscriptionTarget, Vec<Vec<u8>>>>>,
    event_handlers_weak: WeakEventHandlers<Arc<dyn Fn(&[u8]) + Send + Sync + 'static>>,
    worker_closed: Arc<AtomicBool>,
    closed: AtomicBool,
}

impl Drop for Inner {
    fn drop(&mut self) {
        self.internal_message_receiver.close();
    }
}

#[derive(Clone)]
pub(crate) struct Channel {
    inner: Arc<Inner>,
}

impl Channel {
    pub(super) fn new(
        worker_closed: Arc<AtomicBool>,
    ) -> (Self, PreparedChannelRead, PreparedChannelWrite) {
        let outgoing_message_buffer = Arc::new(Mutex::new(OutgoingMessageBuffer {
            handle: None,
            messages: VecDeque::with_capacity(10),
        }));
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();
        let requests_container_weak = Arc::downgrade(&requests_container);
        let buffered_notifications_for =
            Arc::<Mutex<HashedMap<SubscriptionTarget, Vec<Vec<u8>>>>>::default();
        let event_handlers = EventHandlers::new();
        let event_handlers_weak = event_handlers.downgrade();

        let prepared_channel_read = utils::prepare_channel_read_fn({
            let outgoing_message_buffer = Arc::clone(&outgoing_message_buffer);

            move |handle| {
                let mut outgoing_message_buffer = outgoing_message_buffer.lock();
                if outgoing_message_buffer.handle.is_none() {
                    outgoing_message_buffer.handle.replace(handle);
                }

                while let Some(maybe_message) = outgoing_message_buffer.messages.pop_front() {
                    // Request might have already been cancelled
                    if let Some(message) = maybe_message.take() {
                        return Some(message);
                    }
                }

                None
            }
        });

        let (internal_message_sender, internal_message_receiver) = async_channel::unbounded();

        let prepared_channel_write = utils::prepare_channel_write_fn({
            let buffered_notifications_for = Arc::clone(&buffered_notifications_for);
            // This this contain cache of targets that are known to not have buffering, so
            // that we can avoid Mutex locking overhead for them
            let mut non_buffered_notifications = LruCache::<SubscriptionTarget, ()>::new(1000);

            move |message| {
                trace!("received raw message: {}", String::from_utf8_lossy(message));

                match deserialize_message(message) {
                    ChannelReceiveMessage::Notification { target_id } => {
                        if !non_buffered_notifications.contains(&target_id) {
                            let mut buffer_notifications_for = buffered_notifications_for.lock();
                            // Check if we need to buffer notifications for this
                            // target_id
                            if let Some(list) = buffer_notifications_for.get_mut(&target_id) {
                                list.push(Vec::from(message));
                                return;
                            }

                            // Remember we don't need to buffer these
                            non_buffered_notifications.put(target_id, ());
                        }
                        event_handlers.call_callbacks_with_single_value(&target_id, message);
                    }
                    ChannelReceiveMessage::ResponseSuccess { id, data, .. } => {
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
                    ChannelReceiveMessage::ResponseError { id, reason } => {
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
                    ChannelReceiveMessage::Event(event_message) => {
                        let _ = internal_message_sender.try_send(event_message);
                    }
                }
            }
        });

        let inner = Arc::new(Inner {
            outgoing_message_buffer,
            internal_message_receiver,
            requests_container_weak,
            buffered_notifications_for,
            event_handlers_weak,
            worker_closed,
            closed: AtomicBool::new(false),
        });

        (
            Self { inner },
            prepared_channel_read,
            prepared_channel_write,
        )
    }

    pub(super) fn get_internal_message_receiver(&self) -> async_channel::Receiver<InternalMessage> {
        self.inner.internal_message_receiver.clone()
    }

    /// This allows to enable buffering for messages for specific target while the target itself is
    /// being created. This allows to avoid missing notifications due to race conditions.
    pub(crate) fn buffer_messages_for(&self, target_id: SubscriptionTarget) -> BufferMessagesGuard {
        let buffered_notifications_for = Arc::clone(&self.inner.buffered_notifications_for);
        let event_handlers_weak = self.inner.event_handlers_weak.clone();
        buffered_notifications_for
            .lock()
            .entry(target_id)
            .or_default();
        BufferMessagesGuard {
            target_id,
            buffered_notifications_for,
            event_handlers_weak,
        }
    }

    pub(crate) async fn request<R>(&self, request: R) -> Result<R::Response, RequestError>
    where
        R: Request + 'static,
    {
        let method = request.as_method();

        let id;
        let (result_sender, result_receiver) = async_oneshot::oneshot();

        {
            let requests_container = match self.inner.requests_container_weak.upgrade() {
                Some(requests_container_lock) => requests_container_lock,
                None => {
                    if let Some(default_response) = R::default_for_soft_error() {
                        return Ok(default_response);
                    }

                    return Err(RequestError::ChannelClosed);
                }
            };
            let mut requests_container_lock = requests_container.lock();

            id = requests_container_lock.next_id;

            requests_container_lock.next_id = requests_container_lock.next_id.wrapping_add(1);
            requests_container_lock.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{}, id:{}]: {:?}", method, id, request);

        let message = Arc::new(AtomicTake::new(
            serde_json::to_vec(&RequestMessage {
                id,
                method,
                request: &request,
            })
            .unwrap(),
        ));

        {
            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back(Arc::clone(&message));
            if let Some(handle) = outgoing_message_buffer.handle {
                if self.inner.worker_closed.load(Ordering::Acquire) {
                    // Forbid all requests after worker closing except one worker closing request
                    let first_worker_closing = TypeId::of::<R>()
                        == TypeId::of::<WorkerCloseRequest>()
                        && !self.inner.closed.swap(true, Ordering::Relaxed);

                    if !first_worker_closing {
                        if let Some(default_response) = R::default_for_soft_error() {
                            return Ok(default_response);
                        }

                        return Err(RequestError::ChannelClosed);
                    }
                }
                unsafe {
                    // Notify worker that there is something to read
                    let ret = mediasoup_sys::uv_async_send(handle);
                    if ret != 0 {
                        error!("uv_async_send call failed with code {}", ret);
                        if let Some(default_response) = R::default_for_soft_error() {
                            return Ok(default_response);
                        }

                        return Err(RequestError::ChannelClosed);
                    }
                }
            }
        }

        // Drop guard to make sure to drop pending request when future is cancelled
        let request_drop_guard = RequestDropGuard {
            id,
            message,
            channel: self,
            removed: false,
        };

        let response_result_fut = result_receiver.await;

        request_drop_guard.remove();

        let response_result = match response_result_fut {
            Ok(response_result) => response_result,
            Err(_closed) => {
                return if let Some(default_response) = R::default_for_soft_error() {
                    Ok(default_response)
                } else {
                    Err(RequestError::ChannelClosed)
                };
            }
        };

        match response_result {
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
                if reason.contains("not found") {
                    if let Some(default_response) = R::default_for_soft_error() {
                        Ok(default_response)
                    } else {
                        Err(RequestError::ChannelClosed)
                    }
                } else {
                    Err(RequestError::Response { reason })
                }
            }
        }
    }

    pub(crate) fn subscribe_to_notifications<F>(
        &self,
        target_id: SubscriptionTarget,
        callback: F,
    ) -> Option<SubscriptionHandler>
    where
        F: Fn(&[u8]) + Send + Sync + 'static,
    {
        self.inner
            .event_handlers_weak
            .upgrade()
            .map(|event_handlers| event_handlers.add(target_id, Arc::new(callback)))
    }
}
