mod producer {
    use async_io::Timer;
    use futures_lite::future;
    use mediasoup::data_structures::{AppData, TransportListenIp};
    use mediasoup::producer::{ProducerOptions, ProducerTraceEventType, ProducerType};
    use mediasoup::router::{Router, RouterOptions};
    use mediasoup::rtp_parameters::{
        MediaKind, MimeTypeAudio, MimeTypeVideo, RtcpFeedback, RtcpParameters, RtpCodecCapability,
        RtpCodecParameters, RtpCodecParametersParametersValue, RtpEncodingParameters,
        RtpEncodingParametersRtx, RtpHeaderExtensionParameters, RtpHeaderExtensionUri,
        RtpParameters,
    };
    use mediasoup::transport::{ProduceError, Transport, TransportGeneric};
    use mediasoup::webrtc_transport::{
        TransportListenIps, WebRtcTransport, WebRtcTransportOptions,
    };
    use mediasoup::worker::WorkerSettings;
    use mediasoup::worker_manager::WorkerManager;
    use std::collections::{BTreeMap, HashMap, HashSet};
    use std::env;
    use std::num::{NonZeroU32, NonZeroU8};
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;
    use std::time::Duration;

    struct ProducerAppData {
        foo: i32,
        bar: &'static str,
    }

