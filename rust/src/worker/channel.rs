use crate::fbs::{message, notification, request, response};
use crate::messages::{NotificationFbs, RequestFbs};
use crate::worker::common::{FBSEventHandlers, FBSWeakEventHandlers, SubscriptionTarget};
use crate::worker::utils;
use crate::worker::utils::{PreparedChannelRead, PreparedChannelWrite};
use crate::worker::{RequestError, SubscriptionHandler};
use atomic_take::AtomicTake;
use hash_hasher::HashedMap;
use log::{debug, error, trace, warn};
use lru::LruCache;
use mediasoup_sys::UvAsyncT;
use parking_lot::Mutex;
use planus::ReadAsRoot;
use serde::Deserialize;
use std::collections::VecDeque;
use std::fmt::{Debug, Display};
use std::num::NonZeroUsize;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};
use thiserror::Error;

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

#[derive(Debug, Error, Eq, PartialEq)]
pub enum NotificationError {
    #[error("Channel already closed")]
    ChannelClosed,
}

/// Flabtuffers notification parse error.
#[derive(Debug, Error, Eq, PartialEq)]
pub enum NotificationParseError {
    /// Invalid event
    #[error("Invalid event")]
    InvalidEvent,
}

#[allow(clippy::type_complexity)]
pub(crate) struct BufferMessagesGuard {
    target_id: SubscriptionTarget,
    fbs_buffered_notifications_for: Arc<Mutex<HashedMap<SubscriptionTarget, Vec<Vec<u8>>>>>,
    fbs_event_handlers_weak: FBSWeakEventHandlers<
        Arc<dyn Fn(notification::NotificationRef<'_>) + Send + Sync + 'static>,
    >,
}

impl Drop for BufferMessagesGuard {
    fn drop(&mut self) {
        let mut fbs_buffered_notifications_for = self.fbs_buffered_notifications_for.lock();
        if let Some(notifications) = fbs_buffered_notifications_for.remove(&self.target_id) {
            if let Some(event_handlers) = self.fbs_event_handlers_weak.upgrade() {
                for notification in notifications {
                    let notification =
                        notification::NotificationRef::read_as_root(&notification).unwrap();
                    event_handlers.call_callbacks_with_single_value(&self.target_id, notification);
                }
            }
        }
    }
}

#[derive(Debug)]
enum ChannelReceiveMessage<'a> {
    Notification(notification::NotificationRef<'a>),
    Response(response::ResponseRef<'a>),
    Event(InternalMessage),
}

fn deserialize_message(bytes: &[u8]) -> ChannelReceiveMessage<'_> {
    let message_ref = message::MessageRef::read_as_root(&bytes[4..]).unwrap();

    match message_ref.data().unwrap() {
        message::BodyRef::Log(data) => match data.data().unwrap().chars().next() {
            // Debug log
            Some('D') => ChannelReceiveMessage::Event(InternalMessage::Debug(
                String::from_utf8(Vec::from(&data.data().unwrap().as_bytes()[1..])).unwrap(),
            )),
            // Warn log
            Some('W') => ChannelReceiveMessage::Event(InternalMessage::Warn(
                String::from_utf8(Vec::from(&data.data().unwrap().as_bytes()[1..])).unwrap(),
            )),
            // Error log
            Some('E') => ChannelReceiveMessage::Event(InternalMessage::Error(
                String::from_utf8(Vec::from(&data.data().unwrap().as_bytes()[1..])).unwrap(),
            )),
            // Dump log
            Some('X') => ChannelReceiveMessage::Event(InternalMessage::Dump(
                String::from_utf8(Vec::from(&data.data().unwrap().as_bytes()[1..])).unwrap(),
            )),
            // This should never happen.
            _ => ChannelReceiveMessage::Event(InternalMessage::Unexpected(Vec::from(bytes))),
        },
        message::BodyRef::Notification(data) => ChannelReceiveMessage::Notification(data),
        message::BodyRef::Response(data) => ChannelReceiveMessage::Response(data),

        _ => ChannelReceiveMessage::Event(InternalMessage::Unexpected(Vec::from(bytes))),
    }
}

struct ResponseError {
    reason: String,
}

type FBSResponseResult = Result<Option<response::Body>, ResponseError>;

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
        if let Some(requests_container) = self.channel.inner.fbs_requests_container_weak.upgrade() {
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
struct FBSRequestsContainer {
    next_id: u32,
    handlers: HashedMap<u32, async_oneshot::Sender<FBSResponseResult>>,
}

struct OutgoingMessageBuffer {
    handle: Option<UvAsyncT>,
    messages: VecDeque<Arc<AtomicTake<Vec<u8>>>>,
}

// TODO: use 'close' in 'request' method.
#[allow(clippy::type_complexity, dead_code)]
struct Inner {
    outgoing_message_buffer: Arc<Mutex<OutgoingMessageBuffer>>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    fbs_requests_container_weak: Weak<Mutex<FBSRequestsContainer>>,
    fbs_buffered_notifications_for: Arc<Mutex<HashedMap<SubscriptionTarget, Vec<Vec<u8>>>>>,
    fbs_event_handlers_weak: FBSWeakEventHandlers<
        Arc<dyn Fn(notification::NotificationRef<'_>) + Send + Sync + 'static>,
    >,
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
        let fbs_requests_container = Arc::<Mutex<FBSRequestsContainer>>::default();
        let fbs_requests_container_weak = Arc::downgrade(&fbs_requests_container);
        let fbs_buffered_notifications_for =
            Arc::<Mutex<HashedMap<SubscriptionTarget, Vec<Vec<u8>>>>>::default();
        let fbs_event_handlers = FBSEventHandlers::new();
        let fbs_event_handlers_weak = fbs_event_handlers.downgrade();

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
            let fbs_buffered_notifications_for = Arc::clone(&fbs_buffered_notifications_for);
            // This this contain cache of targets that are known to not have buffering, so
            // that we can avoid Mutex locking overhead for them
            let mut non_buffered_notifications = LruCache::<SubscriptionTarget, ()>::new(
                NonZeroUsize::new(1000).expect("Not zero; qed"),
            );

            move |message| {
                trace!("received raw message: {}", String::from_utf8_lossy(message));

                match deserialize_message(message) {
                    ChannelReceiveMessage::Notification(notification) => {
                        let target_id: SubscriptionTarget = SubscriptionTarget::String(
                            notification.handler_id().unwrap().to_string(),
                        );

                        if !non_buffered_notifications.contains(&target_id) {
                            let mut fbs_buffer_notifications_for =
                                fbs_buffered_notifications_for.lock();
                            // Check if we need to buffer notifications for this
                            // target_id
                            if let Some(list) = fbs_buffer_notifications_for.get_mut(&target_id) {
                                list.push(Vec::from(message));
                                return;
                            }

                            // Remember we don't need to buffer these
                            non_buffered_notifications.put(target_id.clone(), ());
                        }
                        fbs_event_handlers
                            .call_callbacks_with_single_value(&target_id, notification);
                    }
                    ChannelReceiveMessage::Response(response) => {
                        let sender = fbs_requests_container
                            .lock()
                            .handlers
                            .remove(&response.id().unwrap());
                        if let Some(mut sender) = sender {
                            // Request did not succeed.
                            if let Ok(Some(error)) = response.error() {
                                let _ = sender.send(Err(ResponseError {
                                    reason: error.to_string(),
                                }));
                            }
                            // Request succeeded.
                            else {
                                match response.body().expect("failed accessing response body") {
                                    // Response has body.
                                    Some(body_ref) => {
                                        let _ = sender.send(Ok(Some(
                                            body_ref
                                                .try_into()
                                                .expect("failed to retrieve response body"),
                                        )));
                                    }
                                    // Response does not have body.
                                    None => {
                                        let _ = sender.send(Ok(None));
                                    }
                                }
                            }
                        } else {
                            warn!(
                                "received success response does not match any sent request [id:{}]",
                                response.id().unwrap(),
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
            fbs_requests_container_weak,
            fbs_buffered_notifications_for,
            fbs_event_handlers_weak,
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
        let fbs_buffered_notifications_for = Arc::clone(&self.inner.fbs_buffered_notifications_for);
        let fbs_event_handlers_weak = self.inner.fbs_event_handlers_weak.clone();
        fbs_buffered_notifications_for
            .lock()
            .entry(target_id.clone())
            .or_default();
        BufferMessagesGuard {
            target_id,
            fbs_buffered_notifications_for,
            fbs_event_handlers_weak,
        }
    }

    pub(crate) async fn request_fbs<R, HandlerId>(
        &self,
        handler_id: HandlerId,
        request: R,
    ) -> Result<R::Response, RequestError>
    where
        R: RequestFbs<HandlerId = HandlerId> + 'static,
        HandlerId: Display,
    {
        let id;
        let (result_sender, result_receiver) = async_oneshot::oneshot();

        {
            let requests_container = match self.inner.fbs_requests_container_weak.upgrade() {
                Some(requests_container_lock) => requests_container_lock,
                None => {
                    return Err(RequestError::ChannelClosed);
                }
            };
            let mut requests_container_lock = requests_container.lock();

            id = requests_container_lock.next_id;

            requests_container_lock.next_id = requests_container_lock.next_id.wrapping_add(1);
            requests_container_lock.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{:?}, id:{}]", R::METHOD, id);

        let data = request.into_bytes(id, handler_id);

        let buffer = Arc::new(AtomicTake::new(data));

        {
            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back(Arc::clone(&buffer));
            if let Some(handle) = outgoing_message_buffer.handle {
                if self.inner.worker_closed.load(Ordering::Acquire) {
                    // Forbid all requests after worker closing except one worker closing request
                    // TODO: We were checking before that inner.closed.
                    if R::METHOD != request::Method::WorkerClose {
                        return Err(RequestError::ChannelClosed);
                    }
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
            message: buffer,
            channel: self,
            removed: false,
        };

        let response_result_fut = result_receiver.await;

        request_drop_guard.remove();

        let response_result = match response_result_fut {
            Ok(response_result) => response_result,
            Err(_closed) => Err(ResponseError {
                reason: String::from("ChannelClosed"),
            }),
        };

        match response_result {
            Ok(data) => {
                debug!("request succeeded [method:{:?}, id:{}]", R::METHOD, id);
                trace!("{data:?}");

                Ok(R::convert_response(data).map_err(RequestError::ResponseConversion)?)
            }
            Err(ResponseError { reason }) => {
                debug!(
                    "request failed [method:{:?}, id:{}]: {}",
                    R::METHOD,
                    id,
                    reason
                );
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

    pub(crate) fn notify_fbs<N, HandlerId>(
        &self,
        handler_id: HandlerId,
        notification: N,
    ) -> Result<(), NotificationError>
    where
        N: NotificationFbs<HandlerId = HandlerId>,
        HandlerId: Display,
    {
        debug!("notify() [{notification:?}]");

        let data = notification.into_bytes(handler_id);

        let message = Arc::new(AtomicTake::new(data));

        {
            let mut outgoing_message_buffer = self.inner.outgoing_message_buffer.lock();
            outgoing_message_buffer
                .messages
                .push_back(Arc::clone(&message));
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
    pub(crate) fn subscribe_to_fbs_notifications<F>(
        &self,
        target_id: SubscriptionTarget,
        callback: F,
    ) -> Option<SubscriptionHandler>
    where
        F: Fn(notification::NotificationRef<'_>) + Send + Sync + 'static,
    {
        self.inner
            .fbs_event_handlers_weak
            .upgrade()
            .map(|fbs_event_handlers| fbs_event_handlers.add(target_id, Arc::new(callback)))
    }
}
