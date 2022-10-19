// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_REQUEST_FBS_REQUEST_H_
#define FLATBUFFERS_GENERATED_REQUEST_FBS_REQUEST_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 2 &&
              FLATBUFFERS_VERSION_MINOR == 0 &&
              FLATBUFFERS_VERSION_REVISION == 8,
             "Non-compatible flatbuffers version included");

#include "transport_generated.h"
#include "worker_generated.h"

namespace FBS {
namespace Request {

struct Request;
struct RequestBuilder;

inline const flatbuffers::TypeTable *RequestTypeTable();

enum class Method : uint8_t {
  WORKER_CLOSE = 0,
  WORKER_DUMP = 1,
  WORKER_GET_RESOURCE_USAGE = 2,
  WORKER_UPDATE_SETTINGS = 3,
  WORKER_CREATE_WEBRTC_SERVER = 4,
  WORKER_CREATE_ROUTER = 5,
  WORKER_WEBRTC_SERVER_CLOSE = 6,
  WEBRTC_SERVER_DUMP = 7,
  WORKER_CLOSE_ROUTER = 8,
  ROUTER_DUMP = 9,
  ROUTER_CREATE_WEBRTC_TRANSPORT = 10,
  ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER = 11,
  ROUTER_CREATE_PLAIN_TRANSPORT = 12,
  ROUTER_CREATE_PIPE_TRANSPORT = 13,
  ROUTER_CREATE_DIRECT_TRANSPORT = 14,
  ROUTER_CLOSE_TRANSPORT = 15,
  ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER = 16,
  ROUTER_CREATE_AUDIO_LEVEL_OBSERVER = 17,
  ROUTER_CLOSE_RTP_OBSERVER = 18,
  TRANSPORT_DUMP = 19,
  TRANSPORT_GET_STATS = 20,
  TRANSPORT_CONNECT = 21,
  TRANSPORT_SET_MAX_INCOMING_BITRATE = 22,
  TRANSPORT_SET_MAX_OUTGOING_BITRATE = 23,
  TRANSPORT_RESTART_ICE = 24,
  TRANSPORT_PRODUCE = 25,
  TRANSPORT_PRODUCE_DATA = 26,
  TRANSPORT_CONSUME = 27,
  TRANSPORT_CONSUME_DATA = 28,
  TRANSPORT_ENABLE_TRACE_EVENT = 29,
  TRANSPORT_CLOSE_PRODUCER = 30,
  TRANSPORT_CLOSE_CONSUMER = 31,
  TRANSPORT_CLOSE_DATA_PRODUCER = 32,
  TRANSPORT_CLOSE_DATA_CONSUMER = 33,
  PRODUCER_DUMP = 34,
  PRODUCER_GET_STATS = 35,
  PRODUCER_PAUSE = 36,
  PRODUCER_RESUME = 37,
  PRODUCER_ENABLE_TRACE_EVENT = 38,
  CONSUMER_DUMP = 39,
  CONSUMER_GET_STATS = 40,
  CONSUMER_PAUSE = 41,
  CONSUMER_RESUME = 42,
  CONSUMER_SET_PREFERRED_LAYERS = 43,
  CONSUMER_SET_PRIORITY = 44,
  CONSUMER_REQUEST_KEY_FRAME = 45,
  CONSUMER_ENABLE_TRACE_EVENT = 46,
  DATA_PRODUCER_DUMP = 47,
  DATA_PRODUCER_GET_STATS = 48,
  DATA_CONSUMER_DUMP = 49,
  DATA_CONSUMER_GET_STATS = 50,
  DATA_CONSUMER_GET_BUFFERED_AMOUNT = 51,
  DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD = 52,
  RTP_OBSERVER_PAUSE = 53,
  RTP_OBSERVER_RESUME = 54,
  RTP_OBSERVER_ADD_PRODUCER = 55,
  RTP_OBSERVER_REMOVE_PRODUCER = 56,
  MIN = WORKER_CLOSE,
  MAX = RTP_OBSERVER_REMOVE_PRODUCER
};

inline const Method (&EnumValuesMethod())[57] {
  static const Method values[] = {
    Method::WORKER_CLOSE,
    Method::WORKER_DUMP,
    Method::WORKER_GET_RESOURCE_USAGE,
    Method::WORKER_UPDATE_SETTINGS,
    Method::WORKER_CREATE_WEBRTC_SERVER,
    Method::WORKER_CREATE_ROUTER,
    Method::WORKER_WEBRTC_SERVER_CLOSE,
    Method::WEBRTC_SERVER_DUMP,
    Method::WORKER_CLOSE_ROUTER,
    Method::ROUTER_DUMP,
    Method::ROUTER_CREATE_WEBRTC_TRANSPORT,
    Method::ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER,
    Method::ROUTER_CREATE_PLAIN_TRANSPORT,
    Method::ROUTER_CREATE_PIPE_TRANSPORT,
    Method::ROUTER_CREATE_DIRECT_TRANSPORT,
    Method::ROUTER_CLOSE_TRANSPORT,
    Method::ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER,
    Method::ROUTER_CREATE_AUDIO_LEVEL_OBSERVER,
    Method::ROUTER_CLOSE_RTP_OBSERVER,
    Method::TRANSPORT_DUMP,
    Method::TRANSPORT_GET_STATS,
    Method::TRANSPORT_CONNECT,
    Method::TRANSPORT_SET_MAX_INCOMING_BITRATE,
    Method::TRANSPORT_SET_MAX_OUTGOING_BITRATE,
    Method::TRANSPORT_RESTART_ICE,
    Method::TRANSPORT_PRODUCE,
    Method::TRANSPORT_PRODUCE_DATA,
    Method::TRANSPORT_CONSUME,
    Method::TRANSPORT_CONSUME_DATA,
    Method::TRANSPORT_ENABLE_TRACE_EVENT,
    Method::TRANSPORT_CLOSE_PRODUCER,
    Method::TRANSPORT_CLOSE_CONSUMER,
    Method::TRANSPORT_CLOSE_DATA_PRODUCER,
    Method::TRANSPORT_CLOSE_DATA_CONSUMER,
    Method::PRODUCER_DUMP,
    Method::PRODUCER_GET_STATS,
    Method::PRODUCER_PAUSE,
    Method::PRODUCER_RESUME,
    Method::PRODUCER_ENABLE_TRACE_EVENT,
    Method::CONSUMER_DUMP,
    Method::CONSUMER_GET_STATS,
    Method::CONSUMER_PAUSE,
    Method::CONSUMER_RESUME,
    Method::CONSUMER_SET_PREFERRED_LAYERS,
    Method::CONSUMER_SET_PRIORITY,
    Method::CONSUMER_REQUEST_KEY_FRAME,
    Method::CONSUMER_ENABLE_TRACE_EVENT,
    Method::DATA_PRODUCER_DUMP,
    Method::DATA_PRODUCER_GET_STATS,
    Method::DATA_CONSUMER_DUMP,
    Method::DATA_CONSUMER_GET_STATS,
    Method::DATA_CONSUMER_GET_BUFFERED_AMOUNT,
    Method::DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD,
    Method::RTP_OBSERVER_PAUSE,
    Method::RTP_OBSERVER_RESUME,
    Method::RTP_OBSERVER_ADD_PRODUCER,
    Method::RTP_OBSERVER_REMOVE_PRODUCER
  };
  return values;
}

inline const char * const *EnumNamesMethod() {
  static const char * const names[58] = {
    "WORKER_CLOSE",
    "WORKER_DUMP",
    "WORKER_GET_RESOURCE_USAGE",
    "WORKER_UPDATE_SETTINGS",
    "WORKER_CREATE_WEBRTC_SERVER",
    "WORKER_CREATE_ROUTER",
    "WORKER_WEBRTC_SERVER_CLOSE",
    "WEBRTC_SERVER_DUMP",
    "WORKER_CLOSE_ROUTER",
    "ROUTER_DUMP",
    "ROUTER_CREATE_WEBRTC_TRANSPORT",
    "ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER",
    "ROUTER_CREATE_PLAIN_TRANSPORT",
    "ROUTER_CREATE_PIPE_TRANSPORT",
    "ROUTER_CREATE_DIRECT_TRANSPORT",
    "ROUTER_CLOSE_TRANSPORT",
    "ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER",
    "ROUTER_CREATE_AUDIO_LEVEL_OBSERVER",
    "ROUTER_CLOSE_RTP_OBSERVER",
    "TRANSPORT_DUMP",
    "TRANSPORT_GET_STATS",
    "TRANSPORT_CONNECT",
    "TRANSPORT_SET_MAX_INCOMING_BITRATE",
    "TRANSPORT_SET_MAX_OUTGOING_BITRATE",
    "TRANSPORT_RESTART_ICE",
    "TRANSPORT_PRODUCE",
    "TRANSPORT_PRODUCE_DATA",
    "TRANSPORT_CONSUME",
    "TRANSPORT_CONSUME_DATA",
    "TRANSPORT_ENABLE_TRACE_EVENT",
    "TRANSPORT_CLOSE_PRODUCER",
    "TRANSPORT_CLOSE_CONSUMER",
    "TRANSPORT_CLOSE_DATA_PRODUCER",
    "TRANSPORT_CLOSE_DATA_CONSUMER",
    "PRODUCER_DUMP",
    "PRODUCER_GET_STATS",
    "PRODUCER_PAUSE",
    "PRODUCER_RESUME",
    "PRODUCER_ENABLE_TRACE_EVENT",
    "CONSUMER_DUMP",
    "CONSUMER_GET_STATS",
    "CONSUMER_PAUSE",
    "CONSUMER_RESUME",
    "CONSUMER_SET_PREFERRED_LAYERS",
    "CONSUMER_SET_PRIORITY",
    "CONSUMER_REQUEST_KEY_FRAME",
    "CONSUMER_ENABLE_TRACE_EVENT",
    "DATA_PRODUCER_DUMP",
    "DATA_PRODUCER_GET_STATS",
    "DATA_CONSUMER_DUMP",
    "DATA_CONSUMER_GET_STATS",
    "DATA_CONSUMER_GET_BUFFERED_AMOUNT",
    "DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD",
    "RTP_OBSERVER_PAUSE",
    "RTP_OBSERVER_RESUME",
    "RTP_OBSERVER_ADD_PRODUCER",
    "RTP_OBSERVER_REMOVE_PRODUCER",
    nullptr
  };
  return names;
}

inline const char *EnumNameMethod(Method e) {
  if (flatbuffers::IsOutRange(e, Method::WORKER_CLOSE, Method::RTP_OBSERVER_REMOVE_PRODUCER)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesMethod()[index];
}

enum class Body : uint8_t {
  NONE = 0,
  FBS_Worker_UpdateableSettings = 1,
  FBS_Worker_CreateWebRtcServerRequest = 2,
  FBS_Worker_CloseWebRtcServerRequest = 3,
  FBS_Transport_ConsumeRequest = 4,
  MIN = NONE,
  MAX = FBS_Transport_ConsumeRequest
};

inline const Body (&EnumValuesBody())[5] {
  static const Body values[] = {
    Body::NONE,
    Body::FBS_Worker_UpdateableSettings,
    Body::FBS_Worker_CreateWebRtcServerRequest,
    Body::FBS_Worker_CloseWebRtcServerRequest,
    Body::FBS_Transport_ConsumeRequest
  };
  return values;
}

inline const char * const *EnumNamesBody() {
  static const char * const names[6] = {
    "NONE",
    "FBS_Worker_UpdateableSettings",
    "FBS_Worker_CreateWebRtcServerRequest",
    "FBS_Worker_CloseWebRtcServerRequest",
    "FBS_Transport_ConsumeRequest",
    nullptr
  };
  return names;
}

inline const char *EnumNameBody(Body e) {
  if (flatbuffers::IsOutRange(e, Body::NONE, Body::FBS_Transport_ConsumeRequest)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesBody()[index];
}

template<typename T> struct BodyTraits {
  static const Body enum_value = Body::NONE;
};

template<> struct BodyTraits<FBS::Worker::UpdateableSettings> {
  static const Body enum_value = Body::FBS_Worker_UpdateableSettings;
};

template<> struct BodyTraits<FBS::Worker::CreateWebRtcServerRequest> {
  static const Body enum_value = Body::FBS_Worker_CreateWebRtcServerRequest;
};

template<> struct BodyTraits<FBS::Worker::CloseWebRtcServerRequest> {
  static const Body enum_value = Body::FBS_Worker_CloseWebRtcServerRequest;
};

template<> struct BodyTraits<FBS::Transport::ConsumeRequest> {
  static const Body enum_value = Body::FBS_Transport_ConsumeRequest;
};

bool VerifyBody(flatbuffers::Verifier &verifier, const void *obj, Body type);
bool VerifyBodyVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<Body> *types);

struct Request FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef RequestBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return RequestTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ID = 4,
    VT_METHOD = 6,
    VT_HANDLERID = 8,
    VT_BODY_TYPE = 10,
    VT_BODY = 12
  };
  uint32_t id() const {
    return GetField<uint32_t>(VT_ID, 0);
  }
  FBS::Request::Method method() const {
    return static_cast<FBS::Request::Method>(GetField<uint8_t>(VT_METHOD, 0));
  }
  const flatbuffers::String *handlerId() const {
    return GetPointer<const flatbuffers::String *>(VT_HANDLERID);
  }
  FBS::Request::Body body_type() const {
    return static_cast<FBS::Request::Body>(GetField<uint8_t>(VT_BODY_TYPE, 0));
  }
  const void *body() const {
    return GetPointer<const void *>(VT_BODY);
  }
  template<typename T> const T *body_as() const;
  const FBS::Worker::UpdateableSettings *body_as_FBS_Worker_UpdateableSettings() const {
    return body_type() == FBS::Request::Body::FBS_Worker_UpdateableSettings ? static_cast<const FBS::Worker::UpdateableSettings *>(body()) : nullptr;
  }
  const FBS::Worker::CreateWebRtcServerRequest *body_as_FBS_Worker_CreateWebRtcServerRequest() const {
    return body_type() == FBS::Request::Body::FBS_Worker_CreateWebRtcServerRequest ? static_cast<const FBS::Worker::CreateWebRtcServerRequest *>(body()) : nullptr;
  }
  const FBS::Worker::CloseWebRtcServerRequest *body_as_FBS_Worker_CloseWebRtcServerRequest() const {
    return body_type() == FBS::Request::Body::FBS_Worker_CloseWebRtcServerRequest ? static_cast<const FBS::Worker::CloseWebRtcServerRequest *>(body()) : nullptr;
  }
  const FBS::Transport::ConsumeRequest *body_as_FBS_Transport_ConsumeRequest() const {
    return body_type() == FBS::Request::Body::FBS_Transport_ConsumeRequest ? static_cast<const FBS::Transport::ConsumeRequest *>(body()) : nullptr;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ID, 4) &&
           VerifyField<uint8_t>(verifier, VT_METHOD, 1) &&
           VerifyOffset(verifier, VT_HANDLERID) &&
           verifier.VerifyString(handlerId()) &&
           VerifyField<uint8_t>(verifier, VT_BODY_TYPE, 1) &&
           VerifyOffset(verifier, VT_BODY) &&
           VerifyBody(verifier, body(), body_type()) &&
           verifier.EndTable();
  }
};

