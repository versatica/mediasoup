use async_executor::Executor;
use parking_lot::Mutex;
use std::collections::HashMap;
use std::sync::Arc;

struct EventHandlersList<V: Clone + 'static> {
    index: usize,
    callbacks: HashMap<usize, Box<dyn Fn(V) + Send>>,
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
    executor: Arc<Executor<'static>>,
    handlers: Arc<Mutex<HashMap<String, EventHandlersList<V>>>>,
}

impl<V: Clone + 'static> EventHandlers<V> {
    pub(super) fn new(executor: Arc<Executor<'static>>) -> Self {
        let handlers = Arc::<Mutex<HashMap<String, EventHandlersList<V>>>>::default();
        Self { executor, handlers }
    }

    // TODO: Avoid `String` if possible (switch to uuid or enum with uuid and u32)
    pub(super) fn add(
        &self,
        target_id: String,
        callback: Box<dyn Fn(V) + Send + 'static>,
    ) -> SubscriptionHandler {
        let mut event_handlers = self.handlers.lock();
        let list = event_handlers
            .entry(target_id.clone())
            .or_insert_with(EventHandlersList::default);
        let index = list.index;
        list.index += 1;
        list.callbacks.insert(index, Box::new(callback));
        drop(event_handlers);

        SubscriptionHandler::new({
            let event_handlers = self.handlers.clone();

            Box::new(move || {
                let mut handlers = event_handlers.lock();
                let is_empty = {
                    let list = handlers.get_mut(&target_id).unwrap();
                    list.callbacks.remove(&index);
                    list.callbacks.is_empty()
                };
                if is_empty {
                    handlers.remove(&target_id);
                }
            })
        })
    }

    pub(super) fn call_callbacks_with_value(&self, target_id: &str, value: V) {
        let handlers = self.handlers.lock();
        if let Some(list) = handlers.get(target_id) {
            let mut callbacks = list.callbacks.values();
            if callbacks.len() == 1 {
                // Tiny optimization that avoids cloning `value`
                (callbacks.next().unwrap())(value);
            } else {
                for callback in callbacks {
                    callback(value.clone());
                }
            }
        }
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
