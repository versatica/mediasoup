// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_COMMON_FBS_COMMON_H_
#define FLATBUFFERS_GENERATED_COMMON_FBS_COMMON_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 2 &&
              FLATBUFFERS_VERSION_MINOR == 0 &&
              FLATBUFFERS_VERSION_REVISION == 8,
             "Non-compatible flatbuffers version included");

namespace FBS {
namespace Common {

struct StringString;
struct StringStringBuilder;

struct StringUint8;
struct StringUint8Builder;

struct Uint16String;
struct Uint16StringBuilder;

struct Uint32String;
struct Uint32StringBuilder;

struct StringStringArray;
struct StringStringArrayBuilder;

inline const flatbuffers::TypeTable *StringStringTypeTable();

inline const flatbuffers::TypeTable *StringUint8TypeTable();

inline const flatbuffers::TypeTable *Uint16StringTypeTable();

inline const flatbuffers::TypeTable *Uint32StringTypeTable();

inline const flatbuffers::TypeTable *StringStringArrayTypeTable();

struct StringString FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef StringStringBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return StringStringTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_KEY = 4,
    VT_VALUE = 6
  };
  const flatbuffers::String *key() const {
    return GetPointer<const flatbuffers::String *>(VT_KEY);
  }
  const flatbuffers::String *value() const {
    return GetPointer<const flatbuffers::String *>(VT_VALUE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffsetRequired(verifier, VT_KEY) &&
           verifier.VerifyString(key()) &&
           VerifyOffsetRequired(verifier, VT_VALUE) &&
           verifier.VerifyString(value()) &&
           verifier.EndTable();
  }
};

struct StringStringBuilder {
  typedef StringString Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_key(flatbuffers::Offset<flatbuffers::String> key) {
    fbb_.AddOffset(StringString::VT_KEY, key);
  }
  void add_value(flatbuffers::Offset<flatbuffers::String> value) {
    fbb_.AddOffset(StringString::VT_VALUE, value);
  }
  explicit StringStringBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<StringString> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<StringString>(end);
    fbb_.Required(o, StringString::VT_KEY);
    fbb_.Required(o, StringString::VT_VALUE);
    return o;
  }
};

inline flatbuffers::Offset<StringString> CreateStringString(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> key = 0,
    flatbuffers::Offset<flatbuffers::String> value = 0) {
  StringStringBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_key(key);
  return builder_.Finish();
}

inline flatbuffers::Offset<StringString> CreateStringStringDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *key = nullptr,
    const char *value = nullptr) {
  auto key__ = key ? _fbb.CreateString(key) : 0;
  auto value__ = value ? _fbb.CreateString(value) : 0;
  return FBS::Common::CreateStringString(
      _fbb,
      key__,
      value__);
}

struct StringUint8 FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef StringUint8Builder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return StringUint8TypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_KEY = 4,
    VT_VALUE = 6
  };
  const flatbuffers::String *key() const {
    return GetPointer<const flatbuffers::String *>(VT_KEY);
  }
  uint8_t value() const {
    return GetField<uint8_t>(VT_VALUE, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffsetRequired(verifier, VT_KEY) &&
           verifier.VerifyString(key()) &&
           VerifyField<uint8_t>(verifier, VT_VALUE, 1) &&
           verifier.EndTable();
  }
};

struct StringUint8Builder {
  typedef StringUint8 Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_key(flatbuffers::Offset<flatbuffers::String> key) {
    fbb_.AddOffset(StringUint8::VT_KEY, key);
  }
  void add_value(uint8_t value) {
    fbb_.AddElement<uint8_t>(StringUint8::VT_VALUE, value, 0);
  }
  explicit StringUint8Builder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<StringUint8> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<StringUint8>(end);
    fbb_.Required(o, StringUint8::VT_KEY);
    return o;
  }
};

inline flatbuffers::Offset<StringUint8> CreateStringUint8(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> key = 0,
    uint8_t value = 0) {
  StringUint8Builder builder_(_fbb);
  builder_.add_key(key);
  builder_.add_value(value);
  return builder_.Finish();
}

inline flatbuffers::Offset<StringUint8> CreateStringUint8Direct(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *key = nullptr,
    uint8_t value = 0) {
  auto key__ = key ? _fbb.CreateString(key) : 0;
  return FBS::Common::CreateStringUint8(
      _fbb,
      key__,
      value);
}

struct Uint16String FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef Uint16StringBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return Uint16StringTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_KEY = 4,
    VT_VALUE = 6
  };
  uint16_t key() const {
    return GetField<uint16_t>(VT_KEY, 0);
  }
  const flatbuffers::String *value() const {
    return GetPointer<const flatbuffers::String *>(VT_VALUE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_KEY, 2) &&
           VerifyOffsetRequired(verifier, VT_VALUE) &&
           verifier.VerifyString(value()) &&
           verifier.EndTable();
  }
};

struct Uint16StringBuilder {
  typedef Uint16String Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_key(uint16_t key) {
    fbb_.AddElement<uint16_t>(Uint16String::VT_KEY, key, 0);
  }
  void add_value(flatbuffers::Offset<flatbuffers::String> value) {
    fbb_.AddOffset(Uint16String::VT_VALUE, value);
  }
  explicit Uint16StringBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<Uint16String> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Uint16String>(end);
    fbb_.Required(o, Uint16String::VT_VALUE);
    return o;
  }
};

