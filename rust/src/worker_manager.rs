use crate::worker::{Worker, WorkerSettings};
use async_executor::Executor;
use async_oneshot::Sender;
use futures_lite::future;
use std::io;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};

#[derive(Default)]
struct Handlers {
    new_worker: Mutex<Vec<Box<dyn Fn(&Arc<Worker>)>>>,
}

pub struct WorkerManager {
    executor: Arc<Executor>,
    handlers: Handlers,
    /// This field is only used in order to be dropped with the worker manager itself to stop the
    /// thread created with `WorkerManager::new()` call
    _stop_sender: Option<Sender<()>>,
}

impl WorkerManager {
    /// Create new worker manager, internally a new thread with executor will be created
    pub fn new() -> Self {
        let executor = Arc::new(Executor::new());
        let (stop_sender, stop_receiver) = async_oneshot::oneshot::<()>();
        {
            let executor = Arc::clone(&executor);
            std::thread::spawn(move || {
                // Will return Err(Closed) when `WorkerManager` struct is dropped
                drop(future::block_on(executor.run(stop_receiver)));
            });
        }

        let handlers = Handlers::default();

        Self {
            executor,
            handlers,
            _stop_sender: Some(stop_sender),
        }
    }

    /// Create new worker manager, uses externally created executor
    pub fn with_executor(executor: Arc<Executor>) -> Self {
        let handlers = Handlers::default();

        Self {
            executor,
            handlers,
            _stop_sender: None,
        }
    }

    /// Create a Worker.
    pub async fn create_worker(
        &self,
        worker_binary: PathBuf,
        worker_settings: WorkerSettings,
    ) -> io::Result<Arc<Worker>> {
        let worker = Arc::new(
            Worker::new(Arc::clone(&self.executor), worker_binary, worker_settings).await?,
        );
        {
            for callback in self.handlers.new_worker.lock().unwrap().iter() {
                callback(&worker);
            }
        }

        Ok(worker)
    }

    pub fn connect_new_worker<F: Fn(&Arc<Worker>) + 'static>(&self, callback: F) {
        self.handlers
            .new_worker
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env;

    fn init() {
        drop(env_logger::builder().is_test(true).try_init());
    }

    #[test]
    fn worker_manager_test() {
        init();

        let worker_manager = WorkerManager::new();

        let worker_settings = WorkerSettings::default();
        future::block_on(async move {
            worker_manager
                .create_worker(
                    env::var("MEDIASOUP_WORKER_BIN")
                        .map(|path| path.into())
                        .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into()),
                    worker_settings,
                )
                .await
                .unwrap();
        })
    }
}
