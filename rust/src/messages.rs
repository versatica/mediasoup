use crate::active_speaker_observer::ActiveSpeakerObserverOptions;
use crate::audio_level_observer::AudioLevelObserverOptions;
use crate::consumer::{
    ConsumerDump, ConsumerId, ConsumerLayers, ConsumerScore, ConsumerStats, ConsumerTraceEventType,
    ConsumerType,
};
use crate::data_consumer::{DataConsumerDump, DataConsumerId, DataConsumerStat, DataConsumerType};
use crate::data_producer::{DataProducerDump, DataProducerId, DataProducerStat, DataProducerType};
use crate::data_structures::{
    DtlsParameters, DtlsRole, DtlsState, IceCandidate, IceParameters, IceRole, IceState,
    ListenInfo, SctpState, TransportTuple,
};
use crate::direct_transport::DirectTransportOptions;
use crate::fbs::{
    direct_transport, message, pipe_transport, plain_transport, request, response, router,
    transport, web_rtc_transport, worker,
};
use crate::ortc::RtpMapping;
use crate::pipe_transport::PipeTransportOptions;
use crate::plain_transport::PlainTransportOptions;
use crate::producer::{
    ProducerDump, ProducerId, ProducerStat, ProducerTraceEventType, ProducerType,
};
use crate::router::{RouterDump, RouterId};
use crate::rtp_observer::RtpObserverId;
use crate::rtp_parameters::{MediaKind, RtpEncodingParameters, RtpParameters};
use crate::sctp_parameters::{NumSctpStreams, SctpParameters, SctpStreamParameters};
use crate::srtp_parameters::{SrtpCryptoSuite, SrtpParameters};
use crate::transport::{TransportId, TransportTraceEventType};
use crate::webrtc_server::{
    WebRtcServerDump, WebRtcServerIceUsernameFragment, WebRtcServerId, WebRtcServerIpPort,
    WebRtcServerListenInfos, WebRtcServerTupleHash,
};
use crate::webrtc_transport::{
    WebRtcTransportListen, WebRtcTransportListenInfos, WebRtcTransportOptions,
};
use crate::worker::{ChannelMessageHandlers, WorkerDump, WorkerUpdateSettings};
use parking_lot::Mutex;
use planus::Builder;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::error::Error;
use std::fmt::{Debug, Display};
use std::net::IpAddr;
use std::num::NonZeroU16;

pub(crate) trait Request
where
    Self: Debug + Serialize,
{
    type HandlerId: Display;
    type Response: DeserializeOwned;

    /// Request method to call on worker.
    fn as_method(&self) -> &'static str;

    /// Default response to return in case of soft error, such as channel already closed, entity
    /// doesn't exist on worker during closing.
    fn default_for_soft_error() -> Option<Self::Response> {
        None
    }
}

pub(crate) trait RequestFbs
where
    Self: Debug,
{
    /// Request method to call on worker.
    const METHOD: request::Method;
    type HandlerId: Display;
    type Response;

    /// Get a serialized message out of this request.
    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8>;

    /// Default response to return in case of soft error, such as channel already closed, entity
    /// doesn't exist on worker during closing.
    fn default_for_soft_error() -> Option<Self::Response> {
        None
    }

    /// Convert generic response into specific type of this request.
    fn convert_response(response: Option<response::Body>)
        -> Result<Self::Response, Box<dyn Error>>;
}

pub(crate) trait Notification: Debug + Serialize {
    type HandlerId: Display;

    /// Request event to call on worker.
    fn as_event(&self) -> &'static str;
}

// macro_rules! request_response_fbs {
//     (
//         $handler_id_type: ty,
//         $method: literal,
//         $request_struct_name: ident { $( $(#[$request_field_name_attributes: meta])? $request_field_name: ident: $request_field_type: ty$(,)? )* },
//         $existing_response_type: ty,
//         $default_for_soft_error: expr $(,)?
//     ) => {
//         #[derive(Debug, Serialize)]
//         #[serde(rename_all = "camelCase")]
//         pub(crate) struct $request_struct_name {
//             $(
//                 $(#[$request_field_name_attributes])*
//                 pub(crate) $request_field_name: $request_field_type,
//             )*
//         }
//
//         impl RequestFbs for $request_struct_name {
//             type HandlerId = $handler_id_type;
//             type Response = $existing_response_type;
//
//             fn as_method(&self) -> &'static str {
//                 $method
//             }
//
//             fn default_for_soft_error() -> Option<Self::Response> {
//                 $default_for_soft_error
//             }
//         }
//     };
//     // Call above macro with no default for soft error
//     (
//         $handler_id_type: ty,
//         $method: literal,
//         $request_struct_name: ident $request_struct_impl: tt $(,)?
//         $existing_response_type: ty $(,)?
//     ) => {
//         request_response!(
//             $handler_id_type,
//             $method,
//             $request_struct_name $request_struct_impl,
//             $existing_response_type,
//             None,
//         );
//     };
//     // Call above macro with unit type as expected response
//     (
//         $handler_id_type: ty,
//         $method: literal,
//         $request_struct_name: ident $request_struct_impl: tt $(,)?
//     ) => {
//         request_response!(
//             $handler_id_type,
//             $method,
//             $request_struct_name $request_struct_impl,
//             (),
//             None,
//         );
//     };
//     (
//         $handler_id_type: ty,
//         $method: literal,
//         $request_struct_name: ident { $( $(#[$request_field_name_attributes: meta])? $request_field_name: ident: $request_field_type: ty$(,)? )* },
//         $response_struct_name: ident { $( $response_field_name: ident: $response_field_type: ty$(,)? )* },
//     ) => {
//         #[derive(Debug, Serialize)]
//         #[serde(rename_all = "camelCase")]
//         pub(crate) struct $request_struct_name {
//             $(
//                 $(#[$request_field_name_attributes])*
//                 pub(crate) $request_field_name: $request_field_type,
//             )*
//         }
//
//         #[derive(Debug, Deserialize)]
//         #[serde(rename_all = "camelCase")]
//         pub(crate) struct $response_struct_name {
//             $( pub(crate) $response_field_name: $response_field_type, )*
//         }
//
//         impl RequestFbs for $request_struct_name {
//             type HandlerId = $handler_id_type;
//             type Response = $response_struct_name;
//
//             fn as_method(&self) -> &'static str {
//                 $method
//             }
//         }
//     };
// }

