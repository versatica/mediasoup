"use strict";
// automatically generated by the FlatBuffers compiler, do not modify
Object.defineProperty(exports, "__esModule", { value: true });
exports.ProduceRequestT = exports.ProduceRequest = exports.EnableTraceEventRequestT = exports.EnableTraceEventRequest = exports.ConsumeRequestT = exports.ConsumeRequest = exports.CloseProducerRequestT = exports.CloseProducerRequest = exports.CloseDataProducerRequestT = exports.CloseDataProducerRequest = exports.CloseDataConsumerRequestT = exports.CloseDataConsumerRequest = exports.CloseConsumerRequestT = exports.CloseConsumerRequest = exports.RemoveProducerRequestT = exports.RemoveProducerRequest = exports.AddProducerRequestT = exports.AddProducerRequest = exports.CreateWebRtcTransportRequestT = exports.CreateWebRtcTransportRequest = exports.CreatePlainTransportRequestT = exports.CreatePlainTransportRequest = exports.CreatePipeTransportRequestT = exports.CreatePipeTransportRequest = exports.CreateDirectTransportRequestT = exports.CreateDirectTransportRequest = exports.CreateAudioLevelObserverRequestT = exports.CreateAudioLevelObserverRequest = exports.CreateActiveSpeakerObserverRequestT = exports.CreateActiveSpeakerObserverRequest = exports.CloseTransportRequestT = exports.CloseTransportRequest = exports.CloseRtpObserverRequestT = exports.CloseRtpObserverRequest = exports.RequestT = exports.Request = exports.Method = exports.unionListToBody = exports.unionToBody = exports.Body = exports.FBS_Producer_EnableTraceEventRequestT = exports.FBS_Producer_EnableTraceEventRequest = exports.SetBufferedAmountLowThresholdRequestT = exports.SetBufferedAmountLowThresholdRequest = exports.SetPriorityRequestT = exports.SetPriorityRequest = exports.SetPreferredLayersRequestT = exports.SetPreferredLayersRequest = exports.FBS_Consumer_EnableTraceEventRequestT = exports.FBS_Consumer_EnableTraceEventRequest = void 0;
exports.UpdateSettingsRequestT = exports.UpdateSettingsRequest = exports.CreateWebRtcServerRequestT = exports.CreateWebRtcServerRequest = exports.CreateRouterRequestT = exports.CreateRouterRequest = exports.CloseWebRtcServerRequestT = exports.CloseWebRtcServerRequest = exports.CloseRouterRequestT = exports.CloseRouterRequest = exports.SetMaxOutgoingBitrateRequestT = exports.SetMaxOutgoingBitrateRequest = exports.SetMaxIncomingBitrateRequestT = exports.SetMaxIncomingBitrateRequest = void 0;
var enable_trace_event_request_1 = require("./fbs/consumer/enable-trace-event-request");
Object.defineProperty(exports, "FBS_Consumer_EnableTraceEventRequest", { enumerable: true, get: function () { return enable_trace_event_request_1.EnableTraceEventRequest; } });
Object.defineProperty(exports, "FBS_Consumer_EnableTraceEventRequestT", { enumerable: true, get: function () { return enable_trace_event_request_1.EnableTraceEventRequestT; } });
var set_preferred_layers_request_1 = require("./fbs/consumer/set-preferred-layers-request");
Object.defineProperty(exports, "SetPreferredLayersRequest", { enumerable: true, get: function () { return set_preferred_layers_request_1.SetPreferredLayersRequest; } });
Object.defineProperty(exports, "SetPreferredLayersRequestT", { enumerable: true, get: function () { return set_preferred_layers_request_1.SetPreferredLayersRequestT; } });
var set_priority_request_1 = require("./fbs/consumer/set-priority-request");
Object.defineProperty(exports, "SetPriorityRequest", { enumerable: true, get: function () { return set_priority_request_1.SetPriorityRequest; } });
Object.defineProperty(exports, "SetPriorityRequestT", { enumerable: true, get: function () { return set_priority_request_1.SetPriorityRequestT; } });
var set_buffered_amount_low_threshold_request_1 = require("./fbs/data-consumer/set-buffered-amount-low-threshold-request");
Object.defineProperty(exports, "SetBufferedAmountLowThresholdRequest", { enumerable: true, get: function () { return set_buffered_amount_low_threshold_request_1.SetBufferedAmountLowThresholdRequest; } });
Object.defineProperty(exports, "SetBufferedAmountLowThresholdRequestT", { enumerable: true, get: function () { return set_buffered_amount_low_threshold_request_1.SetBufferedAmountLowThresholdRequestT; } });
var enable_trace_event_request_2 = require("./fbs/producer/enable-trace-event-request");
Object.defineProperty(exports, "FBS_Producer_EnableTraceEventRequest", { enumerable: true, get: function () { return enable_trace_event_request_2.EnableTraceEventRequest; } });
Object.defineProperty(exports, "FBS_Producer_EnableTraceEventRequestT", { enumerable: true, get: function () { return enable_trace_event_request_2.EnableTraceEventRequestT; } });
var body_1 = require("./fbs/request/body");
Object.defineProperty(exports, "Body", { enumerable: true, get: function () { return body_1.Body; } });
Object.defineProperty(exports, "unionToBody", { enumerable: true, get: function () { return body_1.unionToBody; } });
Object.defineProperty(exports, "unionListToBody", { enumerable: true, get: function () { return body_1.unionListToBody; } });
var method_1 = require("./fbs/request/method");
Object.defineProperty(exports, "Method", { enumerable: true, get: function () { return method_1.Method; } });
var request_1 = require("./fbs/request/request");
Object.defineProperty(exports, "Request", { enumerable: true, get: function () { return request_1.Request; } });
Object.defineProperty(exports, "RequestT", { enumerable: true, get: function () { return request_1.RequestT; } });
var close_rtp_observer_request_1 = require("./fbs/router/close-rtp-observer-request");
Object.defineProperty(exports, "CloseRtpObserverRequest", { enumerable: true, get: function () { return close_rtp_observer_request_1.CloseRtpObserverRequest; } });
Object.defineProperty(exports, "CloseRtpObserverRequestT", { enumerable: true, get: function () { return close_rtp_observer_request_1.CloseRtpObserverRequestT; } });
var close_transport_request_1 = require("./fbs/router/close-transport-request");
Object.defineProperty(exports, "CloseTransportRequest", { enumerable: true, get: function () { return close_transport_request_1.CloseTransportRequest; } });
Object.defineProperty(exports, "CloseTransportRequestT", { enumerable: true, get: function () { return close_transport_request_1.CloseTransportRequestT; } });
var create_active_speaker_observer_request_1 = require("./fbs/router/create-active-speaker-observer-request");
Object.defineProperty(exports, "CreateActiveSpeakerObserverRequest", { enumerable: true, get: function () { return create_active_speaker_observer_request_1.CreateActiveSpeakerObserverRequest; } });
Object.defineProperty(exports, "CreateActiveSpeakerObserverRequestT", { enumerable: true, get: function () { return create_active_speaker_observer_request_1.CreateActiveSpeakerObserverRequestT; } });
var create_audio_level_observer_request_1 = require("./fbs/router/create-audio-level-observer-request");
Object.defineProperty(exports, "CreateAudioLevelObserverRequest", { enumerable: true, get: function () { return create_audio_level_observer_request_1.CreateAudioLevelObserverRequest; } });
Object.defineProperty(exports, "CreateAudioLevelObserverRequestT", { enumerable: true, get: function () { return create_audio_level_observer_request_1.CreateAudioLevelObserverRequestT; } });
var create_direct_transport_request_1 = require("./fbs/router/create-direct-transport-request");
Object.defineProperty(exports, "CreateDirectTransportRequest", { enumerable: true, get: function () { return create_direct_transport_request_1.CreateDirectTransportRequest; } });
Object.defineProperty(exports, "CreateDirectTransportRequestT", { enumerable: true, get: function () { return create_direct_transport_request_1.CreateDirectTransportRequestT; } });
var create_pipe_transport_request_1 = require("./fbs/router/create-pipe-transport-request");
Object.defineProperty(exports, "CreatePipeTransportRequest", { enumerable: true, get: function () { return create_pipe_transport_request_1.CreatePipeTransportRequest; } });
Object.defineProperty(exports, "CreatePipeTransportRequestT", { enumerable: true, get: function () { return create_pipe_transport_request_1.CreatePipeTransportRequestT; } });
var create_plain_transport_request_1 = require("./fbs/router/create-plain-transport-request");
Object.defineProperty(exports, "CreatePlainTransportRequest", { enumerable: true, get: function () { return create_plain_transport_request_1.CreatePlainTransportRequest; } });
Object.defineProperty(exports, "CreatePlainTransportRequestT", { enumerable: true, get: function () { return create_plain_transport_request_1.CreatePlainTransportRequestT; } });
var create_web_rtc_transport_request_1 = require("./fbs/router/create-web-rtc-transport-request");
Object.defineProperty(exports, "CreateWebRtcTransportRequest", { enumerable: true, get: function () { return create_web_rtc_transport_request_1.CreateWebRtcTransportRequest; } });
Object.defineProperty(exports, "CreateWebRtcTransportRequestT", { enumerable: true, get: function () { return create_web_rtc_transport_request_1.CreateWebRtcTransportRequestT; } });
var add_producer_request_1 = require("./fbs/rtp-observer/add-producer-request");
Object.defineProperty(exports, "AddProducerRequest", { enumerable: true, get: function () { return add_producer_request_1.AddProducerRequest; } });
Object.defineProperty(exports, "AddProducerRequestT", { enumerable: true, get: function () { return add_producer_request_1.AddProducerRequestT; } });
var remove_producer_request_1 = require("./fbs/rtp-observer/remove-producer-request");
Object.defineProperty(exports, "RemoveProducerRequest", { enumerable: true, get: function () { return remove_producer_request_1.RemoveProducerRequest; } });
Object.defineProperty(exports, "RemoveProducerRequestT", { enumerable: true, get: function () { return remove_producer_request_1.RemoveProducerRequestT; } });
var close_consumer_request_1 = require("./fbs/transport/close-consumer-request");
Object.defineProperty(exports, "CloseConsumerRequest", { enumerable: true, get: function () { return close_consumer_request_1.CloseConsumerRequest; } });
Object.defineProperty(exports, "CloseConsumerRequestT", { enumerable: true, get: function () { return close_consumer_request_1.CloseConsumerRequestT; } });
var close_data_consumer_request_1 = require("./fbs/transport/close-data-consumer-request");
Object.defineProperty(exports, "CloseDataConsumerRequest", { enumerable: true, get: function () { return close_data_consumer_request_1.CloseDataConsumerRequest; } });
Object.defineProperty(exports, "CloseDataConsumerRequestT", { enumerable: true, get: function () { return close_data_consumer_request_1.CloseDataConsumerRequestT; } });
var close_data_producer_request_1 = require("./fbs/transport/close-data-producer-request");
Object.defineProperty(exports, "CloseDataProducerRequest", { enumerable: true, get: function () { return close_data_producer_request_1.CloseDataProducerRequest; } });
Object.defineProperty(exports, "CloseDataProducerRequestT", { enumerable: true, get: function () { return close_data_producer_request_1.CloseDataProducerRequestT; } });
var close_producer_request_1 = require("./fbs/transport/close-producer-request");
Object.defineProperty(exports, "CloseProducerRequest", { enumerable: true, get: function () { return close_producer_request_1.CloseProducerRequest; } });
Object.defineProperty(exports, "CloseProducerRequestT", { enumerable: true, get: function () { return close_producer_request_1.CloseProducerRequestT; } });
var consume_request_1 = require("./fbs/transport/consume-request");
Object.defineProperty(exports, "ConsumeRequest", { enumerable: true, get: function () { return consume_request_1.ConsumeRequest; } });
Object.defineProperty(exports, "ConsumeRequestT", { enumerable: true, get: function () { return consume_request_1.ConsumeRequestT; } });
var enable_trace_event_request_3 = require("./fbs/transport/enable-trace-event-request");
Object.defineProperty(exports, "EnableTraceEventRequest", { enumerable: true, get: function () { return enable_trace_event_request_3.EnableTraceEventRequest; } });
Object.defineProperty(exports, "EnableTraceEventRequestT", { enumerable: true, get: function () { return enable_trace_event_request_3.EnableTraceEventRequestT; } });
var produce_request_1 = require("./fbs/transport/produce-request");
Object.defineProperty(exports, "ProduceRequest", { enumerable: true, get: function () { return produce_request_1.ProduceRequest; } });
Object.defineProperty(exports, "ProduceRequestT", { enumerable: true, get: function () { return produce_request_1.ProduceRequestT; } });
var set_max_incoming_bitrate_request_1 = require("./fbs/transport/set-max-incoming-bitrate-request");
Object.defineProperty(exports, "SetMaxIncomingBitrateRequest", { enumerable: true, get: function () { return set_max_incoming_bitrate_request_1.SetMaxIncomingBitrateRequest; } });
Object.defineProperty(exports, "SetMaxIncomingBitrateRequestT", { enumerable: true, get: function () { return set_max_incoming_bitrate_request_1.SetMaxIncomingBitrateRequestT; } });
var set_max_outgoing_bitrate_request_1 = require("./fbs/transport/set-max-outgoing-bitrate-request");
Object.defineProperty(exports, "SetMaxOutgoingBitrateRequest", { enumerable: true, get: function () { return set_max_outgoing_bitrate_request_1.SetMaxOutgoingBitrateRequest; } });
Object.defineProperty(exports, "SetMaxOutgoingBitrateRequestT", { enumerable: true, get: function () { return set_max_outgoing_bitrate_request_1.SetMaxOutgoingBitrateRequestT; } });
var close_router_request_1 = require("./fbs/worker/close-router-request");
Object.defineProperty(exports, "CloseRouterRequest", { enumerable: true, get: function () { return close_router_request_1.CloseRouterRequest; } });
Object.defineProperty(exports, "CloseRouterRequestT", { enumerable: true, get: function () { return close_router_request_1.CloseRouterRequestT; } });
var close_web_rtc_server_request_1 = require("./fbs/worker/close-web-rtc-server-request");
Object.defineProperty(exports, "CloseWebRtcServerRequest", { enumerable: true, get: function () { return close_web_rtc_server_request_1.CloseWebRtcServerRequest; } });
Object.defineProperty(exports, "CloseWebRtcServerRequestT", { enumerable: true, get: function () { return close_web_rtc_server_request_1.CloseWebRtcServerRequestT; } });
var create_router_request_1 = require("./fbs/worker/create-router-request");
Object.defineProperty(exports, "CreateRouterRequest", { enumerable: true, get: function () { return create_router_request_1.CreateRouterRequest; } });
Object.defineProperty(exports, "CreateRouterRequestT", { enumerable: true, get: function () { return create_router_request_1.CreateRouterRequestT; } });
var create_web_rtc_server_request_1 = require("./fbs/worker/create-web-rtc-server-request");
Object.defineProperty(exports, "CreateWebRtcServerRequest", { enumerable: true, get: function () { return create_web_rtc_server_request_1.CreateWebRtcServerRequest; } });
Object.defineProperty(exports, "CreateWebRtcServerRequestT", { enumerable: true, get: function () { return create_web_rtc_server_request_1.CreateWebRtcServerRequestT; } });
var update_settings_request_1 = require("./fbs/worker/update-settings-request");
Object.defineProperty(exports, "UpdateSettingsRequest", { enumerable: true, get: function () { return update_settings_request_1.UpdateSettingsRequest; } });
Object.defineProperty(exports, "UpdateSettingsRequestT", { enumerable: true, get: function () { return update_settings_request_1.UpdateSettingsRequestT; } });
