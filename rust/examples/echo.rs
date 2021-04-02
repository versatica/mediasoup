use actix::prelude::*;
use actix_web::web::{Data, Payload};
use actix_web::{web, App, Error, HttpRequest, HttpResponse, HttpServer};
use actix_web_actors::ws;
use mediasoup::consumer::{Consumer, ConsumerId, ConsumerOptions};
use mediasoup::data_structures::{DtlsParameters, IceCandidate, IceParameters, TransportListenIp};
use mediasoup::producer::{Producer, ProducerId, ProducerOptions};
use mediasoup::router::{Router, RouterOptions};
use mediasoup::rtp_parameters::{
    MediaKind, MimeTypeAudio, MimeTypeVideo, RtcpFeedback, RtpCapabilities,
    RtpCapabilitiesFinalized, RtpCodecCapability, RtpCodecParametersParameters, RtpParameters,
};
use mediasoup::transport::{Transport, TransportId};
use mediasoup::webrtc_transport::{
    TransportListenIps, WebRtcTransport, WebRtcTransportOptions, WebRtcTransportRemoteParameters,
};
use mediasoup::worker::WorkerSettings;
use mediasoup::worker_manager::WorkerManager;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::env;
use std::num::{NonZeroU32, NonZeroU8};

fn media_codecs() -> Vec<RtpCodecCapability> {
    vec![
        RtpCodecCapability::Audio {
            mime_type: MimeTypeAudio::Opus,
            preferred_payload_type: None,
            clock_rate: NonZeroU32::new(48000).unwrap(),
            channels: NonZeroU8::new(2).unwrap(),
            parameters: RtpCodecParametersParameters::from([("useinbandfec", 1u32.into())]),
            rtcp_feedback: vec![RtcpFeedback::TransportCc],
        },
        RtpCodecCapability::Video {
            mime_type: MimeTypeVideo::Vp8,
            preferred_payload_type: None,
            clock_rate: NonZeroU32::new(90000).unwrap(),
            parameters: RtpCodecParametersParameters::new(),
            rtcp_feedback: vec![
                RtcpFeedback::Nack,
                RtcpFeedback::NackPli,
                RtcpFeedback::CcmFir,
                RtcpFeedback::GoogRemb,
                RtcpFeedback::TransportCc,
            ],
        },
    ]
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct TransportOptions {
    id: TransportId,
    dtls_parameters: DtlsParameters,
    ice_candidates: Vec<IceCandidate>,
    ice_parameters: IceParameters,
}

#[derive(Serialize)]
#[serde(tag = "action")]
enum ServerMessage {
    #[serde(rename_all = "camelCase")]
    Init {
        consumer_transport_options: TransportOptions,
        producer_transport_options: TransportOptions,
        router_rtp_capabilities: RtpCapabilitiesFinalized,
    },
    ConnectedProducerTransport,
    #[serde(rename_all = "camelCase")]
    Produced {
        id: ProducerId,
    },
    ConnectedConsumerTransport,
    #[serde(rename_all = "camelCase")]
    Consumed {
        id: ConsumerId,
        producer_id: ProducerId,
        kind: MediaKind,
        rtp_parameters: RtpParameters,
    },
}

impl Message for ServerMessage {
    type Result = ();
}

#[derive(Deserialize)]
#[serde(tag = "action")]
enum ClientMessage {
    #[serde(rename_all = "camelCase")]
    Init { rtp_capabilities: RtpCapabilities },
    #[serde(rename_all = "camelCase")]
    ConnectProducerTransport { dtls_parameters: DtlsParameters },
    #[serde(rename_all = "camelCase")]
    Produce {
        kind: MediaKind,
        rtp_parameters: RtpParameters,
    },
    #[serde(rename_all = "camelCase")]
    ConnectConsumerTransport { dtls_parameters: DtlsParameters },
    #[serde(rename_all = "camelCase")]
    Consume { producer_id: ProducerId },
    #[serde(rename_all = "camelCase")]
    ConsumerResume { id: ConsumerId },
}

impl Message for ClientMessage {
    type Result = ();
}

enum InternalMessage {
    SaveProducer(Producer),
    SaveConsumer(Consumer),
    Stop,
}

impl Message for InternalMessage {
    type Result = ();
}

struct Transports {
    consumer: WebRtcTransport,
    producer: WebRtcTransport,
}

struct EchoServer {
    client_rtp_capabilities: Option<RtpCapabilities>,
    consumers: HashMap<ConsumerId, Consumer>,
    producers: Vec<Producer>,
    router: Router,
    transports: Transports,
}

impl EchoServer {
    async fn new(worker_manager: &WorkerManager) -> Result<Self, String> {
        let worker = worker_manager
            .create_worker(WorkerSettings::default())
            .await
            .map_err(|error| format!("Failed to create worker: {}", error))?;
        let router = worker
            .create_router(RouterOptions::new(media_codecs()))
            .await
            .map_err(|error| format!("Failed to create router: {}", error))?;

        let transport_options =
            WebRtcTransportOptions::new(TransportListenIps::new(TransportListenIp {
                ip: "127.0.0.1".parse().unwrap(),
                announced_ip: None,
            }));
        let producer_transport = router
            .create_webrtc_transport(transport_options.clone())
            .await
            .map_err(|error| format!("Failed to create producer transport: {}", error))?;

        let consumer_transport = router
            .create_webrtc_transport(transport_options)
            .await
            .map_err(|error| format!("Failed to create consumer transport: {}", error))?;

        Ok(Self {
            client_rtp_capabilities: None,
            consumers: HashMap::new(),
            producers: vec![],
            router,
            transports: Transports {
                consumer: consumer_transport,
                producer: producer_transport,
            },
        })
    }
}

impl Actor for EchoServer {
    type Context = ws::WebsocketContext<Self>;

    fn started(&mut self, ctx: &mut Self::Context) {
        println!("WebSocket connection created");

        let server_init_message = ServerMessage::Init {
            consumer_transport_options: TransportOptions {
                id: self.transports.consumer.id(),
                dtls_parameters: self.transports.consumer.dtls_parameters(),
                ice_candidates: self.transports.consumer.ice_candidates().clone(),
                ice_parameters: self.transports.consumer.ice_parameters().clone(),
            },
            producer_transport_options: TransportOptions {
                id: self.transports.producer.id(),
                dtls_parameters: self.transports.producer.dtls_parameters(),
                ice_candidates: self.transports.producer.ice_candidates().clone(),
                ice_parameters: self.transports.producer.ice_parameters().clone(),
            },
            router_rtp_capabilities: self.router.rtp_capabilities().clone(),
        };

        ctx.text(serde_json::to_string(&server_init_message).unwrap());
    }

    fn stopped(&mut self, _ctx: &mut Self::Context) {
        println!("WebSocket connection closed");
    }
}

impl StreamHandler<Result<ws::Message, ws::ProtocolError>> for EchoServer {
    fn handle(&mut self, msg: Result<ws::Message, ws::ProtocolError>, ctx: &mut Self::Context) {
        match msg {
            Ok(ws::Message::Ping(msg)) => {
                ctx.pong(&msg);
            }
            Ok(ws::Message::Pong(_)) => {}
            Ok(ws::Message::Text(text)) => match serde_json::from_str::<ClientMessage>(&text) {
                Ok(message) => {
                    ctx.address().do_send(message);
                }
                Err(error) => {
                    eprint!("Failed to parse client message: {}\n{}", error, text);
                }
            },
            Ok(ws::Message::Binary(bin)) => {
                eprint!("Unexpected binary message: {:?}", bin);
            }
            Ok(ws::Message::Close(reason)) => {
                ctx.close(reason);
                ctx.stop();
            }
            _ => ctx.stop(),
        }
    }
}

impl Handler<ClientMessage> for EchoServer {
    type Result = ();

    fn handle(&mut self, message: ClientMessage, ctx: &mut Self::Context) {
        match message {
            ClientMessage::Init { rtp_capabilities } => {
                self.client_rtp_capabilities.replace(rtp_capabilities);
            }
            ClientMessage::ConnectProducerTransport { dtls_parameters } => {
                let address = ctx.address();
                let transport = self.transports.producer.clone();
                actix::spawn(async move {
                    match transport
                        .connect(WebRtcTransportRemoteParameters { dtls_parameters })
                        .await
                    {
                        Ok(_) => {
                            address.do_send(ServerMessage::ConnectedProducerTransport);
                            println!("Producer transport connected");
                        }
                        Err(error) => {
                            eprint!("Failed to connect producer transport: {}", error);
                            address.do_send(InternalMessage::Stop);
                        }
                    }
                });
            }
            ClientMessage::Produce {
                kind,
                rtp_parameters,
            } => {
                let address = ctx.address();
                let transport = self.transports.producer.clone();
                actix::spawn(async move {
                    match transport
                        .produce(ProducerOptions::new(kind, rtp_parameters))
                        .await
                    {
                        Ok(producer) => {
                            let id = producer.id();
                            address.do_send(ServerMessage::Produced { id });
                            address.do_send(InternalMessage::SaveProducer(producer));
                            println!("{:?} producer created: {}", kind, id);
                        }
                        Err(error) => {
                            eprint!("Failed to create {:?} producer: {}", kind, error);
                            address.do_send(InternalMessage::Stop);
                        }
                    }
                });
            }
            ClientMessage::ConnectConsumerTransport { dtls_parameters } => {
                let address = ctx.address();
                let transport = self.transports.consumer.clone();
                actix::spawn(async move {
                    match transport
                        .connect(WebRtcTransportRemoteParameters { dtls_parameters })
                        .await
                    {
                        Ok(_) => {
                            address.do_send(ServerMessage::ConnectedConsumerTransport);
                            println!("Consumer transport connected");
                        }
                        Err(error) => {
                            eprint!("Failed to connect consumer transport: {}", error);
                            address.do_send(InternalMessage::Stop);
                        }
                    }
                });
            }
            ClientMessage::Consume { producer_id } => {
                let address = ctx.address();
                let transport = self.transports.consumer.clone();
                let rtp_capabilities = match self.client_rtp_capabilities.clone() {
                    Some(rtp_capabilities) => rtp_capabilities,
                    None => {
                        eprintln!("Client should send RTP capabilities before consuming");
                        return;
                    }
                };
                actix::spawn(async move {
                    let mut options = ConsumerOptions::new(producer_id, rtp_capabilities);
                    options.paused = true;

                    match transport.consume(options).await {
                        Ok(consumer) => {
                            let id = consumer.id();
                            let kind = consumer.kind();
                            let rtp_parameters = consumer.rtp_parameters().clone();
                            address.do_send(ServerMessage::Consumed {
                                id,
                                producer_id,
                                kind,
                                rtp_parameters,
                            });
                            address.do_send(InternalMessage::SaveConsumer(consumer));
                            println!("{:?} consumer created: {}", kind, id);
                        }
                        Err(error) => {
                            eprint!("Failed to create consumer: {}", error);
                            address.do_send(InternalMessage::Stop);
                        }
                    }
                });
            }
            ClientMessage::ConsumerResume { id } => match self.consumers.get(&id).cloned() {
                Some(consumer) => actix::spawn(async move {
                    match consumer.resume().await {
                        Ok(_) => {
                            println!(
                                "Successfully resumed {:?} consumer {}",
                                consumer.kind(),
                                consumer.id(),
                            );
                        }
                        Err(error) => {
                            println!(
                                "Failed to resume {:?} consumer {}: {}",
                                consumer.kind(),
                                consumer.id(),
                                error,
                            );
                        }
                    }
                }),
                None => {}
            },
        }
    }
}

impl Handler<ServerMessage> for EchoServer {
    type Result = ();

    fn handle(&mut self, message: ServerMessage, ctx: &mut Self::Context) {
        ctx.text(serde_json::to_string(&message).unwrap());
    }
}

impl Handler<InternalMessage> for EchoServer {
    type Result = ();

    fn handle(&mut self, message: InternalMessage, ctx: &mut Self::Context) {
        match message {
            InternalMessage::Stop => {
                ctx.stop();
            }
            InternalMessage::SaveProducer(producer) => {
                // Retain producer to prevent it from being destroyed
                self.producers.push(producer);
            }
            InternalMessage::SaveConsumer(consumer) => {
                self.consumers.insert(consumer.id(), consumer);
            }
        }
    }
}

async fn ws_index(
    r: HttpRequest,
    worker_manager: Data<WorkerManager>,
    stream: Payload,
) -> Result<HttpResponse, Error> {
    let echo_server = EchoServer::new(&worker_manager).await.map_err(|error| {
        eprintln!("{}", error);

        HttpResponse::InternalServerError().finish()
    })?;

    ws::start(echo_server, &r, stream)
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    env_logger::init();

    let worker_manager = WorkerManager::new(
        env::var("MEDIASOUP_WORKER_BIN")
            .map(|path| path.into())
            .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into()),
    );
    HttpServer::new(move || {
        App::new()
            .data(worker_manager.clone())
            .route("/ws", web::get().to(ws_index))
    })
    .workers(2)
    .bind("127.0.0.1:3000")?
    .run()
    .await
}