macro_rules! request_response {
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident { $( $(#[$request_field_name_attributes: meta])? $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $existing_response_type: ty,
        $default_for_soft_error: expr $(,)?
    ) => {
        #[derive(Debug, Serialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $request_struct_name {
            $(
                $(#[$request_field_name_attributes])*
                pub(crate) $request_field_name: $request_field_type,
            )*
        }

        impl Request for $request_struct_name {
            type HandlerId = $handler_id_type;
            type Response = $existing_response_type;

            fn as_method(&self) -> &'static str {
                $method
            }

            fn default_for_soft_error() -> Option<Self::Response> {
                $default_for_soft_error
            }
        }
    };
    // Call above macro with no default for soft error
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
        $existing_response_type: ty $(,)?
    ) => {
        request_response!(
            $handler_id_type,
            $method,
            $request_struct_name $request_struct_impl,
            $existing_response_type,
            None,
        );
    };
    // Call above macro with unit type as expected response
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident $request_struct_impl: tt $(,)?
    ) => {
        request_response!(
            $handler_id_type,
            $method,
            $request_struct_name $request_struct_impl,
            (),
            None,
        );
    };
    (
        $handler_id_type: ty,
        $method: literal,
        $request_struct_name: ident { $( $(#[$request_field_name_attributes: meta])? $request_field_name: ident: $request_field_type: ty$(,)? )* },
        $response_struct_name: ident { $( $response_field_name: ident: $response_field_type: ty$(,)? )* },
    ) => {
        #[derive(Debug, Serialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $request_struct_name {
            $(
                $(#[$request_field_name_attributes])*
                pub(crate) $request_field_name: $request_field_type,
            )*
        }

        #[derive(Debug, Deserialize)]
        #[serde(rename_all = "camelCase")]
        pub(crate) struct $response_struct_name {
            $( pub(crate) $response_field_name: $response_field_type, )*
        }

        impl Request for $request_struct_name {
            type HandlerId = $handler_id_type;
            type Response = $response_struct_name;

            fn as_method(&self) -> &'static str {
                $method
            }
        }
    };
}

#[derive(Debug)]
pub(crate) struct WorkerCloseRequest {}

impl RequestFbs for WorkerCloseRequest {
    const METHOD: request::Method = request::Method::WorkerClose;
    type HandlerId = &'static str;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct WorkerDumpRequest {}

impl RequestFbs for WorkerDumpRequest {
    const METHOD: request::Method = request::Method::WorkerDump;
    type HandlerId = &'static str;
    type Response = WorkerDump;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsWorkerDumpResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(WorkerDump {
            router_ids: data
                .router_ids
                .into_iter()
                .map(|id| id.parse())
                .collect::<Result<_, _>>()?,
            webrtc_server_ids: data
                .web_rtc_server_ids
                .into_iter()
                .map(|id| id.parse())
                .collect::<Result<_, _>>()?,
            channel_message_handlers: ChannelMessageHandlers {
                channel_request_handlers: data
                    .channel_message_handlers
                    .channel_request_handlers
                    .into_iter()
                    .map(|id| id.parse())
                    .collect::<Result<_, _>>()?,
                channel_notification_handlers: data
                    .channel_message_handlers
                    .channel_notification_handlers
                    .into_iter()
                    .map(|id| id.parse())
                    .collect::<Result<_, _>>()?,
            },
        })
    }
}

#[derive(Debug)]
pub(crate) struct WorkerUpdateSettingsRequest {
    pub(crate) data: WorkerUpdateSettings,
}

impl RequestFbs for WorkerUpdateSettingsRequest {
    const METHOD: request::Method = request::Method::WorkerUpdateSettings;
    type HandlerId = &'static str;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = worker::UpdateSettingsRequest::create(
            &mut builder,
            self.data
                .log_level
                .as_ref()
                .map(|log_level| log_level.as_str()),
            self.data.log_tags.as_ref().map(|log_tags| {
                log_tags
                    .iter()
                    .map(|log_tag| log_tag.as_str())
                    .collect::<Vec<_>>()
            }),
        );
        let request_body = request::Body::create_update_settings_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct WorkerCreateWebRtcServerRequest {
    pub(crate) webrtc_server_id: WebRtcServerId,
    pub(crate) listen_infos: WebRtcServerListenInfos,
}

impl RequestFbs for WorkerCreateWebRtcServerRequest {
    const METHOD: request::Method = request::Method::WorkerCreateWebrtcserver;
    type HandlerId = &'static str;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = worker::CreateWebRtcServerRequest::create(
            &mut builder,
            self.webrtc_server_id.to_string(),
            self.listen_infos.to_fbs(),
        );
        let request_body = request::Body::create_create_web_rtc_server_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct WebRtcServerCloseRequest {
    pub(crate) webrtc_server_id: WebRtcServerId,
}

impl RequestFbs for WebRtcServerCloseRequest {
    const METHOD: request::Method = request::Method::WorkerWebrtcserverClose;
    type HandlerId = &'static str;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let data = worker::CloseWebRtcServerRequest::create(
            &mut builder,
            self.webrtc_server_id.to_string(),
        );
        let request_body = request::Body::create_close_web_rtc_server_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct WebRtcServerDumpRequest {}

impl RequestFbs for WebRtcServerDumpRequest {
    const METHOD: request::Method = request::Method::WebrtcserverDump;
    type HandlerId = WebRtcServerId;
    type Response = WebRtcServerDump;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsWebRtcServerDumpResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(WebRtcServerDump {
            id: data.id.parse()?,
            udp_sockets: data
                .udp_sockets
                .into_iter()
                .map(|ip_port| WebRtcServerIpPort {
                    ip: ip_port.ip.parse().unwrap(),
                    port: ip_port.port,
                })
                .collect(),
            tcp_servers: data
                .tcp_servers
                .into_iter()
                .map(|ip_port| WebRtcServerIpPort {
                    ip: ip_port.ip.parse().unwrap(),
                    port: ip_port.port,
                })
                .collect(),
            webrtc_transport_ids: data
                .web_rtc_transport_ids
                .into_iter()
                .map(|id| id.parse())
                .collect::<Result<_, _>>()?,
            local_ice_username_fragments: data
                .local_ice_username_fragments
                .into_iter()
                .map(|username_fragment| WebRtcServerIceUsernameFragment {
                    local_ice_username_fragment: username_fragment
                        .local_ice_username_fragment
                        .parse()
                        .unwrap(),
                    webrtc_transport_id: username_fragment.web_rtc_transport_id.parse().unwrap(),
                })
                .collect(),
            tuple_hashes: data
                .tuple_hashes
                .into_iter()
                .map(|tuple_hash| WebRtcServerTupleHash {
                    tuple_hash: tuple_hash.local_ice_username_fragment,
                    webrtc_transport_id: tuple_hash.web_rtc_transport_id.parse().unwrap(),
                })
                .collect(),
        })
    }
}

