//! Container that creates [`Worker`] instances.

#[cfg(test)]
mod tests;

use crate::worker::{Worker, WorkerSettings};
use async_executor::Executor;
use async_oneshot::Sender;
use event_listener_primitives::{Bag, HandlerId};
use futures_lite::future;
use log::debug;
use std::io;
use std::path::PathBuf;
use std::sync::Arc;

#[derive(Default)]
struct Handlers {
    new_worker: Bag<Box<dyn Fn(&Worker) + Send + Sync>>,
}

struct Inner {
    executor: Arc<Executor<'static>>,
    handlers: Handlers,
    /// This field is only used in order to be dropped with the worker manager itself to stop the
    /// thread created with `WorkerManager::new()` call
    _stop_sender: Option<Sender<()>>,
    worker_binary: PathBuf,
}

/// Container that creates [`Worker`] instances.
///
/// # Examples
/// ```no_run
/// use futures_lite::future;
/// use mediasoup::worker::WorkerSettings;
/// use mediasoup::worker_manager::WorkerManager;
///
/// // Create a manager that will use specified binary for spawning new worker processes
/// let worker_manager = WorkerManager::new(
///     "/path/to/mediasoup-worker".into(),
/// );
///
/// future::block_on(async move {
///     // Create a new worker with default settings
///     let worker = worker_manager
///         .create_worker(WorkerSettings::default())
///         .await
///         .unwrap();
/// })
/// ```
///
/// If you already happen to have [`async_executor::Executor`] instance available or need a
/// multi-threaded executor, [`WorkerManager::with_executor()`] can be used to create an instance
/// instead.
#[derive(Clone)]
pub struct WorkerManager {
    inner: Arc<Inner>,
}

impl WorkerManager {
    /// Create new worker manager, internally a new single-threaded executor will be created.
    pub fn new(worker_binary: PathBuf) -> Self {
        let executor = Arc::new(Executor::new());
        let (stop_sender, stop_receiver) = async_oneshot::oneshot::<()>();
        {
            let executor = Arc::clone(&executor);
            std::thread::spawn(move || {
                // Will return Err(Closed) when `WorkerManager` struct is dropped
                let _ = future::block_on(executor.run(stop_receiver));
            });
        }

        let handlers = Handlers::default();

        let inner = Arc::new(Inner {
            executor,
            handlers,
            _stop_sender: Some(stop_sender),
            worker_binary,
        });

        Self { inner }
    }

    /// Create new worker manager, uses externally provided executor.
    pub fn with_executor(worker_binary: PathBuf, executor: Arc<Executor<'static>>) -> Self {
        let handlers = Handlers::default();

        let inner = Arc::new(Inner {
            executor,
            handlers,
            _stop_sender: None,
            worker_binary,
        });

        Self { inner }
    }

    /// Creates a new worker with the given settings.
    ///
    /// Worker manager will be kept alive as long as at least one worker instance is alive.
    pub async fn create_worker(&self, worker_settings: WorkerSettings) -> io::Result<Worker> {
        debug!("create_worker()");

        let worker = Worker::new(
            Arc::clone(&self.inner.executor),
            self.inner.worker_binary.clone(),
            worker_settings,
            self.clone(),
        )
        .await?;
        self.inner.handlers.new_worker.call(|callback| {
            callback(&worker);
        });

        Ok(worker)
    }

    /// Callback is called when a new worker is created.
    pub fn on_new_worker<F: Fn(&Worker) + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.new_worker.add(Box::new(callback))
    }
}