template<> inline const FBS::Worker::UpdateableSettings *Request::body_as<FBS::Worker::UpdateableSettings>() const {
  return body_as_FBS_Worker_UpdateableSettings();
}

template<> inline const FBS::Worker::CreateWebRtcServerRequest *Request::body_as<FBS::Worker::CreateWebRtcServerRequest>() const {
  return body_as_FBS_Worker_CreateWebRtcServerRequest();
}

template<> inline const FBS::Worker::CloseWebRtcServerRequest *Request::body_as<FBS::Worker::CloseWebRtcServerRequest>() const {
  return body_as_FBS_Worker_CloseWebRtcServerRequest();
}

template<> inline const FBS::Transport::ConsumeRequest *Request::body_as<FBS::Transport::ConsumeRequest>() const {
  return body_as_FBS_Transport_ConsumeRequest();
}

struct RequestBuilder {
  typedef Request Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(uint32_t id) {
    fbb_.AddElement<uint32_t>(Request::VT_ID, id, 0);
  }
  void add_method(FBS::Request::Method method) {
    fbb_.AddElement<uint8_t>(Request::VT_METHOD, static_cast<uint8_t>(method), 0);
  }
  void add_handlerId(flatbuffers::Offset<flatbuffers::String> handlerId) {
    fbb_.AddOffset(Request::VT_HANDLERID, handlerId);
  }
  void add_body_type(FBS::Request::Body body_type) {
    fbb_.AddElement<uint8_t>(Request::VT_BODY_TYPE, static_cast<uint8_t>(body_type), 0);
  }
  void add_body(flatbuffers::Offset<void> body) {
    fbb_.AddOffset(Request::VT_BODY, body);
  }
  explicit RequestBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<Request> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Request>(end);
    return o;
  }
};