#[derive(Debug)]
pub(crate) struct WorkerCreateRouterRequest {
    pub(crate) router_id: RouterId,
}

impl RequestFbs for WorkerCreateRouterRequest {
    const METHOD: request::Method = request::Method::WorkerCreateRouter;
    type HandlerId = &'static str;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = worker::CreateRouterRequest::create(&mut builder, self.router_id.to_string());
        let request_body = request::Body::create_create_router_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct RouterCloseRequest {
    pub(crate) router_id: RouterId,
}

impl RequestFbs for RouterCloseRequest {
    const METHOD: request::Method = request::Method::WorkerCloseRouter;
    type HandlerId = &'static str;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let data = worker::CloseRouterRequest::create(&mut builder, self.router_id.to_string());
        let request_body = request::Body::create_close_router_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct RouterDumpRequest {}

impl RequestFbs for RouterDumpRequest {
    const METHOD: request::Method = request::Method::RouterDump;
    type HandlerId = RouterId;
    type Response = RouterDump;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsRouterDumpResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(RouterDump {
            id: data.id.parse()?,
            map_consumer_id_producer_id: data
                .map_consumer_id_producer_id
                .into_iter()
                .map(|key_value| Ok((key_value.key.parse()?, key_value.value.parse()?)))
                .collect::<Result<_, Box<dyn Error>>>()?,
            map_data_consumer_id_data_producer_id: data
                .map_data_consumer_id_data_producer_id
                .into_iter()
                .map(|key_value| Ok((key_value.key.parse()?, key_value.value.parse()?)))
                .collect::<Result<_, Box<dyn Error>>>()?,
            map_data_producer_id_data_consumer_ids: data
                .map_data_producer_id_data_consumer_ids
                .into_iter()
                .map(|key_values| {
                    Ok((
                        key_values.key.parse()?,
                        key_values
                            .values
                            .into_iter()
                            .map(|value| value.parse())
                            .collect::<Result<_, _>>()?,
                    ))
                })
                .collect::<Result<_, Box<dyn Error>>>()?,
            map_producer_id_consumer_ids: data
                .map_producer_id_consumer_ids
                .into_iter()
                .map(|key_values| {
                    Ok((
                        key_values.key.parse()?,
                        key_values
                            .values
                            .into_iter()
                            .map(|value| value.parse())
                            .collect::<Result<_, _>>()?,
                    ))
                })
                .collect::<Result<_, Box<dyn Error>>>()?,
            map_producer_id_observer_ids: data
                .map_producer_id_observer_ids
                .into_iter()
                .map(|key_values| {
                    Ok((
                        key_values.key.parse()?,
                        key_values
                            .values
                            .into_iter()
                            .map(|value| value.parse())
                            .collect::<Result<_, _>>()?,
                    ))
                })
                .collect::<Result<_, Box<dyn Error>>>()?,
            rtp_observer_ids: data
                .rtp_observer_ids
                .into_iter()
                .map(|id| id.parse())
                .collect::<Result<_, _>>()?,
            transport_ids: data
                .transport_ids
                .into_iter()
                .map(|id| id.parse())
                .collect::<Result<_, _>>()?,
        })
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateDirectTransportData {
    transport_id: TransportId,
    direct: bool,
    max_message_size: u32,
}

impl RouterCreateDirectTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        direct_transport_options: &DirectTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            direct: true,
            max_message_size: direct_transport_options.max_message_size,
        }
    }

