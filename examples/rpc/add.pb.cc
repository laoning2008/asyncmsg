// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: add.proto

#include "add.pb.h"

#include <algorithm>
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/wire_format.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"
PROTOBUF_PRAGMA_INIT_SEG
namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = ::PROTOBUF_NAMESPACE_ID::internal;
template <typename>
PROTOBUF_CONSTEXPR add_req::add_req(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.left_)*/ 0

  , /*decltype(_impl_.right_)*/ 0

  , /*decltype(_impl_._cached_size_)*/{}} {}
struct add_reqDefaultTypeInternal {
  PROTOBUF_CONSTEXPR add_reqDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~add_reqDefaultTypeInternal() {}
  union {
    add_req _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 add_reqDefaultTypeInternal _add_req_default_instance_;
template <typename>
PROTOBUF_CONSTEXPR add_rsp::add_rsp(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.result_)*/ 0

  , /*decltype(_impl_._cached_size_)*/{}} {}
struct add_rspDefaultTypeInternal {
  PROTOBUF_CONSTEXPR add_rspDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~add_rspDefaultTypeInternal() {}
  union {
    add_rsp _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 add_rspDefaultTypeInternal _add_rsp_default_instance_;
static ::_pb::Metadata file_level_metadata_add_2eproto[2];
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_add_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_add_2eproto = nullptr;
const ::uint32_t TableStruct_add_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(
    protodesc_cold) = {
    ~0u,  // no _has_bits_
    PROTOBUF_FIELD_OFFSET(::add_req, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    ~0u,  // no _weak_field_map_
    ~0u,  // no _inlined_string_donated_
    ~0u,  // no _split_
    ~0u,  // no sizeof(Split)
    PROTOBUF_FIELD_OFFSET(::add_req, _impl_.left_),
    PROTOBUF_FIELD_OFFSET(::add_req, _impl_.right_),
    ~0u,  // no _has_bits_
    PROTOBUF_FIELD_OFFSET(::add_rsp, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    ~0u,  // no _weak_field_map_
    ~0u,  // no _inlined_string_donated_
    ~0u,  // no _split_
    ~0u,  // no sizeof(Split)
    PROTOBUF_FIELD_OFFSET(::add_rsp, _impl_.result_),
};

static const ::_pbi::MigrationSchema
    schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
        { 0, -1, -1, sizeof(::add_req)},
        { 10, -1, -1, sizeof(::add_rsp)},
};

static const ::_pb::Message* const file_default_instances[] = {
    &::_add_req_default_instance_._instance,
    &::_add_rsp_default_instance_._instance,
};
const char descriptor_table_protodef_add_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
    "\n\tadd.proto\"&\n\007add_req\022\014\n\004left\030\001 \001(\005\022\r\n\005"
    "right\030\002 \001(\005\"\031\n\007add_rsp\022\016\n\006result\030\001 \001(\005b\006"
    "proto3"
};
static ::absl::once_flag descriptor_table_add_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_add_2eproto = {
    false,
    false,
    86,
    descriptor_table_protodef_add_2eproto,
    "add.proto",
    &descriptor_table_add_2eproto_once,
    nullptr,
    0,
    2,
    schemas,
    file_default_instances,
    TableStruct_add_2eproto::offsets,
    file_level_metadata_add_2eproto,
    file_level_enum_descriptors_add_2eproto,
    file_level_service_descriptors_add_2eproto,
};

// This function exists to be marked as weak.
// It can significantly speed up compilation by breaking up LLVM's SCC
// in the .pb.cc translation units. Large translation units see a
// reduction of more than 35% of walltime for optimized builds. Without
// the weak attribute all the messages in the file, including all the
// vtables and everything they use become part of the same SCC through
// a cycle like:
// GetMetadata -> descriptor table -> default instances ->
//   vtables -> GetMetadata
// By adding a weak function here we break the connection from the
// individual vtables back into the descriptor table.
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_add_2eproto_getter() {
  return &descriptor_table_add_2eproto;
}
// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2
static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_add_2eproto(&descriptor_table_add_2eproto);
// ===================================================================

class add_req::_Internal {
 public:
};

add_req::add_req(::PROTOBUF_NAMESPACE_ID::Arena* arena)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena) {
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:add_req)
}
add_req::add_req(const add_req& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(), _impl_(from._impl_) {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(
      from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:add_req)
}

inline void add_req::SharedCtor(::_pb::Arena* arena) {
  (void)arena;
  new (&_impl_) Impl_{
      decltype(_impl_.left_) { 0 }

    , decltype(_impl_.right_) { 0 }

    , /*decltype(_impl_._cached_size_)*/{}
  };
}

add_req::~add_req() {
  // @@protoc_insertion_point(destructor:add_req)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void add_req::SharedDtor() {
  ABSL_DCHECK(GetArenaForAllocation() == nullptr);
}

void add_req::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void add_req::Clear() {
// @@protoc_insertion_point(message_clear_start:add_req)
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&_impl_.left_, 0, static_cast<::size_t>(
      reinterpret_cast<char*>(&_impl_.right_) -
      reinterpret_cast<char*>(&_impl_.left_)) + sizeof(_impl_.right_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* add_req::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // int32 left = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::uint8_t>(tag) == 8)) {
          _impl_.left_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else {
          goto handle_unusual;
        }
        continue;
      // int32 right = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::uint8_t>(tag) == 16)) {
          _impl_.right_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else {
          goto handle_unusual;
        }
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

::uint8_t* add_req::_InternalSerialize(
    ::uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:add_req)
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // int32 left = 1;
  if (this->_internal_left() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(
        1, this->_internal_left(), target);
  }

  // int32 right = 2;
  if (this->_internal_right() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(
        2, this->_internal_right(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:add_req)
  return target;
}

::size_t add_req::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:add_req)
  ::size_t total_size = 0;

  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // int32 left = 1;
  if (this->_internal_left() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_left());
  }

  // int32 right = 2;
  if (this->_internal_right() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_right());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData add_req::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    add_req::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*add_req::GetClassData() const { return &_class_data_; }


void add_req::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<add_req*>(&to_msg);
  auto& from = static_cast<const add_req&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:add_req)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_left() != 0) {
    _this->_internal_set_left(from._internal_left());
  }
  if (from._internal_right() != 0) {
    _this->_internal_set_right(from._internal_right());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void add_req::CopyFrom(const add_req& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:add_req)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool add_req::IsInitialized() const {
  return true;
}

void add_req::InternalSwap(add_req* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(add_req, _impl_.right_)
      + sizeof(add_req::_impl_.right_)
      - PROTOBUF_FIELD_OFFSET(add_req, _impl_.left_)>(
          reinterpret_cast<char*>(&_impl_.left_),
          reinterpret_cast<char*>(&other->_impl_.left_));
}

::PROTOBUF_NAMESPACE_ID::Metadata add_req::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_add_2eproto_getter, &descriptor_table_add_2eproto_once,
      file_level_metadata_add_2eproto[0]);
}
// ===================================================================

