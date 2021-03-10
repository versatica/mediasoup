use async_io::Timer;
use futures_lite::future;
use mediasoup::data_structures::AppData;
use mediasoup::worker::{
    WorkerDtlsFiles, WorkerLogLevel, WorkerLogTag, WorkerSettings, WorkerUpdateSettings,
};
use mediasoup::worker_manager::WorkerManager;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::Duration;
use std::{env, io};

async fn init() -> WorkerManager {
    {
        let mut builder = env_logger::builder();
        if env::var(env_logger::DEFAULT_FILTER_ENV).is_err() {
            builder.filter_level(log::LevelFilter::Off);
        }
        let _ = builder.is_test(true).try_init();
    }

    WorkerManager::new(
        env::var("MEDIASOUP_WORKER_BIN")
            .map(|path| path.into())
            .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into()),
    )
}

#[test]
fn create_worker_succeeds() {
    future::block_on(async move {
        let worker_manager = init().await;

        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .expect("Failed to create worker with default settings");

        drop(worker);

        #[derive(Debug, PartialEq)]
        struct CustomAppData {
            bar: u16,
        }

        let worker = worker_manager
            .create_worker({
                let mut settings = WorkerSettings::default();

                settings.log_level = WorkerLogLevel::Debug;
                settings.log_tags = vec![WorkerLogTag::Info];
                settings.rtc_ports_range = 0..=9999;
                settings.dtls_files = Some(WorkerDtlsFiles {
                    certificate: "tests/integration/data/dtls-cert.pem".into(),
                    private_key: "tests/integration/data/dtls-key.pem".into(),
                });
                settings.app_data = AppData::new(CustomAppData { bar: 456 });

                settings
            })
            .await
            .expect("Failed to create worker with custom settings");

        assert_eq!(worker.closed(), false);
        assert_eq!(
            worker.app_data().downcast_ref::<CustomAppData>(),
            Some(&CustomAppData { bar: 456 }),
        );

        drop(worker);
    });
}

#[test]
fn create_worker_wrong_settings() {
    future::block_on(async move {
        let worker_manager = init().await;

        {
            let worker_result = worker_manager
                .create_worker({
                    let mut settings = WorkerSettings::default();

                    settings.rtc_ports_range = 1000..=999;

                    settings
                })
                .await;

            assert!(matches!(worker_result, Err(io::Error { .. })));
        }

        {
            let worker_result = worker_manager
                .create_worker({
                    let mut settings = WorkerSettings::default();

                    settings.dtls_files = Some(WorkerDtlsFiles {
                        certificate: "/notfound/cert.pem".into(),
                        private_key: "/notfound/priv.pem".into(),
                    });

                    settings
                })
                .await;

            assert!(matches!(worker_result, Err(io::Error { .. })));
        }
    });
}

#[test]
fn update_settings_succeeds() {
    future::block_on(async move {
        let worker_manager = init().await;

        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .expect("Failed to create worker with default settings");

        worker
            .update_settings({
                let mut settings = WorkerUpdateSettings::default();

                settings.log_level = Some(WorkerLogLevel::Debug);
                settings.log_tags = Some(vec![WorkerLogTag::Ice]);

                settings
            })
            .await
            .expect("Failed to update worker settings");
    });
}

#[test]
fn dump_succeeds() {
    future::block_on(async move {
        let worker_manager = init().await;

        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .expect("Failed to create worker with default settings");

        let dump = worker.dump().await.expect("Failed to dump worker");

        assert_eq!(dump.pid, worker.pid());
        assert_eq!(dump.router_ids, vec![]);
    });
}

#[test]
fn get_resource_usage_succeeds() {
    future::block_on(async move {
        let worker_manager = init().await;

        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .expect("Failed to create worker with default settings");

        worker
            .get_resource_usage()
            .await
            .expect("Failed to get worker's resource usage");
    });
}

#[test]
fn close_event() {
    future::block_on(async move {
        let worker_manager = init().await;

        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .expect("Failed to create worker with default settings");

        let (tx, rx) = async_oneshot::oneshot::<()>();
        let _handler = worker.on_close(move || {
            let _ = tx.send(());
        });
        drop(worker);

        rx.await.expect("Failed to receive close event");
    });
}

#[test]
fn emits_dead() {
    future::block_on(async move {
        let worker_manager = init().await;

        for &signal in &[libc::SIGINT, libc::SIGTERM, libc::SIGKILL] {
            let worker = worker_manager
                .create_worker(WorkerSettings::default())
                .await
                .expect("Failed to create worker with default settings");

            let (close_tx, close_rx) = async_oneshot::oneshot::<()>();
            let _handler = worker.on_close(move || {
                let _ = close_tx.send(());
            });

            let (dead_tx, dead_rx) = async_oneshot::oneshot::<()>();
            let _handler = worker.on_dead(move |_exit_status| {
                let _ = dead_tx.send(());
            });

            unsafe {
                libc::kill(worker.pid() as i32, signal);
            }

            dead_rx.await.expect("Failed to receive dead event");
            close_rx.await.expect("Failed to receive close event");

            assert_eq!(worker.closed(), true);
        }
    });
}

#[test]
fn ignores_pipe_hup_alrm_usr1_usr2_signals() {
    future::block_on(async move {
        let worker_manager = init().await;

        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .expect("Failed to create worker with default settings");

        let closed = Arc::new(AtomicBool::new(false));
        worker
            .on_close({
                let closed = Arc::clone(&closed);

                move || {
                    closed.store(true, Ordering::SeqCst);
                }
            })
            .detach();

        for &signal in &[
            libc::SIGPIPE,
            libc::SIGHUP,
            libc::SIGALRM,
            libc::SIGUSR1,
            libc::SIGUSR2,
        ] {
            unsafe {
                libc::kill(worker.pid() as i32, signal);
            }
        }

        // Wait for a bit for worker to respond if there is any response
        Timer::after(Duration::from_secs(2)).await;

        assert_eq!(closed.load(Ordering::SeqCst), false);
    });
}
