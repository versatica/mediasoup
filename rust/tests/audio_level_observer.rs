mod audio_level_observer {
    use async_io::Timer;
    use futures_lite::future;
    use mediasoup::audio_level_observer::AudioLevelObserverOptions;
    use mediasoup::router::{Router, RouterOptions};
    use mediasoup::rtp_observer::RtpObserver;
    use mediasoup::rtp_parameters::{
        MimeTypeAudio, RtpCodecCapability, RtpCodecParametersParametersValue,
    };
    use mediasoup::worker::WorkerSettings;
    use mediasoup::worker_manager::WorkerManager;
    use std::collections::BTreeMap;
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
            parameters: {
                let mut parameters = BTreeMap::new();
                parameters.insert(
                    "userinbandfec".to_string(),
                    RtpCodecParametersParametersValue::Number(1),
                );
                parameters.insert(
                    "foo".to_string(),
                    RtpCodecParametersParametersValue::String("bar".to_string()),
                );
                parameters
            },
            rtcp_feedback: vec![],
        }]
    }

    async fn init() -> Router {
        {
            let mut builder = env_logger::builder();
            if env::var(env_logger::DEFAULT_FILTER_ENV).is_err() {
                builder.filter_level(log::LevelFilter::Off);
            }
            let _ = builder.is_test(true).try_init();
        }

        let worker_manager = WorkerManager::new(
            env::var("MEDIASOUP_WORKER_BIN")
                .map(|path| path.into())
                .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into()),
        );

        let worker_settings = WorkerSettings::default();

        let worker = worker_manager
            .create_worker(worker_settings)
            .await
            .expect("Failed to create worker");

        worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router")
    }

    #[test]
    fn create() {
        future::block_on(async move {
            let router = init().await;

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
            assert_eq!(audio_level_observer.paused(), false);

            let dump = router.dump().await.expect("Failed to get router dump");

            assert_eq!(
                dump.rtp_observer_ids.into_iter().collect::<Vec<_>>(),
                vec![audio_level_observer.id()]
            );
        });
    }

    #[test]
    fn pause_resume() {
        future::block_on(async move {
            let router = init().await;

            let audio_level_observer = router
                .create_audio_level_observer(AudioLevelObserverOptions::default())
                .await
                .expect("Failed to create AudioLevelObserver");

            audio_level_observer.pause().await.expect("Failed to pause");
            assert_eq!(audio_level_observer.paused(), true);

            audio_level_observer
                .resume()
                .await
                .expect("Failed to resume");
            assert_eq!(audio_level_observer.paused(), false);
        });
    }

    #[test]
    fn drop_test() {
        future::block_on(async move {
            let router = init().await;

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
}