    fn media_codecs() -> Vec<RtpCodecCapability> {
        vec![
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::Opus,
                preferred_payload_type: None,
                clock_rate: NonZeroU32::new(48000).unwrap(),
                channels: NonZeroU8::new(2).unwrap(),
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "foo".to_string(),
                        RtpCodecParametersParametersValue::String("111".to_string()),
                    );
                    parameters
                },
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::VP8,
                preferred_payload_type: None,
                clock_rate: NonZeroU32::new(90000).unwrap(),
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::H264,
                preferred_payload_type: None,
                clock_rate: NonZeroU32::new(90000).unwrap(),
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "level-asymmetry-allowed".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters.insert(
                        "packetization-mode".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters.insert(
                        "profile-level-id".to_string(),
                        RtpCodecParametersParametersValue::String("4d0032".to_string()),
                    );
                    parameters.insert(
                        "foo".to_string(),
                        RtpCodecParametersParametersValue::String("bar".to_string()),
                    );
                    parameters
                },
                rtcp_feedback: vec![],
            },
        ]
    }

    fn audio_producer_options() -> ProducerOptions {
        let mut options = ProducerOptions::new(MediaKind::Audio, {
            let mut parameters = RtpParameters::default();
            parameters.mid = Some("AUDIO".to_string());
            parameters.codecs = vec![RtpCodecParameters::Audio {
                mime_type: MimeTypeAudio::Opus,
                payload_type: 0,
                clock_rate: NonZeroU32::new(48000).unwrap(),
                channels: NonZeroU8::new(2).unwrap(),
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "useinbandfec".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters.insert(
                        "usedtx".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters.insert(
                        "foo".to_string(),
                        RtpCodecParametersParametersValue::String("222.222".to_string()),
                    );
                    parameters.insert(
                        "bar".to_string(),
                        RtpCodecParametersParametersValue::String("333".to_string()),
                    );
                    parameters
                },
                rtcp_feedback: vec![],
            }];
            parameters.header_extensions = vec![
                RtpHeaderExtensionParameters {
                    uri: RtpHeaderExtensionUri::SDES,
                    id: 10,
                    encrypt: false,
                    parameters: Default::default(),
                },
                RtpHeaderExtensionParameters {
                    uri: RtpHeaderExtensionUri::AudioLevel,
                    id: 12,
                    encrypt: false,
                    parameters: Default::default(),
                },
            ];
            // Missing encodings on purpose.
            parameters.rtcp = {
                let mut rtcp = RtcpParameters::default();
                rtcp.cname = Some("audio-1".to_string());
                rtcp
            };
            parameters
        });

        options.app_data = AppData::new(ProducerAppData { foo: 1, bar: "2" });

        options
    }

    fn video_producer_options() -> ProducerOptions {
        let mut options = ProducerOptions::new(MediaKind::Video, {
            let mut parameters = RtpParameters::default();
            parameters.mid = Some("VIDEO".to_string());
            parameters.codecs = vec![
                RtpCodecParameters::Video {
                    mime_type: MimeTypeVideo::H264,
                    payload_type: 112,
                    clock_rate: NonZeroU32::new(90000).unwrap(),
                    parameters: {
                        let mut parameters = BTreeMap::new();
                        parameters.insert(
                            "packetization-mode".to_string(),
                            RtpCodecParametersParametersValue::Number(1),
                        );
                        parameters.insert(
                            "profile-level-id".to_string(),
                            RtpCodecParametersParametersValue::String("4d0032".to_string()),
                        );
                        parameters
                    },
                    rtcp_feedback: vec![
                        RtcpFeedback::Nack,
                        RtcpFeedback::NackPli,
                        RtcpFeedback::GoogRemb,
                    ],
                },
                RtpCodecParameters::Video {
                    mime_type: MimeTypeVideo::RTX,
                    payload_type: 113,
                    clock_rate: NonZeroU32::new(90000).unwrap(),
                    parameters: {
                        let mut parameters = BTreeMap::new();
                        parameters.insert(
                            "apt".to_string(),
                            RtpCodecParametersParametersValue::Number(112),
                        );
                        parameters
                    },
                    rtcp_feedback: vec![],
                },
            ];
            parameters.header_extensions = vec![
                RtpHeaderExtensionParameters {
                    uri: RtpHeaderExtensionUri::SDES,
                    id: 10,
                    encrypt: false,
                    parameters: Default::default(),
                },
                RtpHeaderExtensionParameters {
                    uri: RtpHeaderExtensionUri::VideoOrientation,
                    id: 13,
                    encrypt: false,
                    parameters: Default::default(),
                },
            ];
            parameters.encodings = vec![
                {
                    let mut encoding = RtpEncodingParameters::default();
                    encoding.ssrc = Some(22222222);
                    encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 22222223 });
                    encoding.scalability_mode = Some("L1T3".to_string());
                    encoding
                },
                {
                    let mut encoding = RtpEncodingParameters::default();
                    encoding.ssrc = Some(22222224);
                    encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 22222225 });
                    encoding
                },
                {
                    let mut encoding = RtpEncodingParameters::default();
                    encoding.ssrc = Some(22222226);
                    encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 22222227 });
                    encoding
                },
                {
                    let mut encoding = RtpEncodingParameters::default();
                    encoding.ssrc = Some(22222228);
                    encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 22222229 });
                    encoding
                },
            ];
            parameters.rtcp = {
                let mut rtcp = RtcpParameters::default();
                rtcp.cname = Some("video-1".to_string());
                rtcp
            };
            parameters
        });

        options.app_data = AppData::new(ProducerAppData { foo: 1, bar: "2" });

        options
    }

    async fn init() -> (Router, WebRtcTransport, WebRtcTransport) {
        let _ = env_logger::builder().is_test(true).try_init();

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

        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .expect("Failed to create router");

        let transport_options =
            WebRtcTransportOptions::new(TransportListenIps::new(TransportListenIp {
                ip: "127.0.0.1".parse().unwrap(),
                announced_ip: None,
            }));

        let transport_1 = router
            .create_webrtc_transport(transport_options.clone())
            .await
            .expect("Failed to create transport1");

        let transport_2 = router
            .create_webrtc_transport(transport_options)
            .await
            .expect("Failed to create transport1");

        (router, transport_1, transport_2)
    }

    #[test]
    fn produce_succeeds() {
        future::block_on(async move {
            let (router, transport_1, transport_2) = init().await;

            {
                let new_producers_count = Arc::new(AtomicUsize::new(0));

                transport_1
                    .on_new_producer({
                        let new_producers_count = Arc::clone(&new_producers_count);

                        move |_consumer| {
                            new_producers_count.fetch_add(1, Ordering::SeqCst);
                        }
                    })
                    .detach();

                let audio_producer = transport_1
                    .produce(audio_producer_options())
                    .await
                    .expect("Failed to produce audio");

                assert_eq!(new_producers_count.load(Ordering::SeqCst), 1);
                assert_eq!(audio_producer.kind(), MediaKind::Audio);
                assert_eq!(audio_producer.r#type(), ProducerType::Simple);
                assert_eq!(audio_producer.paused(), false);
                assert_eq!(audio_producer.score(), vec![]);
                assert_eq!(
                    audio_producer
                        .app_data()
                        .downcast_ref::<ProducerAppData>()
                        .unwrap()
                        .foo,
                    1
                );
                assert_eq!(
                    audio_producer
                        .app_data()
                        .downcast_ref::<ProducerAppData>()
                        .unwrap()
                        .bar,
                    "2"
                );

                let router_dump = router.dump().await.expect("Failed to get router dump");

                assert_eq!(router_dump.map_producer_id_consumer_ids, {
                    let mut map = HashMap::new();
                    map.insert(audio_producer.id(), HashSet::new());
                    map
                });
                assert_eq!(router_dump.map_consumer_id_producer_id, HashMap::new());

                let transport_1_dump = transport_1
                    .dump()
                    .await
                    .expect("Failed to get transport 1 dump");

                assert_eq!(transport_1_dump.producer_ids, vec![audio_producer.id()]);
                assert_eq!(transport_1_dump.consumer_ids, vec![]);
            }

            {
                let new_producers_count = Arc::new(AtomicUsize::new(0));

                transport_2
                    .on_new_producer({
                        let new_producers_count = Arc::clone(&new_producers_count);

                        move |_consumer| {
                            new_producers_count.fetch_add(1, Ordering::SeqCst);
                        }
                    })
                    .detach();

                let video_producer = transport_2
                    .produce(video_producer_options())
                    .await
                    .expect("Failed to produce video");

                assert_eq!(new_producers_count.load(Ordering::SeqCst), 1);
                assert_eq!(video_producer.kind(), MediaKind::Video);
                assert_eq!(video_producer.r#type(), ProducerType::Simulcast);
                assert_eq!(video_producer.paused(), false);
                assert_eq!(video_producer.score(), vec![]);
                assert_eq!(
                    video_producer
                        .app_data()
                        .downcast_ref::<ProducerAppData>()
                        .unwrap()
                        .foo,
                    1
                );
                assert_eq!(
                    video_producer
                        .app_data()
                        .downcast_ref::<ProducerAppData>()
                        .unwrap()
                        .bar,
                    "2"
                );

                let router_dump = router.dump().await.expect("Failed to get router dump");

                assert_eq!(router_dump.map_producer_id_consumer_ids, {
                    let mut map = HashMap::new();
                    map.insert(video_producer.id(), HashSet::new());
                    map
                });
                assert_eq!(router_dump.map_consumer_id_producer_id, HashMap::new());

                let transport_2_dump = transport_2
                    .dump()
                    .await
                    .expect("Failed to get transport 2 dump");

                assert_eq!(transport_2_dump.producer_ids, vec![video_producer.id()]);
                assert_eq!(transport_2_dump.consumer_ids, vec![]);
            }
        });
    }

    #[test]
    fn produce_wrong_arguments() {
        future::block_on(async move {
            let (_router, transport_1, _transport_2) = init().await;

            // Empty rtp_parameters.codecs.
            assert!(matches!(
                transport_1
                    .produce(ProducerOptions::new(
                        MediaKind::Audio,
                        RtpParameters::default()
                    ))
                    .await,
                Err(ProduceError::Request(_)),
            ));

            {
                // Empty rtp_parameters.encodings.
                let produce_result = transport_1
                    .produce(ProducerOptions::new(MediaKind::Video, {
                        let mut parameters = RtpParameters::default();
                        parameters.codecs = vec![
                            RtpCodecParameters::Video {
                                mime_type: MimeTypeVideo::H264,
                                payload_type: 112,
                                clock_rate: NonZeroU32::new(90000).unwrap(),
                                parameters: {
                                    let mut parameters = BTreeMap::new();
                                    parameters.insert(
                                        "packetization-mode".to_string(),
                                        RtpCodecParametersParametersValue::Number(1),
                                    );
                                    parameters.insert(
                                        "profile-level-id".to_string(),
                                        RtpCodecParametersParametersValue::String(
                                            "4d0032".to_string(),
                                        ),
                                    );
                                    parameters
                                },
                                rtcp_feedback: vec![],
                            },
                            RtpCodecParameters::Video {
                                mime_type: MimeTypeVideo::RTX,
                                payload_type: 113,
                                clock_rate: NonZeroU32::new(90000).unwrap(),
                                parameters: {
                                    let mut parameters = BTreeMap::new();
                                    parameters.insert(
                                        "apt".to_string(),
                                        RtpCodecParametersParametersValue::Number(112),
                                    );
                                    parameters
                                },
                                rtcp_feedback: vec![],
                            },
                        ];
                        parameters.rtcp = {
                            let mut rtcp = RtcpParameters::default();
                            rtcp.cname = Some("qwerty".to_string());
                            rtcp
                        };
                        parameters
                    }))
                    .await;

                assert!(matches!(produce_result, Err(ProduceError::Request(_))));
            }

            {
                // Wrong apt in RTX codec.
                let produce_result = transport_1
                    .produce(ProducerOptions::new(MediaKind::Video, {
                        let mut parameters = RtpParameters::default();
                        parameters.codecs = vec![
                            RtpCodecParameters::Video {
                                mime_type: MimeTypeVideo::H264,
                                payload_type: 112,
                                clock_rate: NonZeroU32::new(90000).unwrap(),
                                parameters: {
                                    let mut parameters = BTreeMap::new();
                                    parameters.insert(
                                        "packetization-mode".to_string(),
                                        RtpCodecParametersParametersValue::Number(1),
                                    );
                                    parameters.insert(
                                        "profile-level-id".to_string(),
                                        RtpCodecParametersParametersValue::String(
                                            "4d0032".to_string(),
                                        ),
                                    );
                                    parameters
                                },
                                rtcp_feedback: vec![],
                            },
                            RtpCodecParameters::Video {
                                mime_type: MimeTypeVideo::RTX,
                                payload_type: 113,
                                clock_rate: NonZeroU32::new(90000).unwrap(),
                                parameters: {
                                    let mut parameters = BTreeMap::new();
                                    parameters.insert(
                                        "apt".to_string(),
                                        RtpCodecParametersParametersValue::Number(111),
                                    );
                                    parameters
                                },
                                rtcp_feedback: vec![],
                            },
                        ];
                        parameters.encodings = vec![{
                            let mut encoding = RtpEncodingParameters::default();
                            encoding.ssrc = Some(6666);
                            encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 6667 });
                            encoding
                        }];
                        parameters.rtcp = {
                            let mut rtcp = RtcpParameters::default();
                            rtcp.cname = Some("video-1".to_string());
                            rtcp
                        };
                        parameters
                    }))
                    .await;

                assert!(matches!(
                    produce_result,
                    Err(ProduceError::FailedRtpParametersMapping(_))
                ));
            }
        });
    }

    #[test]
    fn produce_unsupported_codecs() {
        future::block_on(async move {
            let (_router, transport_1, _transport_2) = init().await;

            // Empty rtp_parameters.codecs.
            assert!(matches!(
                transport_1
                    .produce(ProducerOptions::new(MediaKind::Audio, {
                        let mut parameters = RtpParameters::default();
                        parameters.codecs = vec![RtpCodecParameters::Audio {
                            mime_type: MimeTypeAudio::ISAC,
                            payload_type: 108,
                            clock_rate: NonZeroU32::new(32000).unwrap(),
                            channels: NonZeroU8::new(1).unwrap(),
                            parameters: BTreeMap::new(),
                            rtcp_feedback: vec![],
                        }];
                        parameters.header_extensions = vec![];
                        parameters.encodings = vec![{
                            let mut encoding = RtpEncodingParameters::default();
                            encoding.ssrc = Some(1111);
                            encoding
                        }];
                        parameters.rtcp = {
                            let mut rtcp = RtcpParameters::default();
                            rtcp.cname = Some("audio".to_string());
                            rtcp
                        };
                        parameters
                    }))
                    .await,
                Err(ProduceError::FailedRtpParametersMapping(_)),
            ));

            {
                // Invalid H264 profile-level-id.
                let produce_result = transport_1
                    .produce(ProducerOptions::new(MediaKind::Video, {
                        let mut parameters = RtpParameters::default();
                        parameters.codecs = vec![
                            RtpCodecParameters::Video {
                                mime_type: MimeTypeVideo::H264,
                                payload_type: 112,
                                clock_rate: NonZeroU32::new(90000).unwrap(),
                                parameters: {
                                    let mut parameters = BTreeMap::new();
                                    parameters.insert(
                                        "packetization-mode".to_string(),
                                        RtpCodecParametersParametersValue::Number(1),
                                    );
                                    parameters.insert(
                                        "profile-level-id".to_string(),
                                        RtpCodecParametersParametersValue::String(
                                            "CHICKEN".to_string(),
                                        ),
                                    );
                                    parameters
                                },
                                rtcp_feedback: vec![],
                            },
                            RtpCodecParameters::Video {
                                mime_type: MimeTypeVideo::RTX,
                                payload_type: 113,
                                clock_rate: NonZeroU32::new(90000).unwrap(),
                                parameters: {
                                    let mut parameters = BTreeMap::new();
                                    parameters.insert(
                                        "apt".to_string(),
                                        RtpCodecParametersParametersValue::Number(112),
                                    );
                                    parameters
                                },
                                rtcp_feedback: vec![],
                            },
                        ];
                        parameters.encodings = vec![{
                            let mut encoding = RtpEncodingParameters::default();
                            encoding.ssrc = Some(6666);
                            encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 6667 });
                            encoding
                        }];
                        parameters
                    }))
                    .await;

                // TODO: unlock once h264-profile-level-id is ported to Rust and this starts to fail
                // assert!(matches!(
                //     produce_result,
                //     Err(ProduceError::FailedRtpParametersMapping(_))
                // ));
            }
        });
    }

    #[test]
    fn produce_already_used_mid_ssrc() {
        future::block_on(async move {
            let (_router, transport_1, transport_2) = init().await;

            {
                let _first_producer = transport_1
                    .produce(audio_producer_options())
                    .await
                    .expect("Failed to produce audio");

                let produce_result = transport_1
                    .produce(ProducerOptions::new(MediaKind::Audio, {
                        let mut parameters = RtpParameters::default();
                        parameters.mid = Some("AUDIO".to_string());
                        parameters.codecs = vec![RtpCodecParameters::Audio {
                            mime_type: MimeTypeAudio::Opus,
                            payload_type: 0,
                            clock_rate: NonZeroU32::new(48000).unwrap(),
                            channels: NonZeroU8::new(2).unwrap(),
                            parameters: BTreeMap::new(),
                            rtcp_feedback: vec![],
                        }];
                        parameters.header_extensions = vec![];
                        parameters.encodings = vec![{
                            let mut encoding = RtpEncodingParameters::default();
                            encoding.ssrc = Some(33333333);
                            encoding
                        }];
                        parameters.rtcp = {
                            let mut rtcp = RtcpParameters::default();
                            rtcp.cname = Some("audio-2".to_string());
                            rtcp
                        };
                        parameters
                    }))
                    .await;

                assert!(matches!(produce_result, Err(ProduceError::Request(_)),));
            }

            {
                let _first_producer = transport_2
                    .produce(video_producer_options())
                    .await
                    .expect("Failed to produce video");

                // Invalid H264 profile-level-id.
                let produce_result = transport_2
                    .produce(ProducerOptions::new(MediaKind::Video, {
                        let mut parameters = RtpParameters::default();
                        parameters.mid = Some("VIDEO2".to_string());
                        parameters.codecs = vec![RtpCodecParameters::Video {
                            mime_type: MimeTypeVideo::VP8,
                            payload_type: 112,
                            clock_rate: NonZeroU32::new(90000).unwrap(),
                            parameters: BTreeMap::new(),
                            rtcp_feedback: vec![],
                        }];
                        parameters.encodings = vec![{
                            let mut encoding = RtpEncodingParameters::default();
                            encoding.ssrc = Some(22222222);
                            encoding.rtx = Some(RtpEncodingParametersRtx { ssrc: 6667 });
                            encoding
                        }];
                        parameters
                    }))
                    .await;

                assert!(matches!(produce_result, Err(ProduceError::Request(_))));
            }
        });
    }

    #[test]
    fn produce_no_mid_single_encoding_without_dir_or_ssrc() {
        future::block_on(async move {
            let (_router, transport_1, _transport_2) = init().await;

            let produce_result = transport_1
                .produce(ProducerOptions::new(MediaKind::Audio, {
                    let mut parameters = RtpParameters::default();
                    parameters.codecs = vec![RtpCodecParameters::Audio {
                        mime_type: MimeTypeAudio::Opus,
                        payload_type: 111,
                        clock_rate: NonZeroU32::new(48000).unwrap(),
                        channels: NonZeroU8::new(2).unwrap(),
                        parameters: BTreeMap::new(),
                        rtcp_feedback: vec![],
                    }];
                    parameters.header_extensions = vec![];
                    parameters.encodings = vec![RtpEncodingParameters::default()];
                    parameters.rtcp = {
                        let mut rtcp = RtcpParameters::default();
                        rtcp.cname = Some("audio-2".to_string());
                        rtcp
                    };
                    parameters
                }))
                .await;

            assert!(matches!(produce_result, Err(ProduceError::Request(_)),));
        });
    }

    #[test]
    fn dump_succeeds() {
        future::block_on(async move {
            let (_router, transport_1, transport_2) = init().await;

            {
                let audio_producer = transport_1
                    .produce(audio_producer_options())
                    .await
                    .expect("Failed to produce audio");

                let dump = audio_producer
                    .dump()
                    .await
                    .expect("Failed to dump audio producer");

                assert_eq!(dump.id, audio_producer.id());
                assert_eq!(dump.kind, audio_producer.kind());
                assert_eq!(
                    dump.rtp_parameters.codecs,
                    audio_producer_options().rtp_parameters.codecs,
                );
                assert_eq!(
                    dump.rtp_parameters.header_extensions,
                    audio_producer_options().rtp_parameters.header_extensions,
                );
                assert_eq!(
                    dump.rtp_parameters.encodings,
                    vec![RtpEncodingParameters {
                        ssrc: None,
                        rid: None,
                        codec_payload_type: Some(0),
                        rtx: None,
                        dtx: None,
                        scalability_mode: None,
                        scale_resolution_down_by: None,
                        max_bitrate: None
                    }],
                );
                assert_eq!(dump.r#type, ProducerType::Simple);
            }

            {
                let video_producer = transport_2
                    .produce(video_producer_options())
                    .await
                    .expect("Failed to produce video");

                let dump = video_producer
                    .dump()
                    .await
                    .expect("Failed to dump video producer");

                assert_eq!(dump.id, video_producer.id());
                assert_eq!(dump.kind, video_producer.kind());
                assert_eq!(
                    dump.rtp_parameters.codecs,
                    video_producer_options().rtp_parameters.codecs,
                );
                assert_eq!(
                    dump.rtp_parameters.header_extensions,
                    video_producer_options().rtp_parameters.header_extensions,
                );
                assert_eq!(
                    dump.rtp_parameters.encodings,
                    vec![
                        RtpEncodingParameters {
                            ssrc: Some(22222222),
                            rid: None,
                            codec_payload_type: Some(112),
                            rtx: Some(RtpEncodingParametersRtx { ssrc: 22222223 }),
                            dtx: None,
                            scalability_mode: Some("L1T3".to_string()),
                            scale_resolution_down_by: None,
                            max_bitrate: None
                        },
                        RtpEncodingParameters {
                            ssrc: Some(22222224),
                            rid: None,
                            codec_payload_type: Some(112),
                            rtx: Some(RtpEncodingParametersRtx { ssrc: 22222225 }),
                            dtx: None,
                            scalability_mode: None,
                            scale_resolution_down_by: None,
                            max_bitrate: None
                        },
                        RtpEncodingParameters {
                            ssrc: Some(22222226),
                            rid: None,
                            codec_payload_type: Some(112),
                            rtx: Some(RtpEncodingParametersRtx { ssrc: 22222227 }),
                            dtx: None,
                            scalability_mode: None,
                            scale_resolution_down_by: None,
                            max_bitrate: None
                        },
                        RtpEncodingParameters {
                            ssrc: Some(22222228),
                            rid: None,
                            codec_payload_type: Some(112),
                            rtx: Some(RtpEncodingParametersRtx { ssrc: 22222229 }),
                            dtx: None,
                            scalability_mode: None,
                            scale_resolution_down_by: None,
                            max_bitrate: None
                        },
                    ],
                );
                assert_eq!(dump.r#type, ProducerType::Simulcast);
            }
        });
    }

    #[test]
    fn get_stats_succeeds() {
        future::block_on(async move {
            let (_router, transport_1, transport_2) = init().await;

            {
                let audio_producer = transport_1
                    .produce(audio_producer_options())
                    .await
                    .expect("Failed to produce audio");

                let stats = audio_producer
                    .get_stats()
                    .await
                    .expect("Failed to get stats on audio producer");

                assert_eq!(stats, vec![]);
            }

            {
                let video_producer = transport_2
                    .produce(video_producer_options())
                    .await
                    .expect("Failed to produce video");

                let stats = video_producer
                    .get_stats()
                    .await
                    .expect("Failed to get stats on video producer");

                assert_eq!(stats, vec![]);
            }
        });
    }

    #[test]
    fn pause_resume_succeeds() {
        future::block_on(async move {
            let (_router, transport_1, _transport_2) = init().await;

            let audio_producer = transport_1
                .produce(audio_producer_options())
                .await
                .expect("Failed to produce audio");

            {
                audio_producer
                    .pause()
                    .await
                    .expect("Failed to pause audio producer");

                assert_eq!(audio_producer.paused(), true);

                let dump = audio_producer
                    .dump()
                    .await
                    .expect("Failed to dump audio producer");

                assert_eq!(dump.paused, true);
            }

            {
                audio_producer
                    .resume()
                    .await
                    .expect("Failed to resume audio producer");

                assert_eq!(audio_producer.paused(), false);

                let dump = audio_producer
                    .dump()
                    .await
                    .expect("Failed to dump audio producer");

                assert_eq!(dump.paused, false);
            }
        });
    }

    #[test]
    fn enable_trace_event_succeeds() {
        future::block_on(async move {
            let (_router, transport_1, _transport_2) = init().await;

            let audio_producer = transport_1
                .produce(audio_producer_options())
                .await
                .expect("Failed to produce audio");

            {
                audio_producer
                    .enable_trace_event(vec![
                        ProducerTraceEventType::RTP,
                        ProducerTraceEventType::PLI,
                    ])
                    .await
                    .expect("Failed to enable trace event");

                let dump = audio_producer
                    .dump()
                    .await
                    .expect("Failed to dump audio producer");

                assert_eq!(dump.trace_event_types.as_str(), "rtp,pli");
            }

            {
                audio_producer
                    .enable_trace_event(vec![])
                    .await
                    .expect("Failed to enable trace event");

                let dump = audio_producer
                    .dump()
                    .await
                    .expect("Failed to dump audio producer");

                assert_eq!(dump.trace_event_types.as_str(), "");
            }
        });
    }

    #[test]
    fn close_event() {
        future::block_on(async move {
            let (router, transport_1, _transport_2) = init().await;

            let audio_producer = transport_1
                .produce(audio_producer_options())
                .await
                .expect("Failed to produce audio");

            {
                let (tx, rx) = async_oneshot::oneshot::<()>();
                let _handler = audio_producer.on_close(move || {
                    let _ = tx.send(());
                });
                drop(audio_producer);

                rx.await.expect("Failed to receive close event");
            }

            // Drop is async, give producer a bit of time to finish
            Timer::after(Duration::from_millis(200)).await;

            {
                let dump = router.dump().await.expect("Failed to dump router");

                assert_eq!(dump.map_producer_id_consumer_ids, HashMap::new());
                assert_eq!(dump.map_consumer_id_producer_id, HashMap::new());
            }

            {
                let dump = transport_1.dump().await.expect("Failed to dump transport");

                assert_eq!(dump.producer_ids, vec![]);
                assert_eq!(dump.consumer_ids, vec![]);
            }
        });
    }
}
