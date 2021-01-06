use async_executor::Executor;
use async_mutex::Mutex;
use std::collections::HashMap;
use std::pin::Pin;
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

    pub(super) async fn add(
        &self,
        target_id: String,
        callback: Box<dyn Fn(V) + Send + 'static>,
    ) -> SubscriptionHandler {
        let mut event_handlers = self.handlers.lock().await;
        let list = event_handlers
            .entry(target_id.clone())
            .or_insert_with(EventHandlersList::default);
        let index = list.index;
        list.index += 1;
        list.callbacks.insert(index, Box::new(callback));
        drop(event_handlers);

        SubscriptionHandler::new(
            Arc::clone(&self.executor),
            Box::pin({
                let event_handlers = self.handlers.clone();

                async move {
                    let mut event_handlers = event_handlers.lock().await;
                    let is_empty = {
                        let list = event_handlers.get_mut(&target_id).unwrap();
                        list.callbacks.remove(&index);
                        list.callbacks.is_empty()
                    };
                    if is_empty {
                        event_handlers.remove(&target_id);
                    }
                }
            }),
        )
    }

    pub(super) async fn call_callbacks_with_value(&self, target_id: &str, value: V) {
        let handlers = self.handlers.lock().await;
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
    executor: Arc<Executor<'static>>,
    remove_fut: Option<Pin<Box<dyn std::future::Future<Output = ()> + Send + Sync>>>,
}

impl SubscriptionHandler {
    pub(super) fn new(
        executor: Arc<Executor<'static>>,
        remove_fut: Pin<Box<dyn std::future::Future<Output = ()> + Send + Sync>>,
    ) -> Self {
        Self {
            executor,
            remove_fut: Some(remove_fut),
        }
    }
}

impl Drop for SubscriptionHandler {
    fn drop(&mut self) {
        self.executor
            .spawn(self.remove_fut.take().unwrap())
            .detach();
    }
}
