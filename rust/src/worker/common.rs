use async_executor::Executor;
use std::pin::Pin;
use std::sync::Arc;

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