    pub(crate) fn to_fbs(&self) -> direct_transport::DirectTransportOptions {
        direct_transport::DirectTransportOptions {
            base: Box::new(transport::Options {
                direct: true,
                max_message_size: self.max_message_size,
                initial_available_outgoing_bitrate: 0,
                enable_sctp: false,
                num_sctp_streams: None,
                max_sctp_message_size: 0,
                sctp_send_buffer_size: 0,
                is_data_channel: false,
            }),
        }
    }
}

#[derive(Debug)]
pub(crate) struct RouterCreateDirectTransportRequest {
    pub(crate) data: RouterCreateDirectTransportData,
}

impl RequestFbs for RouterCreateDirectTransportRequest {
    const METHOD: request::Method = request::Method::RouterCreateDirecttransport;
    type HandlerId = RouterId;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = router::CreateDirectTransportRequest::create(
            &mut builder,
            self.data.transport_id.to_string(),
            self.data.to_fbs(),
        );
        let request_body =
            request::Body::create_create_direct_transport_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug, Serialize)]
#[serde(untagged)]
enum RouterCreateWebrtcTransportListen {
    #[serde(rename_all = "camelCase")]
    Individual {
        listen_infos: WebRtcTransportListenInfos,
    },
    Server {
        #[serde(rename = "webRtcServerId")]
        webrtc_server_id: WebRtcServerId,
    },
}

impl RouterCreateWebrtcTransportListen {
    pub(crate) fn to_fbs(&self) -> web_rtc_transport::Listen {
        match self {
            RouterCreateWebrtcTransportListen::Individual { listen_infos } => {
                web_rtc_transport::Listen::ListenIndividual(Box::new(
                    web_rtc_transport::ListenIndividual {
                        listen_infos: listen_infos
                            .iter()
                            .map(|listen_info| listen_info.to_fbs())
                            .collect(),
                    },
                ))
            }
            RouterCreateWebrtcTransportListen::Server { webrtc_server_id } => {
                web_rtc_transport::Listen::ListenServer(Box::new(web_rtc_transport::ListenServer {
                    web_rtc_server_id: webrtc_server_id.to_string(),
                }))
            }
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateWebrtcTransportData {
    transport_id: TransportId,
    #[serde(flatten)]
    listen: RouterCreateWebrtcTransportListen,
    initial_available_outgoing_bitrate: u32,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    is_data_channel: bool,
}

impl RouterCreateWebrtcTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        webrtc_transport_options: &WebRtcTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            listen: match &webrtc_transport_options.listen {
                WebRtcTransportListen::Individual { listen_infos } => {
                    RouterCreateWebrtcTransportListen::Individual {
                        listen_infos: listen_infos.clone(),
                    }
                }
                WebRtcTransportListen::Server { webrtc_server } => {
                    RouterCreateWebrtcTransportListen::Server {
                        webrtc_server_id: webrtc_server.id(),
                    }
                }
            },
            initial_available_outgoing_bitrate: webrtc_transport_options
                .initial_available_outgoing_bitrate,
            enable_sctp: webrtc_transport_options.enable_sctp,
            num_sctp_streams: webrtc_transport_options.num_sctp_streams,
            max_sctp_message_size: webrtc_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: webrtc_transport_options.sctp_send_buffer_size,
            is_data_channel: true,
        }
    }