inline flatbuffers::Offset<Uint16String> CreateUint16String(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint16_t key = 0,
    flatbuffers::Offset<flatbuffers::String> value = 0) {
  Uint16StringBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_key(key);
  return builder_.Finish();
}

inline flatbuffers::Offset<Uint16String> CreateUint16StringDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint16_t key = 0,
    const char *value = nullptr) {
  auto value__ = value ? _fbb.CreateString(value) : 0;
  return FBS::Common::CreateUint16String(
      _fbb,
      key,
      value__);
}

struct Uint32String FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef Uint32StringBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return Uint32StringTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_KEY = 4,
    VT_VALUE = 6
  };
  uint32_t key() const {
    return GetField<uint32_t>(VT_KEY, 0);
  }
  const flatbuffers::String *value() const {
    return GetPointer<const flatbuffers::String *>(VT_VALUE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_KEY, 4) &&
           VerifyOffsetRequired(verifier, VT_VALUE) &&
           verifier.VerifyString(value()) &&
           verifier.EndTable();
  }
};

struct Uint32StringBuilder {
  typedef Uint32String Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_key(uint32_t key) {
    fbb_.AddElement<uint32_t>(Uint32String::VT_KEY, key, 0);
  }
  void add_value(flatbuffers::Offset<flatbuffers::String> value) {
    fbb_.AddOffset(Uint32String::VT_VALUE, value);
  }
  explicit Uint32StringBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<Uint32String> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Uint32String>(end);
    fbb_.Required(o, Uint32String::VT_VALUE);
    return o;
  }
};

inline flatbuffers::Offset<Uint32String> CreateUint32String(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t key = 0,
    flatbuffers::Offset<flatbuffers::String> value = 0) {
  Uint32StringBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_key(key);
  return builder_.Finish();
}

inline flatbuffers::Offset<Uint32String> CreateUint32StringDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t key = 0,
    const char *value = nullptr) {
  auto value__ = value ? _fbb.CreateString(value) : 0;
  return FBS::Common::CreateUint32String(
      _fbb,
      key,
      value__);
}

struct StringStringArray FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef StringStringArrayBuilder Builder;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return StringStringArrayTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_KEY = 4,
    VT_VALUES = 6
  };
  const flatbuffers::String *key() const {
    return GetPointer<const flatbuffers::String *>(VT_KEY);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *values() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_VALUES);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffsetRequired(verifier, VT_KEY) &&
           verifier.VerifyString(key()) &&
           VerifyOffset(verifier, VT_VALUES) &&
           verifier.VerifyVector(values()) &&
           verifier.VerifyVectorOfStrings(values()) &&
           verifier.EndTable();
  }
};

struct StringStringArrayBuilder {
  typedef StringStringArray Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_key(flatbuffers::Offset<flatbuffers::String> key) {
    fbb_.AddOffset(StringStringArray::VT_KEY, key);
  }
  void add_values(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> values) {
    fbb_.AddOffset(StringStringArray::VT_VALUES, values);
  }
  explicit StringStringArrayBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<StringStringArray> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<StringStringArray>(end);
    fbb_.Required(o, StringStringArray::VT_KEY);
    return o;
  }
};

inline flatbuffers::Offset<StringStringArray> CreateStringStringArray(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> key = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> values = 0) {
  StringStringArrayBuilder builder_(_fbb);
  builder_.add_values(values);
  builder_.add_key(key);
  return builder_.Finish();
}

inline flatbuffers::Offset<StringStringArray> CreateStringStringArrayDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *key = nullptr,
    const std::vector<flatbuffers::Offset<flatbuffers::String>> *values = nullptr) {
  auto key__ = key ? _fbb.CreateString(key) : 0;
  auto values__ = values ? _fbb.CreateVector<flatbuffers::Offset<flatbuffers::String>>(*values) : 0;
  return FBS::Common::CreateStringStringArray(
      _fbb,
      key__,
      values__);
}

inline const flatbuffers::TypeTable *StringStringTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_STRING, 0, -1 },
    { flatbuffers::ET_STRING, 0, -1 }
  };
  static const char * const names[] = {
    "key",
    "value"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *StringUint8TypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_STRING, 0, -1 },
    { flatbuffers::ET_UCHAR, 0, -1 }
  };
  static const char * const names[] = {
    "key",
    "value"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *Uint16StringTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_USHORT, 0, -1 },
    { flatbuffers::ET_STRING, 0, -1 }
  };
  static const char * const names[] = {
    "key",
    "value"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *Uint32StringTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_UINT, 0, -1 },
    { flatbuffers::ET_STRING, 0, -1 }
  };
  static const char * const names[] = {
    "key",
    "value"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

inline const flatbuffers::TypeTable *StringStringArrayTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_STRING, 0, -1 },
    { flatbuffers::ET_STRING, 1, -1 }
  };
  static const char * const names[] = {
    "key",
    "values"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

}  // namespace Common
}  // namespace FBS

#endif  // FLATBUFFERS_GENERATED_COMMON_FBS_COMMON_H_
