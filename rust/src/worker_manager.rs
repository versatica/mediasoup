use crate::worker::{Worker, WorkerSettings};
use async_executor::Executor;
use async_oneshot::Sender;
use futures_lite::future;
use std::io;
use std::path::PathBuf;
use std::sync::Arc;

pub struct WorkerManager {
    executor: Arc<Executor>,
    /// This field is only used in order to be dropped with the worker manager itself to stop the
    /// thread created with `WorkerManager::new()` call
    #[doc(hidden)]
    _stop_sender: Option<Sender<()>>,
}

impl WorkerManager {
    /// Create new worker manager, internally a new thread with executor will be created
    pub fn new() -> Self {
        let executor = Arc::new(Executor::new());
        let (stop_sender, stop_receiver) = async_oneshot::oneshot::<()>();
        std::thread::spawn({
            let executor = Arc::clone(&executor);

            move || {
                let _ = future::block_on(executor.run(stop_receiver));
            }
        });
        Self {
            executor,
            _stop_sender: Some(stop_sender),
        }
    }

    /// Create new worker manager, uses externally created executor
    pub fn with_executor(executor: Arc<Executor>) -> Self {
        Self {
            executor,
            _stop_sender: None,
        }
    }

    pub fn create_worker(
        &self,
        worker_binary: PathBuf,
        worker_settings: WorkerSettings,
    ) -> io::Result<Worker> {
        Worker::new(Arc::clone(&self.executor), worker_binary, worker_settings)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env;

    fn init() {
        let _ = env_logger::builder().is_test(true).try_init();
    }

    #[test]
    fn worker_manager_test() {
        init();

        {
            let worker_manager = WorkerManager::new();

            let worker_settings = WorkerSettings::default();
            let worker = worker_manager
                .create_worker(
                    env::var("MEDIASOUP_WORKER_BIN")
                        .map(|path| path.into())
                        .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into()),
                    worker_settings,
                )
                .unwrap();
        }
    }
}