    pub(crate) fn to_fbs(&self) -> web_rtc_transport::WebRtcTransportOptions {
        web_rtc_transport::WebRtcTransportOptions {
            base: Box::new(transport::Options {
                direct: false,
                max_message_size: 0,
                initial_available_outgoing_bitrate: self.initial_available_outgoing_bitrate,
                enable_sctp: self.enable_sctp,
                num_sctp_streams: Some(Box::new(self.num_sctp_streams.to_fbs())),
                max_sctp_message_size: self.max_sctp_message_size,
                sctp_send_buffer_size: self.sctp_send_buffer_size,
                is_data_channel: true,
            }),
            listen: self.listen.to_fbs(),
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct WebRtcTransportData {
    pub(crate) ice_role: IceRole,
    pub(crate) ice_parameters: IceParameters,
    pub(crate) ice_candidates: Vec<IceCandidate>,
    pub(crate) ice_state: Mutex<IceState>,
    pub(crate) ice_selected_tuple: Mutex<Option<TransportTuple>>,
    pub(crate) dtls_parameters: Mutex<DtlsParameters>,
    pub(crate) dtls_state: Mutex<DtlsState>,
    pub(crate) dtls_remote_cert: Mutex<Option<String>>,
    pub(crate) sctp_parameters: Option<SctpParameters>,
    pub(crate) sctp_state: Mutex<Option<SctpState>>,
}

#[derive(Debug)]
pub(crate) struct RouterCreateWebRtcTransportRequest {
    pub(crate) data: RouterCreateWebrtcTransportData,
}

impl RequestFbs for RouterCreateWebRtcTransportRequest {
    const METHOD: request::Method = request::Method::RouterCreateWebrtctransport;
    type HandlerId = RouterId;
    type Response = WebRtcTransportData;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = router::CreateWebRtcTransportRequest::create(
            &mut builder,
            self.data.transport_id.to_string(),
            self.data.to_fbs(),
        );
        let request_body =
            request::Body::create_create_web_rtc_transport_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsWebRtcTransportDumpResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(WebRtcTransportData {
            ice_role: IceRole::from_fbs(data.ice_role),
            ice_parameters: IceParameters::from_fbs(*data.ice_parameters),
            ice_candidates: data
                .ice_candidates
                .iter()
                .map(IceCandidate::from_fbs)
                .collect(),
            ice_state: Mutex::new(IceState::from_fbs(data.ice_state)),
            ice_selected_tuple: Mutex::new(
                data.ice_selected_tuple
                    .map(|tuple| TransportTuple::from_fbs(tuple.as_ref())),
            ),
            dtls_parameters: Mutex::new(DtlsParameters::from_fbs(*data.dtls_parameters)),
            dtls_state: Mutex::new(DtlsState::from_fbs(data.dtls_state)),
            dtls_remote_cert: Mutex::new(None),
            sctp_parameters: data
                .base
                .sctp_parameters
                .map(|parameters| SctpParameters::from_fbs(parameters.as_ref())),
            sctp_state: Mutex::new(
                data.base
                    .sctp_state
                    .map(|state| SctpState::from_fbs(&state)),
            ),
        })
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreatePlainTransportData {
    transport_id: TransportId,
    listen_info: ListenInfo,
    rtcp_listen_info: Option<ListenInfo>,
    rtcp_mux: bool,
    comedia: bool,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    enable_srtp: bool,
    srtp_crypto_suite: SrtpCryptoSuite,
    is_data_channel: bool,
}

impl RouterCreatePlainTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        plain_transport_options: &PlainTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            listen_info: plain_transport_options.listen_info,
            rtcp_listen_info: plain_transport_options.rtcp_listen_info,
            rtcp_mux: plain_transport_options.rtcp_mux,
            comedia: plain_transport_options.comedia,
            enable_sctp: plain_transport_options.enable_sctp,
            num_sctp_streams: plain_transport_options.num_sctp_streams,
            max_sctp_message_size: plain_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: plain_transport_options.sctp_send_buffer_size,
            enable_srtp: plain_transport_options.enable_srtp,
            srtp_crypto_suite: plain_transport_options.srtp_crypto_suite,
            is_data_channel: false,
        }
    }

    pub(crate) fn to_fbs(&self) -> plain_transport::PlainTransportOptions {
        plain_transport::PlainTransportOptions {
            base: Box::new(transport::Options {
                direct: false,
                initial_available_outgoing_bitrate: 0,
                enable_sctp: self.enable_sctp,
                num_sctp_streams: Some(Box::new(self.num_sctp_streams.to_fbs())),
                max_sctp_message_size: self.max_sctp_message_size,
                sctp_send_buffer_size: self.sctp_send_buffer_size,
                is_data_channel: self.is_data_channel,
                max_message_size: self.max_sctp_message_size,
            }),
            listen_info: Box::new(self.listen_info.to_fbs()),
            rtcp_listen_info: self
                .rtcp_listen_info
                .map(|listen_info| Box::new(listen_info.to_fbs())),
            rtcp_mux: self.rtcp_mux,
            comedia: self.comedia,
            enable_srtp: self.enable_srtp,
            srtp_crypto_suite: Some(SrtpCryptoSuite::to_fbs(self.srtp_crypto_suite)),
        }
    }
}

#[derive(Debug)]
pub(crate) struct RouterCreatePlainTransportRequest {
    pub(crate) data: RouterCreatePlainTransportData,
}

impl RequestFbs for RouterCreatePlainTransportRequest {
    const METHOD: request::Method = request::Method::RouterCreatePlaintransport;
    type HandlerId = RouterId;
    type Response = PlainTransportData;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = router::CreatePlainTransportRequest::create(
            &mut builder,
            self.data.transport_id.to_string(),
            self.data.to_fbs(),
        );
        let request_body = request::Body::create_create_plain_transport_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsPlainTransportDumpResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(PlainTransportData {
            tuple: Mutex::new(TransportTuple::from_fbs(data.tuple.as_ref())),
            rtcp_tuple: Mutex::new(
                data.rtcp_tuple
                    .map(|tuple| TransportTuple::from_fbs(tuple.as_ref())),
            ),
            sctp_parameters: data
                .base
                .sctp_parameters
                .map(|parameters| SctpParameters::from_fbs(parameters.as_ref())),
            sctp_state: Mutex::new(
                data.base
                    .sctp_state
                    .map(|state| SctpState::from_fbs(&state)),
            ),
            srtp_parameters: Mutex::new(
                data.srtp_parameters
                    .map(|parameters| SrtpParameters::from_fbs(parameters.as_ref())),
            ),
        })
    }
}

