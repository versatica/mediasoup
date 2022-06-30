use super::*;
use std::env;

fn init() {
    {
        let mut builder = env_logger::builder();
        if env::var(env_logger::DEFAULT_FILTER_ENV).is_err() {
            builder.filter_level(log::LevelFilter::Off);
        }
        let _ = builder.is_test(true).try_init();
    }
}

#[test]
fn worker_manager_test() {
    init();

    let worker_manager = WorkerManager::new();

    future::block_on(async move {
        let _ = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .unwrap();
    });
}
