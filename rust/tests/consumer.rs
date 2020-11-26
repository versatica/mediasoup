mod consumer {
    use futures_lite::future;
    use mediasoup::data_structures::{AppData, TransportListenIp};
    use mediasoup::router::consumer::{
        ConsumerLayers, ConsumerOptions, ConsumerScore, ConsumerType,
    };
    use mediasoup::router::producer::ProducerOptions;
    use mediasoup::router::webrtc_transport::{
        TransportListenIps, WebRtcTransport, WebRtcTransportOptions,
    };
    use mediasoup::router::{Router, RouterOptions};
    use mediasoup::rtp_parameters::{
        MediaKind, MimeTypeAudio, MimeTypeVideo, RtcpFeedback, RtcpParameters, RtpCapabilities,
        RtpCodecCapability, RtpCodecParameters, RtpCodecParametersParametersValue,
        RtpEncodingParameters, RtpEncodingParametersRtx, RtpHeaderExtension,
        RtpHeaderExtensionDirection, RtpHeaderExtensionParameters, RtpHeaderExtensionUri,
        RtpParameters,
    };
    use mediasoup::transport::{Transport, TransportGeneric};
    use mediasoup::worker::WorkerSettings;
    use mediasoup::worker_manager::WorkerManager;
    use std::collections::{BTreeMap, HashMap, HashSet};
    use std::env;
    use std::num::{NonZeroU32, NonZeroU8};
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    struct ProducerAppData {
        foo: i32,
        bar: &'static str,
    }

    struct ConsumerAppData {
        baz: &'static str,
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
                        RtpCodecParametersParametersValue::String("bar".to_string()),
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
                payload_type: 111,
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
            parameters.encodings = vec![{
                let mut encoding = RtpEncodingParameters::default();
                encoding.ssrc = Some(11111111);
                encoding
            }];
            parameters.rtcp = {
                let mut rtcp = RtcpParameters::default();
                rtcp.cname = Some("FOOBAR".to_string());
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
                rtcp.cname = Some("FOOBAR".to_string());
                rtcp
            };
            parameters
        });

        options.app_data = AppData::new(ProducerAppData { foo: 1, bar: "2" });

        options
    }

    fn consumer_device_capabilities() -> RtpCapabilities {
        RtpCapabilities {
            codecs: vec![
                RtpCodecCapability::Audio {
                    mime_type: MimeTypeAudio::Opus,
                    preferred_payload_type: Some(100),
                    clock_rate: NonZeroU32::new(48000).unwrap(),
                    channels: NonZeroU8::new(2).unwrap(),
                    parameters: BTreeMap::new(),
                    rtcp_feedback: vec![],
                },
                RtpCodecCapability::Video {
                    mime_type: MimeTypeVideo::H264,
                    preferred_payload_type: Some(101),
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
                        parameters
                    },
                    rtcp_feedback: vec![
                        RtcpFeedback::Nack,
                        RtcpFeedback::NackPli,
                        RtcpFeedback::CcmFir,
                        RtcpFeedback::GoogRemb,
                    ],
                },
                RtpCodecCapability::Video {
                    mime_type: MimeTypeVideo::RTX,
                    preferred_payload_type: Some(102),
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
            ],
            header_extensions: vec![
                RtpHeaderExtension {
                    kind: Some(MediaKind::Audio),
                    uri: RtpHeaderExtensionUri::SDES,
                    preferred_id: 1,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Video),
                    uri: RtpHeaderExtensionUri::SDES,
                    preferred_id: 1,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Video),
                    uri: RtpHeaderExtensionUri::RtpStreamId,
                    preferred_id: 2,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Audio),
                    uri: RtpHeaderExtensionUri::AbsSendTime,
                    preferred_id: 4,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Video),
                    uri: RtpHeaderExtensionUri::AbsSendTime,
                    preferred_id: 4,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Audio),
                    uri: RtpHeaderExtensionUri::AudioLevel,
                    preferred_id: 10,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Video),
                    uri: RtpHeaderExtensionUri::VideoOrientation,
                    preferred_id: 11,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
                RtpHeaderExtension {
                    kind: Some(MediaKind::Video),
                    uri: RtpHeaderExtensionUri::TimeOffset,
                    preferred_id: 12,
                    preferred_encrypt: false,
                    direction: RtpHeaderExtensionDirection::default(),
                },
            ],
            fec_mechanisms: vec![],
        }
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
    fn consume_succeeds() {
        future::block_on(async move {
            let (router, transport_1, transport_2) = init().await;

            let audio_producer = transport_1
                .produce(audio_producer_options())
                .await
                .expect("Failed to produce audio");

            let video_producer = transport_1
                .produce(video_producer_options())
                .await
                .expect("Failed to produce video");

            video_producer
                .pause()
                .await
                .expect("Failed to pause video producer");

            let new_consumer_count = Arc::new(AtomicUsize::new(0));

            transport_2
                .on_new_consumer({
                    let new_consumer_count = Arc::clone(&new_consumer_count);

                    move |_consumer| {
                        new_consumer_count.fetch_add(1, Ordering::SeqCst);
                    }
                })
                .detach();

            let consumer_device_capabilities = consumer_device_capabilities();

            assert!(router.can_consume(&audio_producer.id(), &consumer_device_capabilities));

            let audio_consumer = transport_2
                .consume({
                    let mut options = ConsumerOptions::new(
                        audio_producer.id(),
                        consumer_device_capabilities.clone(),
                    );
                    options.app_data = AppData::new(ConsumerAppData { baz: "LOL" });
                    options
                })
                .await
                .expect("Failed to consume audio");

            assert_eq!(new_consumer_count.load(Ordering::SeqCst), 1);
            assert_eq!(audio_consumer.producer_id(), audio_producer.id());
            assert_eq!(audio_consumer.kind(), MediaKind::Audio);
            assert_eq!(audio_consumer.rtp_parameters().mid, Some("0".to_string()));
            assert_eq!(
                audio_consumer.rtp_parameters().codecs,
                vec![RtpCodecParameters::Audio {
                    mime_type: MimeTypeAudio::Opus,
                    payload_type: 100,
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
                }]
            );
            assert_eq!(audio_consumer.r#type(), ConsumerType::Simple);
            assert_eq!(audio_consumer.paused(), false);
            assert_eq!(audio_consumer.producer_paused(), false);
            assert_eq!(audio_consumer.priority(), 1);
            assert_eq!(
                audio_consumer.score(),
                ConsumerScore {
                    score: 10,
                    producer_score: 0,
                    producer_scores: vec![0]
                }
            );
            assert_eq!(audio_consumer.preferred_layers(), None);
            assert_eq!(audio_consumer.current_layers(), None);
            assert_eq!(
                audio_consumer
                    .app_data()
                    .downcast_ref::<ConsumerAppData>()
                    .unwrap()
                    .baz,
                "LOL"
            );

            let router_dump = router.dump().await.expect("Failed to get router dump");

            assert_eq!(router_dump.map_producer_id_consumer_ids, {
                let mut map = HashMap::new();
                map.insert(audio_producer.id(), {
                    let mut set = HashSet::new();
                    set.insert(audio_consumer.id());
                    set
                });
                map.insert(video_producer.id(), HashSet::new());
                map
            });

            let transport_2_dump = transport_2
                .dump()
                .await
                .expect("Failed to get transport 2 dump");

            assert_eq!(transport_2_dump.producer_ids, vec![]);
            assert_eq!(transport_2_dump.consumer_ids, vec![audio_consumer.id()]);

            assert!(router.can_consume(&video_producer.id(), &consumer_device_capabilities));

            let video_consumer = transport_2
                .consume({
                    let mut options = ConsumerOptions::new(
                        video_producer.id(),
                        consumer_device_capabilities.clone(),
                    );
                    options.paused = true;
                    options.preferred_layers = Some(ConsumerLayers {
                        spatial_layer: 12,
                        temporal_layer: None,
                    });
                    options.app_data = AppData::new(ConsumerAppData { baz: "LOL" });
                    options
                })
                .await
                .expect("Failed to consume video");

            assert_eq!(new_consumer_count.load(Ordering::SeqCst), 2);
            assert_eq!(video_consumer.producer_id(), video_producer.id());
            assert_eq!(video_consumer.kind(), MediaKind::Video);
            assert_eq!(video_consumer.rtp_parameters().mid, Some("1".to_string()));
            assert_eq!(
                video_consumer.rtp_parameters().codecs,
                vec![
                    RtpCodecParameters::Video {
                        mime_type: MimeTypeVideo::H264,
                        payload_type: 103,
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
                            RtcpFeedback::CcmFir,
                            RtcpFeedback::GoogRemb,
                        ],
                    },
                    RtpCodecParameters::Video {
                        mime_type: MimeTypeVideo::RTX,
                        payload_type: 104,
                        clock_rate: NonZeroU32::new(90000).unwrap(),
                        parameters: {
                            let mut parameters = BTreeMap::new();
                            parameters.insert(
                                "apt".to_string(),
                                RtpCodecParametersParametersValue::Number(103),
                            );
                            parameters
                        },
                        rtcp_feedback: vec![],
                    },
                ]
            );
            assert_eq!(video_consumer.r#type(), ConsumerType::Simulcast);
            assert_eq!(video_consumer.paused(), true);
            assert_eq!(video_consumer.producer_paused(), true);
            assert_eq!(video_consumer.priority(), 1);
            assert_eq!(
                video_consumer.score(),
                ConsumerScore {
                    score: 10,
                    producer_score: 0,
                    producer_scores: vec![0, 0, 0, 0]
                }
            );
            assert_eq!(
                video_consumer.preferred_layers(),
                Some(ConsumerLayers {
                    spatial_layer: 3,
                    temporal_layer: Some(0)
                })
            );
            assert_eq!(video_consumer.current_layers(), None);
            assert_eq!(
                video_consumer
                    .app_data()
                    .downcast_ref::<ConsumerAppData>()
                    .unwrap()
                    .baz,
                "LOL"
            );

            let router_dump = router.dump().await.expect("Failed to get router dump");

            assert_eq!(router_dump.map_producer_id_consumer_ids, {
                let mut map = HashMap::new();
                map.insert(audio_producer.id(), {
                    let mut set = HashSet::new();
                    set.insert(audio_consumer.id());
                    set
                });
                map.insert(video_producer.id(), {
                    let mut set = HashSet::new();
                    set.insert(video_consumer.id());
                    set
                });
                map
            });

            let transport_2_dump = transport_2
                .dump()
                .await
                .expect("Failed to get transport 2 dump");

            assert_eq!(transport_2_dump.producer_ids, vec![]);
            assert_eq!(
                transport_2_dump.consumer_ids.clone().sort(),
                vec![audio_consumer.id(), video_consumer.id()].sort()
            );
        });
    }

    // TODO: The rest of consumer tests
}