pub(crate) struct PlainTransportData {
    // The following fields are present, but unused
    // rtcp_mux: bool,
    // comedia: bool,
    pub(crate) tuple: Mutex<TransportTuple>,
    pub(crate) rtcp_tuple: Mutex<Option<TransportTuple>>,
    pub(crate) sctp_parameters: Option<SctpParameters>,
    pub(crate) sctp_state: Mutex<Option<SctpState>>,
    pub(crate) srtp_parameters: Mutex<Option<SrtpParameters>>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreatePipeTransportData {
    transport_id: TransportId,
    listen_info: ListenInfo,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    enable_rtx: bool,
    enable_srtp: bool,
    is_data_channel: bool,
}

impl RouterCreatePipeTransportData {
    pub(crate) fn from_options(
        transport_id: TransportId,
        pipe_transport_options: &PipeTransportOptions,
    ) -> Self {
        Self {
            transport_id,
            listen_info: pipe_transport_options.listen_info,
            enable_sctp: pipe_transport_options.enable_sctp,
            num_sctp_streams: pipe_transport_options.num_sctp_streams,
            max_sctp_message_size: pipe_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: pipe_transport_options.sctp_send_buffer_size,
            enable_rtx: pipe_transport_options.enable_rtx,
            enable_srtp: pipe_transport_options.enable_srtp,
            is_data_channel: false,
        }
    }
}

request_response!(
    RouterId,
    "router.createPipeTransport",
    RouterCreatePipeTransportRequest {
        #[serde(flatten)]
        data: RouterCreatePipeTransportData,
    },
    PipeTransportData {
        tuple: Mutex<TransportTuple>,
        sctp_parameters: Option<SctpParameters>,
        sctp_state: Mutex<Option<SctpState>>,
        rtx: bool,
        srtp_parameters: Mutex<Option<SrtpParameters>>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateAudioLevelObserverData {
    rtp_observer_id: RtpObserverId,
    max_entries: NonZeroU16,
    threshold: i8,
    interval: u16,
}

impl RouterCreateAudioLevelObserverData {
    pub(crate) fn from_options(
        rtp_observer_id: RtpObserverId,
        audio_level_observer_options: &AudioLevelObserverOptions,
    ) -> Self {
        Self {
            rtp_observer_id,
            max_entries: audio_level_observer_options.max_entries,
            threshold: audio_level_observer_options.threshold,
            interval: audio_level_observer_options.interval,
        }
    }
}

request_response!(
    RouterId,
    "router.createAudioLevelObserver",
    RouterCreateAudioLevelObserverRequest {
        #[serde(flatten)]
        data: RouterCreateAudioLevelObserverData,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateActiveSpeakerObserverData {
    rtp_observer_id: RtpObserverId,
    interval: u16,
}

impl RouterCreateActiveSpeakerObserverData {
    pub(crate) fn from_options(
        rtp_observer_id: RtpObserverId,
        active_speaker_observer_options: &ActiveSpeakerObserverOptions,
    ) -> Self {
        Self {
            rtp_observer_id,
            interval: active_speaker_observer_options.interval,
        }
    }
}

request_response!(
    RouterId,
    "router.createActiveSpeakerObserver",
    RouterCreateActiveSpeakerObserverRequest {
        #[serde(flatten)]
        data: RouterCreateActiveSpeakerObserverData,
    },
);

request_response!(
    RouterId,
    "router.closeTransport",
    TransportCloseRequest {
        transport_id: TransportId,
    },
    (),
    Some(()),
);

request_response!(
    TransportId,
    "transport.dump",
    TransportDumpRequest {},
    Value,
);

#[derive(Debug)]
pub(crate) struct TransportDumpRequestFbs {}

impl RequestFbs for TransportDumpRequestFbs {
    const METHOD: request::Method = request::Method::TransportDump;
    type HandlerId = TransportId;
    type Response = response::Body;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        match response {
            Some(data) => Ok(data),
            _ => {
                panic!("Wrong message from worker: {response:?}");
            }
        }
    }
}
request_response!(
    TransportId,
    "transport.getStats",
    TransportGetStatsRequest {},
    Value,
);

#[derive(Debug)]
pub(crate) struct TransportGetStatsRequestFbs {}

impl RequestFbs for TransportGetStatsRequestFbs {
    const METHOD: request::Method = request::Method::TransportGetStats;
    type HandlerId = TransportId;
    type Response = response::Body;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        match response {
            Some(data) => Ok(data),
            _ => {
                panic!("Wrong message from worker: {response:?}");
            }
        }
    }
}

#[derive(Debug)]
pub(crate) struct TransportCloseRequestFbs {
    pub(crate) transport_id: TransportId,
}

impl RequestFbs for TransportCloseRequestFbs {
    const METHOD: request::Method = request::Method::RouterCloseTransport;
    type HandlerId = RouterId;
    type Response = ();

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data =
            router::CloseTransportRequest::create(&mut builder, self.transport_id.to_string());
        let request_body = request::Body::create_close_transport_request(&mut builder, data);

        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        _response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        Ok(())
    }
}

#[derive(Debug)]
pub(crate) struct WebRtcTransportConnectResponse {
    pub(crate) dtls_local_role: DtlsRole,
}

#[derive(Debug)]
pub(crate) struct WebRtcTransportConnectRequest {
    pub(crate) dtls_parameters: DtlsParameters,
}

impl RequestFbs for WebRtcTransportConnectRequest {
    const METHOD: request::Method = request::Method::WebrtctransportConnect;
    type HandlerId = TransportId;
    type Response = WebRtcTransportConnectResponse;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data =
            web_rtc_transport::ConnectRequest::create(&mut builder, self.dtls_parameters.to_fbs());
        let request_body =
            request::Body::create_web_rtc_transport_connect_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsWebRtcTransportConnectResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(WebRtcTransportConnectResponse {
            dtls_local_role: DtlsRole::from_fbs(data.dtls_local_role),
        })
    }
}

#[derive(Debug)]
pub(crate) struct PipeTransportConnectResponse {
    pub(crate) tuple: TransportTuple,
}

#[derive(Debug)]
pub(crate) struct PipeTransportConnectRequest {
    pub(crate) ip: IpAddr,
    pub(crate) port: u16,
    pub(crate) srtp_parameters: Option<SrtpParameters>,
}

impl RequestFbs for PipeTransportConnectRequest {
    const METHOD: request::Method = request::Method::PipetransportConnect;
    type HandlerId = TransportId;
    type Response = PipeTransportConnectResponse;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = pipe_transport::ConnectRequest::create(
            &mut builder,
            self.ip.to_string(),
            self.port,
            self.srtp_parameters.map(|parameters| parameters.to_fbs()),
        );
        let request_body = request::Body::create_pipe_transport_connect_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsPipeTransportConnectResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(PipeTransportConnectResponse {
            tuple: TransportTuple::from_fbs(data.tuple.as_ref()),
        })
    }
}

#[derive(Debug)]
pub(crate) struct PlainTransportConnectResponse {
    pub(crate) tuple: TransportTuple,
    pub(crate) rtcp_tuple: Option<TransportTuple>,
    pub(crate) srtp_parameters: Option<SrtpParameters>,
}