inline flatbuffers::Offset<Request> CreateRequest(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    FBS::Request::Method method = FBS::Request::Method::WORKER_CLOSE,
    flatbuffers::Offset<flatbuffers::String> handlerId = 0,
    FBS::Request::Body body_type = FBS::Request::Body::NONE,
    flatbuffers::Offset<void> body = 0) {
  RequestBuilder builder_(_fbb);
  builder_.add_body(body);
  builder_.add_handlerId(handlerId);
  builder_.add_id(id);
  builder_.add_body_type(body_type);
  builder_.add_method(method);
  return builder_.Finish();
}

inline flatbuffers::Offset<Request> CreateRequestDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    FBS::Request::Method method = FBS::Request::Method::WORKER_CLOSE,
    const char *handlerId = nullptr,
    FBS::Request::Body body_type = FBS::Request::Body::NONE,
    flatbuffers::Offset<void> body = 0) {
  auto handlerId__ = handlerId ? _fbb.CreateString(handlerId) : 0;
  return FBS::Request::CreateRequest(
      _fbb,
      id,
      method,
      handlerId__,
      body_type,
      body);
}

inline bool VerifyBody(flatbuffers::Verifier &verifier, const void *obj, Body type) {
  switch (type) {
    case Body::NONE: {
      return true;
    }
    case Body::FBS_Worker_UpdateableSettings: {
      auto ptr = reinterpret_cast<const FBS::Worker::UpdateableSettings *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Body::FBS_Worker_CreateWebRtcServerRequest: {
      auto ptr = reinterpret_cast<const FBS::Worker::CreateWebRtcServerRequest *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Body::FBS_Worker_CloseWebRtcServerRequest: {
      auto ptr = reinterpret_cast<const FBS::Worker::CloseWebRtcServerRequest *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Body::FBS_Transport_ConsumeRequest: {
      auto ptr = reinterpret_cast<const FBS::Transport::ConsumeRequest *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return true;
  }
}

inline bool VerifyBodyVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<Body> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyBody(
        verifier,  values->Get(i), types->GetEnum<Body>(i))) {
      return false;
    }
  }
  return true;
}

inline const flatbuffers::TypeTable *MethodTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_UCHAR, 0, 0 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    FBS::Request::MethodTypeTable
  };
  static const char * const names[] = {
    "WORKER_CLOSE",
    "WORKER_DUMP",
    "WORKER_GET_RESOURCE_USAGE",
    "WORKER_UPDATE_SETTINGS",
    "WORKER_CREATE_WEBRTC_SERVER",
    "WORKER_CREATE_ROUTER",
    "WORKER_WEBRTC_SERVER_CLOSE",
    "WEBRTC_SERVER_DUMP",
    "WORKER_CLOSE_ROUTER",
    "ROUTER_DUMP",
    "ROUTER_CREATE_WEBRTC_TRANSPORT",
    "ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER",
    "ROUTER_CREATE_PLAIN_TRANSPORT",
    "ROUTER_CREATE_PIPE_TRANSPORT",
    "ROUTER_CREATE_DIRECT_TRANSPORT",
    "ROUTER_CLOSE_TRANSPORT",
    "ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER",
    "ROUTER_CREATE_AUDIO_LEVEL_OBSERVER",
    "ROUTER_CLOSE_RTP_OBSERVER",
    "TRANSPORT_DUMP",
    "TRANSPORT_GET_STATS",
    "TRANSPORT_CONNECT",
    "TRANSPORT_SET_MAX_INCOMING_BITRATE",
    "TRANSPORT_SET_MAX_OUTGOING_BITRATE",
    "TRANSPORT_RESTART_ICE",
    "TRANSPORT_PRODUCE",
    "TRANSPORT_PRODUCE_DATA",
    "TRANSPORT_CONSUME",
    "TRANSPORT_CONSUME_DATA",
    "TRANSPORT_ENABLE_TRACE_EVENT",
    "TRANSPORT_CLOSE_PRODUCER",
    "TRANSPORT_CLOSE_CONSUMER",
    "TRANSPORT_CLOSE_DATA_PRODUCER",
    "TRANSPORT_CLOSE_DATA_CONSUMER",
    "PRODUCER_DUMP",
    "PRODUCER_GET_STATS",
    "PRODUCER_PAUSE",
    "PRODUCER_RESUME",
    "PRODUCER_ENABLE_TRACE_EVENT",
    "CONSUMER_DUMP",
    "CONSUMER_GET_STATS",
    "CONSUMER_PAUSE",
    "CONSUMER_RESUME",
    "CONSUMER_SET_PREFERRED_LAYERS",
    "CONSUMER_SET_PRIORITY",
    "CONSUMER_REQUEST_KEY_FRAME",
    "CONSUMER_ENABLE_TRACE_EVENT",
    "DATA_PRODUCER_DUMP",
    "DATA_PRODUCER_GET_STATS",
    "DATA_CONSUMER_DUMP",
    "DATA_CONSUMER_GET_STATS",
    "DATA_CONSUMER_GET_BUFFERED_AMOUNT",
    "DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD",
    "RTP_OBSERVER_PAUSE",
    "RTP_OBSERVER_RESUME",
    "RTP_OBSERVER_ADD_PRODUCER",
    "RTP_OBSERVER_REMOVE_PRODUCER"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_ENUM, 57, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *BodyTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_SEQUENCE, 0, -1 },
    { flatbuffers::ET_SEQUENCE, 0, 0 },
    { flatbuffers::ET_SEQUENCE, 0, 1 },
    { flatbuffers::ET_SEQUENCE, 0, 2 },
    { flatbuffers::ET_SEQUENCE, 0, 3 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    FBS::Worker::UpdateableSettingsTypeTable,
    FBS::Worker::CreateWebRtcServerRequestTypeTable,
    FBS::Worker::CloseWebRtcServerRequestTypeTable,
    FBS::Transport::ConsumeRequestTypeTable
  };
  static const char * const names[] = {
    "NONE",
    "FBS_Worker_UpdateableSettings",
    "FBS_Worker_CreateWebRtcServerRequest",
    "FBS_Worker_CloseWebRtcServerRequest",
    "FBS_Transport_ConsumeRequest"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_UNION, 5, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *RequestTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_UINT, 0, -1 },
    { flatbuffers::ET_UCHAR, 0, 0 },
    { flatbuffers::ET_STRING, 0, -1 },
    { flatbuffers::ET_UTYPE, 0, 1 },
    { flatbuffers::ET_SEQUENCE, 0, 1 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    FBS::Request::MethodTypeTable,
    FBS::Request::BodyTypeTable
  };
  static const char * const names[] = {
    "id",
    "method",
    "handlerId",
    "body_type",
    "body"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 5, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const FBS::Request::Request *GetRequest(const void *buf) {
  return flatbuffers::GetRoot<FBS::Request::Request>(buf);
}

inline const FBS::Request::Request *GetSizePrefixedRequest(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<FBS::Request::Request>(buf);
}

inline bool VerifyRequestBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<FBS::Request::Request>(nullptr);
}

inline bool VerifySizePrefixedRequestBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<FBS::Request::Request>(nullptr);
}

inline void FinishRequestBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<FBS::Request::Request> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedRequestBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<FBS::Request::Request> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace Request
}  // namespace FBS

#endif  // FLATBUFFERS_GENERATED_REQUEST_FBS_REQUEST_H_