class add_rsp::_Internal {
 public:
};

add_rsp::add_rsp(::PROTOBUF_NAMESPACE_ID::Arena* arena)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena) {
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:add_rsp)
}
add_rsp::add_rsp(const add_rsp& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(), _impl_(from._impl_) {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(
      from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:add_rsp)
}

inline void add_rsp::SharedCtor(::_pb::Arena* arena) {
  (void)arena;
  new (&_impl_) Impl_{
      decltype(_impl_.result_) { 0 }

    , /*decltype(_impl_._cached_size_)*/{}
  };
}

add_rsp::~add_rsp() {
  // @@protoc_insertion_point(destructor:add_rsp)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void add_rsp::SharedDtor() {
  ABSL_DCHECK(GetArenaForAllocation() == nullptr);
}

void add_rsp::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void add_rsp::Clear() {
// @@protoc_insertion_point(message_clear_start:add_rsp)
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.result_ = 0;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* add_rsp::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // int32 result = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::uint8_t>(tag) == 8)) {
          _impl_.result_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else {
          goto handle_unusual;
        }
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

::uint8_t* add_rsp::_InternalSerialize(
    ::uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:add_rsp)
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // int32 result = 1;
  if (this->_internal_result() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(
        1, this->_internal_result(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:add_rsp)
  return target;
}

::size_t add_rsp::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:add_rsp)
  ::size_t total_size = 0;

  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // int32 result = 1;
  if (this->_internal_result() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_result());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData add_rsp::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    add_rsp::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*add_rsp::GetClassData() const { return &_class_data_; }


void add_rsp::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<add_rsp*>(&to_msg);
  auto& from = static_cast<const add_rsp&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:add_rsp)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_result() != 0) {
    _this->_internal_set_result(from._internal_result());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void add_rsp::CopyFrom(const add_rsp& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:add_rsp)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool add_rsp::IsInitialized() const {
  return true;
}

void add_rsp::InternalSwap(add_rsp* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);

  swap(_impl_.result_, other->_impl_.result_);
}

::PROTOBUF_NAMESPACE_ID::Metadata add_rsp::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_add_2eproto_getter, &descriptor_table_add_2eproto_once,
      file_level_metadata_add_2eproto[1]);
}
// @@protoc_insertion_point(namespace_scope)
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::add_req*
Arena::CreateMaybeMessage< ::add_req >(Arena* arena) {
  return Arena::CreateMessageInternal< ::add_req >(arena);
}
template<> PROTOBUF_NOINLINE ::add_rsp*
Arena::CreateMaybeMessage< ::add_rsp >(Arena* arena) {
  return Arena::CreateMessageInternal< ::add_rsp >(arena);
}
PROTOBUF_NAMESPACE_CLOSE
// @@protoc_insertion_point(global_scope)
#include "google/protobuf/port_undef.inc"