#[derive(Debug)]
pub(crate) struct TransportConnectPlainRequest {
    pub(crate) ip: Option<IpAddr>,
    pub(crate) port: Option<u16>,
    pub(crate) rtcp_port: Option<u16>,
    pub(crate) srtp_parameters: Option<SrtpParameters>,
}

impl RequestFbs for TransportConnectPlainRequest {
    const METHOD: request::Method = request::Method::PlaintransportConnect;
    type HandlerId = TransportId;
    type Response = PlainTransportConnectResponse;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let data = plain_transport::ConnectRequest::create(
            &mut builder,
            self.ip.map(|ip| ip.to_string()),
            self.port,
            self.rtcp_port,
            self.srtp_parameters.map(|parameters| parameters.to_fbs()),
        );
        let request_body =
            request::Body::create_plain_transport_connect_request(&mut builder, data);
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            Some(request_body),
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsPlainTransportConnectResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(PlainTransportConnectResponse {
            tuple: TransportTuple::from_fbs(data.tuple.as_ref()),
            rtcp_tuple: data
                .rtcp_tuple
                .map(|tuple| TransportTuple::from_fbs(tuple.as_ref())),
            srtp_parameters: data
                .srtp_parameters
                .map(|parameters| SrtpParameters::from_fbs(parameters.as_ref())),
        })
    }
}

request_response!(
    TransportId,
    "transport.setMaxIncomingBitrate",
    TransportSetMaxIncomingBitrateRequest { bitrate: u32 },
);

request_response!(
    TransportId,
    "transport.setMaxOutgoingBitrate",
    TransportSetMaxOutgoingBitrateRequest { bitrate: u32 },
);

request_response!(
    TransportId,
    "transport.setMinOutgoingBitrate",
    TransportSetMinOutgoingBitrateRequest { bitrate: u32 },
);

#[derive(Debug)]
pub(crate) struct TransportRestartIceRequest {}

impl RequestFbs for TransportRestartIceRequest {
    const METHOD: request::Method = request::Method::TransportRestartIce;
    type HandlerId = TransportId;
    type Response = IceParameters;

    fn into_bytes(self, id: u32, handler_id: Self::HandlerId) -> Vec<u8> {
        let mut builder = Builder::new();
        let request = request::Request::create(
            &mut builder,
            id,
            Self::METHOD,
            handler_id.to_string(),
            None::<request::Body>,
        );
        let message_body = message::Body::create_request(&mut builder, request);
        let message = message::Message::create(&mut builder, message::Type::Request, message_body);

        builder.finish(message, None).to_vec()
    }

    fn convert_response(
        response: Option<response::Body>,
    ) -> Result<Self::Response, Box<dyn Error>> {
        let Some(response::Body::FbsTransportRestartIceResponse(data)) = response else {
            panic!("Wrong message from worker: {response:?}");
        };

        Ok(IceParameters::from_fbs(web_rtc_transport::IceParameters {
            username_fragment: data.username_fragment,
            password: data.password,
            ice_lite: data.ice_lite,
        }))
    }
}
request_response!(
    TransportId,
    "transport.produce",
    TransportProduceRequest {
        producer_id: ProducerId,
        kind: MediaKind,
        rtp_parameters: RtpParameters,
        rtp_mapping: RtpMapping,
        key_frame_request_delay: u32,
        paused: bool,
    },
    TransportProduceResponse {
        r#type: ProducerType,
    },
);

request_response!(
    TransportId,
    "transport.consume",
    TransportConsumeRequest {
        consumer_id: ConsumerId,
        producer_id: ProducerId,
        kind: MediaKind,
        rtp_parameters: RtpParameters,
        r#type: ConsumerType,
        consumable_rtp_encodings: Vec<RtpEncodingParameters>,
        paused: bool,
        preferred_layers: Option<ConsumerLayers>,
        ignore_dtx: bool,
    },
    TransportConsumeResponse {
        paused: bool,
        producer_paused: bool,
        score: ConsumerScore,
        preferred_layers: Option<ConsumerLayers>,
    },
);

request_response!(
    TransportId,
    "transport.produceData",
    TransportProduceDataRequest {
        data_producer_id: DataProducerId,
        r#type: DataProducerType,
        #[serde(skip_serializing_if = "Option::is_none")]
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        paused: bool,
    },
    TransportProduceDataResponse {
        r#type: DataProducerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        paused: bool,
    },
);

request_response!(
    TransportId,
    "transport.consumeData",
    TransportConsumeDataRequest {
        data_consumer_id: DataConsumerId,
        data_producer_id: DataProducerId,
        r#type: DataConsumerType,
        #[serde(skip_serializing_if = "Option::is_none")]
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        paused: bool,
    },
    TransportConsumeDataResponse {
        r#type: DataConsumerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        paused: bool,
        data_producer_paused: bool,
    },
);

request_response!(
    TransportId,
    "transport.enableTraceEvent",
    TransportEnableTraceEventRequest {
        types: Vec<TransportTraceEventType>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportSendRtcpNotification {}

impl Notification for TransportSendRtcpNotification {
    type HandlerId = TransportId;

    fn as_event(&self) -> &'static str {
        "transport.sendRtcp"
    }
}

request_response!(
    TransportId,
    "transport.closeProducer",
    ProducerCloseRequest {
        producer_id: ProducerId,
    },
    (),
    Some(()),
);

request_response!(
    ProducerId,
    "producer.dump",
    ProducerDumpRequest {},
    ProducerDump
);

request_response!(
    ProducerId,
    "producer.getStats",
    ProducerGetStatsRequest {},
    Vec<ProducerStat>,
);

request_response!(ProducerId, "producer.pause", ProducerPauseRequest {});

request_response!(ProducerId, "producer.resume", ProducerResumeRequest {});

request_response!(
    ProducerId,
    "producer.enableTraceEvent",
    ProducerEnableTraceEventRequest {
        types: Vec<ProducerTraceEventType>,
    },
);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct ProducerSendNotification {}

impl Notification for ProducerSendNotification {
    type HandlerId = ProducerId;

    fn as_event(&self) -> &'static str {
        "producer.send"
    }
}

