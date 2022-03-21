/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: s3_packet_header.proto */

#ifndef PROTOBUF_C_s3_5fpacket_5fheader_2eproto__INCLUDED
#define PROTOBUF_C_s3_5fpacket_5fheader_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1004000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct S3PacketHeader S3PacketHeader;


/* --- enums --- */


/* --- messages --- */

struct  S3PacketHeader
{
  ProtobufCMessage base;
  int32_t pcode;
  uint64_t session_id;
  int64_t data_len;
};
#define S3_PACKET_HEADER__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&s3_packet_header__descriptor) \
    , 0, 0, 0 }


/* S3PacketHeader methods */
void   s3_packet_header__init
                     (S3PacketHeader         *message);
size_t s3_packet_header__get_packed_size
                     (const S3PacketHeader   *message);
size_t s3_packet_header__pack
                     (const S3PacketHeader   *message,
                      uint8_t             *out);
size_t s3_packet_header__pack_to_buffer
                     (const S3PacketHeader   *message,
                      ProtobufCBuffer     *buffer);
S3PacketHeader *
       s3_packet_header__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   s3_packet_header__free_unpacked
                     (S3PacketHeader *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*S3PacketHeader_Closure)
                 (const S3PacketHeader *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor s3_packet_header__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_s3_5fpacket_5fheader_2eproto__INCLUDED */
