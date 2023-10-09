use async_io::Timer;
use futures_lite::future;
use mediasoup::audio_level_observer::AudioLevelObserverOptions;
use mediasoup::prelude::*;
use mediasoup::router::RouterOptions;
use mediasoup::rtp_parameters::{MimeTypeAudio, RtpCodecCapability, RtpCodecParametersParameters};
use mediasoup::worker::{Worker, WorkerSettings};
use mediasoup::worker_manager::WorkerManager;
use std::env;
use std::num::{NonZeroU32, NonZeroU8};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::time::Duration;

fn media_codecs() -> Vec<RtpCodecCapability> {
    vec![RtpCodecCapability::Audio {
        mime_type: MimeTypeAudio::Opus,
        preferred_payload_type: None,
        clock_rate: NonZeroU32::new(48000).unwrap(),
        channels: NonZeroU8::new(2).unwrap(),
        parameters: RtpCodecParametersParameters::from([
            ("useinbandfec", 1_u32.into()),
            ("foo", "bar".into()),
        ]),
        rtcp_feedback: vec![],
    }]
}

async fn init() -> Worker {
    {
        let mut builder = env_logger::builder();
        if env::var(env_logger::DEFAULT_FILTER_ENV).is_err() {
            builder.filter_level(log::LevelFilter::Off);
        }
        let _ = builder.is_test(true).try_init();
    }

    let worker_manager = WorkerManager::new();

    worker_manager
        .create_worker(WorkerSettings::default())
        .await
        .expect("Failed to create worker")
}

#[test]
fn create() {
    future::block_on(async move {
        let worker = init().await;

        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router");

        let new_observer_count = Arc::new(AtomicUsize::new(0));

        router
            .on_new_rtp_observer({
                let new_observer_count = Arc::clone(&new_observer_count);

                move |_new_rtp_observer| {
                    new_observer_count.fetch_add(1, Ordering::SeqCst);
                }
            })
            .detach();

        let audio_level_observer = router
            .create_audio_level_observer(AudioLevelObserverOptions::default())
            .await
            .expect("Failed to create AudioLevelObserver");

        assert_eq!(new_observer_count.load(Ordering::SeqCst), 1);
        assert!(!audio_level_observer.closed());
        assert!(!audio_level_observer.paused());

        let dump = router.dump().await.expect("Failed to get router dump");

        assert_eq!(
            dump.rtp_observer_ids.into_iter().collect::<Vec<_>>(),
            vec![audio_level_observer.id()]
        );
    });
}

#[test]
fn weak() {
    future::block_on(async move {
        let worker = init().await;

        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router");

        let audio_level_observer = router
            .create_audio_level_observer(AudioLevelObserverOptions::default())
            .await
            .expect("Failed to create AudioLevelObserver");

        let weak_audio_level_observer = audio_level_observer.downgrade();

        assert!(weak_audio_level_observer.upgrade().is_some());

        drop(audio_level_observer);

        assert!(weak_audio_level_observer.upgrade().is_none());
    });
}

#[test]
fn pause_resume() {
    future::block_on(async move {
        let worker = init().await;

        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router");

        let audio_level_observer = router
            .create_audio_level_observer(AudioLevelObserverOptions::default())
            .await
            .expect("Failed to create AudioLevelObserver");

        audio_level_observer.pause().await.expect("Failed to pause");
        assert!(audio_level_observer.paused());

        audio_level_observer
            .resume()
            .await
            .expect("Failed to resume");
        assert!(!audio_level_observer.paused());
    });
}

#[test]
fn close_event() {
    future::block_on(async move {
        let worker = init().await;

        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router");

        let audio_level_observer = router
            .create_audio_level_observer(AudioLevelObserverOptions::default())
            .await
            .expect("Failed to create AudioLevelObserver");

        let (mut tx, rx) = async_oneshot::oneshot::<()>();
        let _handler = audio_level_observer.on_close(Box::new(move || {
            let _ = tx.send(());
        }));
        drop(audio_level_observer);

        rx.await.expect("Failed to receive close event");
    });
}

#[test]
fn drop_test() {
    future::block_on(async move {
        let worker = init().await;

        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router");

        let _audio_level_observer = router
            .create_audio_level_observer(AudioLevelObserverOptions::default())
            .await
            .expect("Failed to create AudioLevelObserver");

        let audio_level_observer_2 = router
            .create_audio_level_observer(AudioLevelObserverOptions::default())
            .await
            .expect("Failed to create AudioLevelObserver");

        let dump = router.dump().await.expect("Failed to get router dump");

        assert_eq!(dump.rtp_observer_ids.len(), 2);

        drop(audio_level_observer_2);

        // Drop is async, give it a bit of time to finish
        Timer::after(Duration::from_millis(200)).await;

        let dump = router.dump().await.expect("Failed to get router dump");

        assert_eq!(dump.rtp_observer_ids.len(), 1);
    });
}