request_response!(
    TransportId,
    "transport.closeConsumer",
    ConsumerCloseRequest {
        consumer_id: ConsumerId,
    },
    (),
    Some(()),
);

request_response!(
    ConsumerId,
    "consumer.dump",
    ConsumerDumpRequest {},
    ConsumerDump,
);

request_response!(
    ConsumerId,
    "consumer.getStats",
    ConsumerGetStatsRequest {},
    ConsumerStats,
);

request_response!(ConsumerId, "consumer.pause", ConsumerPauseRequest {},);

request_response!(ConsumerId, "consumer.resume", ConsumerResumeRequest {},);

request_response!(
    ConsumerId,
    "consumer.setPreferredLayers",
    ConsumerSetPreferredLayersRequest {
        #[serde(flatten)]
        data: ConsumerLayers,
    },
    Option<ConsumerLayers>,
);

request_response!(
    ConsumerId,
    "consumer.setPriority",
    ConsumerSetPriorityRequest { priority: u8 },
    ConsumerSetPriorityResponse { priority: u8 },
);

request_response!(
    ConsumerId,
    "consumer.requestKeyFrame",
    ConsumerRequestKeyFrameRequest {},
);

request_response!(
    ConsumerId,
    "consumer.enableTraceEvent",
    ConsumerEnableTraceEventRequest {
        types: Vec<ConsumerTraceEventType>,
    },
);

request_response!(
    TransportId,
    "transport.closeDataProducer",
    DataProducerCloseRequest {
        data_producer_id: DataProducerId,
    },
    (),
    Some(()),
);

request_response!(
    DataProducerId,
    "dataProducer.dump",
    DataProducerDumpRequest {},
    DataProducerDump,
);

request_response!(
    DataProducerId,
    "dataProducer.getStats",
    DataProducerGetStatsRequest {},
    Vec<DataProducerStat>,
);

request_response!(
    DataProducerId,
    "dataProducer.pause",
    DataProducerPauseRequest {}
);

request_response!(
    DataProducerId,
    "dataProducer.resume",
    DataProducerResumeRequest {}
);

#[derive(Debug, Copy, Clone, Serialize)]
#[serde(into = "u32")]
pub(crate) struct DataProducerSendNotification {
    #[serde(flatten)]
    pub(crate) ppid: u32,
}

impl From<DataProducerSendNotification> for u32 {
    fn from(notification: DataProducerSendNotification) -> Self {
        notification.ppid
    }
}

impl Notification for DataProducerSendNotification {
    type HandlerId = DataProducerId;

    fn as_event(&self) -> &'static str {
        "dataProducer.send"
    }
}

request_response!(
    TransportId,
    "transport.closeDataConsumer",
    DataConsumerCloseRequest {
        data_consumer_id: DataConsumerId,
    },
    (),
    Some(()),
);

request_response!(
    DataConsumerId,
    "dataConsumer.dump",
    DataConsumerDumpRequest {},
    DataConsumerDump,
);

request_response!(
    DataConsumerId,
    "dataConsumer.getStats",
    DataConsumerGetStatsRequest {},
    Vec<DataConsumerStat>,
);

request_response!(
    DataConsumerId,
    "dataConsumer.pause",
    DataConsumerPauseRequest {},
);

request_response!(
    DataConsumerId,
    "dataConsumer.resume",
    DataConsumerResumeRequest {},
);

request_response!(
    DataConsumerId,
    "dataConsumer.getBufferedAmount",
    DataConsumerGetBufferedAmountRequest {},
    DataConsumerGetBufferedAmountResponse {
        buffered_amount: u32,
    },
);

request_response!(
    DataConsumerId,
    "dataConsumer.setBufferedAmountLowThreshold",
    DataConsumerSetBufferedAmountLowThresholdRequest { threshold: u32 },
);

#[derive(Debug, Copy, Clone, Serialize)]
#[serde(into = "u32")]
pub(crate) struct DataConsumerSendRequest {
    pub(crate) ppid: u32,
}

impl Request for DataConsumerSendRequest {
    type HandlerId = DataConsumerId;
    type Response = ();

    fn as_method(&self) -> &'static str {
        "dataConsumer.send"
    }
}

impl From<DataConsumerSendRequest> for u32 {
    fn from(request: DataConsumerSendRequest) -> Self {
        request.ppid
    }
}

request_response!(
    RouterId,
    "router.closeRtpObserver",
    RtpObserverCloseRequest {
        rtp_observer_id: RtpObserverId,
    },
    (),
    Some(()),
);

request_response!(
    RtpObserverId,
    "rtpObserver.pause",
    RtpObserverPauseRequest {},
);

request_response!(
    RtpObserverId,
    "rtpObserver.resume",
    RtpObserverResumeRequest {},
);

request_response!(
    RtpObserverId,
    "rtpObserver.addProducer",
    RtpObserverAddProducerRequest {
        producer_id: ProducerId,
    },
);

request_response!(
    RtpObserverId,
    "rtpObserver.removeProducer",
    RtpObserverRemoveProducerRequest {
        producer_id: ProducerId,
    },
);
