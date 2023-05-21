// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: add.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_add_2eproto_2epb_2eh
#define GOOGLE_PROTOBUF_INCLUDED_add_2eproto_2epb_2eh

#include <limits>
#include <string>
#include <type_traits>

#include "google/protobuf/port_def.inc"
#if PROTOBUF_VERSION < 4023000
#error "This file was generated by a newer version of protoc which is"
#error "incompatible with your Protocol Buffer headers. Please update"
#error "your headers."
#endif  // PROTOBUF_VERSION

#if 4023001 < PROTOBUF_MIN_PROTOC_VERSION
#error "This file was generated by an older version of protoc which is"
#error "incompatible with your Protocol Buffer headers. Please"
#error "regenerate this file with a newer version of protoc."
#endif  // PROTOBUF_MIN_PROTOC_VERSION
#include "google/protobuf/port_undef.inc"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/message.h"
#include "google/protobuf/repeated_field.h"  // IWYU pragma: export
#include "google/protobuf/extension_set.h"  // IWYU pragma: export
#include "google/protobuf/unknown_field_set.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"

#define PROTOBUF_INTERNAL_EXPORT_add_2eproto

PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_add_2eproto {
  static const ::uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable
    descriptor_table_add_2eproto;
class add_req;
struct add_reqDefaultTypeInternal;
extern add_reqDefaultTypeInternal _add_req_default_instance_;
class add_rsp;
struct add_rspDefaultTypeInternal;
extern add_rspDefaultTypeInternal _add_rsp_default_instance_;
PROTOBUF_NAMESPACE_OPEN
template <>
::add_req* Arena::CreateMaybeMessage<::add_req>(Arena*);
template <>
::add_rsp* Arena::CreateMaybeMessage<::add_rsp>(Arena*);
PROTOBUF_NAMESPACE_CLOSE


// ===================================================================


// -------------------------------------------------------------------

class add_req final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:add_req) */ {
 public:
  inline add_req() : add_req(nullptr) {}
  ~add_req() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR add_req(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  add_req(const add_req& from);
  add_req(add_req&& from) noexcept
    : add_req() {
    *this = ::std::move(from);
  }

  inline add_req& operator=(const add_req& from) {
    CopyFrom(from);
    return *this;
  }
  inline add_req& operator=(add_req&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const add_req& default_instance() {
    return *internal_default_instance();
  }
  static inline const add_req* internal_default_instance() {
    return reinterpret_cast<const add_req*>(
               &_add_req_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(add_req& a, add_req& b) {
    a.Swap(&b);
  }
  inline void Swap(add_req* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(add_req* other) {
    if (other == this) return;
    ABSL_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  add_req* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<add_req>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const add_req& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const add_req& from) {
    add_req::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(add_req* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "add_req";
  }
  protected:
  explicit add_req(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kLeftFieldNumber = 1,
    kRightFieldNumber = 2,
  };
  // int32 left = 1;
  void clear_left() ;
  ::int32_t left() const;
  void set_left(::int32_t value);

  private:
  ::int32_t _internal_left() const;
  void _internal_set_left(::int32_t value);

  public:
  // int32 right = 2;
  void clear_right() ;
  ::int32_t right() const;
  void set_right(::int32_t value);

  private:
  ::int32_t _internal_right() const;
  void _internal_set_right(::int32_t value);

  public:
  // @@protoc_insertion_point(class_scope:add_req)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::int32_t left_;
    ::int32_t right_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_add_2eproto;
};// -------------------------------------------------------------------

class add_rsp final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:add_rsp) */ {
 public:
  inline add_rsp() : add_rsp(nullptr) {}
  ~add_rsp() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR add_rsp(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  add_rsp(const add_rsp& from);
  add_rsp(add_rsp&& from) noexcept
    : add_rsp() {
    *this = ::std::move(from);
  }

  inline add_rsp& operator=(const add_rsp& from) {
    CopyFrom(from);
    return *this;
  }
  inline add_rsp& operator=(add_rsp&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const add_rsp& default_instance() {
    return *internal_default_instance();
  }
  static inline const add_rsp* internal_default_instance() {
    return reinterpret_cast<const add_rsp*>(
               &_add_rsp_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(add_rsp& a, add_rsp& b) {
    a.Swap(&b);
  }
  inline void Swap(add_rsp* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(add_rsp* other) {
    if (other == this) return;
    ABSL_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  add_rsp* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<add_rsp>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const add_rsp& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const add_rsp& from) {
    add_rsp::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(add_rsp* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "add_rsp";
  }
  protected:
  explicit add_rsp(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kResultFieldNumber = 1,
  };
  // int32 result = 1;
  void clear_result() ;
  ::int32_t result() const;
  void set_result(::int32_t value);

  private:
  ::int32_t _internal_result() const;
  void _internal_set_result(::int32_t value);

  public:
  // @@protoc_insertion_point(class_scope:add_rsp)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::int32_t result_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_add_2eproto;
};

// ===================================================================




// ===================================================================


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// -------------------------------------------------------------------

// add_req

// int32 left = 1;
inline void add_req::clear_left() {
  _impl_.left_ = 0;
}
inline ::int32_t add_req::left() const {
  // @@protoc_insertion_point(field_get:add_req.left)
  return _internal_left();
}
inline void add_req::set_left(::int32_t value) {
  _internal_set_left(value);
  // @@protoc_insertion_point(field_set:add_req.left)
}
inline ::int32_t add_req::_internal_left() const {
  return _impl_.left_;
}
inline void add_req::_internal_set_left(::int32_t value) {
  ;
  _impl_.left_ = value;
}

// int32 right = 2;
inline void add_req::clear_right() {
  _impl_.right_ = 0;
}
inline ::int32_t add_req::right() const {
  // @@protoc_insertion_point(field_get:add_req.right)
  return _internal_right();
}
inline void add_req::set_right(::int32_t value) {
  _internal_set_right(value);
  // @@protoc_insertion_point(field_set:add_req.right)
}
inline ::int32_t add_req::_internal_right() const {
  return _impl_.right_;
}
inline void add_req::_internal_set_right(::int32_t value) {
  ;
  _impl_.right_ = value;
}

// -------------------------------------------------------------------

// add_rsp

// int32 result = 1;
inline void add_rsp::clear_result() {
  _impl_.result_ = 0;
}
inline ::int32_t add_rsp::result() const {
  // @@protoc_insertion_point(field_get:add_rsp.result)
  return _internal_result();
}
inline void add_rsp::set_result(::int32_t value) {
  _internal_set_result(value);
  // @@protoc_insertion_point(field_set:add_rsp.result)
}
inline ::int32_t add_rsp::_internal_result() const {
  return _impl_.result_;
}
inline void add_rsp::_internal_set_result(::int32_t value) {
  ;
  _impl_.result_ = value;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)


// @@protoc_insertion_point(global_scope)

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INCLUDED_add_2eproto_2epb_2eh
