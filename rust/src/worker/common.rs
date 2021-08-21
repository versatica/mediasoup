use parking_lot::Mutex;
use std::collections::HashMap;
use std::sync::{Arc, Weak};
use uuid::Uuid;

struct EventHandlersList<V: Clone + 'static> {
    index: usize,
    #[allow(clippy::type_complexity)]
    callbacks: HashMap<usize, Arc<dyn Fn(V) + Send + Sync>>,
}

impl<V: Clone + 'static> Default for EventHandlersList<V> {
    fn default() -> Self {
        Self {
            index: 0,
            callbacks: HashMap::new(),
        }
    }
}

#[derive(Clone)]
pub(super) struct EventHandlers<V: Clone + 'static> {
    handlers: Arc<Mutex<HashMap<SubscriptionTarget, EventHandlersList<V>>>>,
}

impl<V: Clone + 'static> EventHandlers<V> {
    pub(super) fn new() -> Self {
        let handlers = Arc::<Mutex<HashMap<SubscriptionTarget, EventHandlersList<V>>>>::default();
        Self { handlers }
    }

    pub(super) fn add(
        &self,
        target_id: SubscriptionTarget,
        callback: Box<dyn Fn(V) + Send + Sync + 'static>,
    ) -> SubscriptionHandler {
        let index;
        {
            let mut event_handlers = self.handlers.lock();
            let list = event_handlers
                .entry(target_id)
                .or_insert_with(EventHandlersList::default);
            index = list.index;
            list.index += 1;
            list.callbacks.insert(index, Arc::new(Box::new(callback)));
            drop(event_handlers);
        }

        SubscriptionHandler::new({
            let event_handlers_weak = Arc::downgrade(&self.handlers);

            Box::new(move || {
                if let Some(event_handlers) = event_handlers_weak.upgrade() {
                    // We store removed handler here in order to drop after `event_handlers` lock is
                    // released, otherwise handler will be dropped on removal from callbacks
                    // immediately and if it happens to hold another entity that held subscription
                    // handler, we may arrive here again trying to acquire lock that we didn't
                    // release yet. By dropping callback only when lock is released we avoid
                    // deadlocking.
                    let removed_handler;
                    {
                        let mut handlers = event_handlers.lock();
                        let is_empty = {
                            let list = handlers.get_mut(&target_id).unwrap();
                            removed_handler = list.callbacks.remove(&index);
                            list.callbacks.is_empty()
                        };
                        if is_empty {
                            handlers.remove(&target_id);
                        }
                    }
                    drop(removed_handler);
                }
            })
        })
    }

    pub(super) fn call_callbacks_with_value(&self, target_id: &SubscriptionTarget, value: V) {
        let handlers = self.handlers.lock();
        if let Some(list) = handlers.get(target_id) {
            // Tiny optimization that avoids cloning `value` unless necessary
            if list.callbacks.len() == 1 {
                let callback = list.callbacks.values().next().cloned().unwrap();
                // Drop mutex guard before running callbacks to avoid deadlocks
                drop(handlers);
                callback(value);
            } else {
                let callbacks = list.callbacks.values().cloned().collect::<Vec<_>>();
                // Drop mutex guard before running callbacks to avoid deadlocks
                drop(handlers);
                for callback in callbacks {
                    callback(value.clone());
                }
            }
        }
    }

    pub(super) fn downgrade(&self) -> WeakEventHandlers<V> {
        WeakEventHandlers {
            handlers: Arc::downgrade(&self.handlers),
        }
    }
}

#[derive(Clone)]
pub(super) struct WeakEventHandlers<V: Clone + 'static> {
    handlers: Weak<Mutex<HashMap<SubscriptionTarget, EventHandlersList<V>>>>,
}

impl<V: Clone + 'static> WeakEventHandlers<V> {
    pub(super) fn upgrade(&self) -> Option<EventHandlers<V>> {
        self.handlers
            .upgrade()
            .map(|handlers| EventHandlers { handlers })
    }
}

#[derive(Copy, Clone, Eq, PartialEq, Hash)]
pub(crate) enum SubscriptionTarget {
    Uuid(Uuid),
    Number(u32),
}

impl From<u32> for SubscriptionTarget {
    fn from(number: u32) -> Self {
        Self::Number(number)
    }
}

/// Subscription handler, will remove corresponding subscription when dropped
pub(crate) struct SubscriptionHandler {
    remove_callback: Option<Box<dyn FnOnce() + Send + Sync>>,
}

impl SubscriptionHandler {
    fn new(remove_callback: Box<dyn FnOnce() + Send + Sync>) -> Self {
        Self {
            remove_callback: Some(remove_callback),
        }
    }
}

impl Drop for SubscriptionHandler {
    fn drop(&mut self) {
        let remove_callback = self.remove_callback.take().unwrap();
        remove_callback();
    }
}
