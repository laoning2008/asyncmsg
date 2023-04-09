#pragma once

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__)
#   include <endian.h>
#   if !defined(__GLIBC__) || !defined(__GLIBC_MINOR__) || ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 9)))
#       include <arpa/inet.h>
#       if defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#           define htobe16(x) htons(x)
#           define htole16(x) (x)
#           define be16toh(x) ntohs(x)
#           define le16toh(x) (x)

#           define htobe32(x) htonl(x)
#           define htole32(x) (x)
#           define be32toh(x) ntohl(x)
#           define le32toh(x) (x)

#           define htobe64(x) (((uint64_t)htonl(((uint32_t)(((uint64_t)(x)) >> 32)))) | (((uint64_t)htonl(((uint32_t)(x)))) << 32))
#           define htole64(x) (x)
#           define be64toh(x) (((uint64_t)ntohl(((uint32_t)(((uint64_t)(x)) >> 32)))) | (((uint64_t)ntohl(((uint32_t)(x)))) << 32))
#           define le64toh(x) (x)
#       elif defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)
#           define htobe16(x) (x)
#           define htole16(x) ((((((uint16_t)(x)) >> 8))|((((uint16_t)(x)) << 8)))
#           define be16toh(x) (x)
#           define le16toh(x) ((((((uint16_t)(x)) >> 8))|((((uint16_t)(x)) << 8)))

#           define htobe32(x) (x)
#           define htole32(x) (((uint32_t)htole16(((uint16_t)(((uint32_t)(x)) >> 16)))) | (((uint32_t)htole16(((uint16_t)(x)))) << 16))
#           define be32toh(x) (x)
#           define le32toh(x) (((uint32_t)le16toh(((uint16_t)(((uint32_t)(x)) >> 16)))) | (((uint32_t)le16toh(((uint16_t)(x)))) << 16))

#           define htobe64(x) (x)
#           define htole64(x) (((uint64_t)htole32(((uint32_t)(((uint64_t)(x)) >> 32)))) | (((uint64_t)htole32(((uint32_t)(x)))) << 32))
#           define be64toh(x) (x)
#           define le64toh(x) (((uint64_t)le32toh(((uint32_t)(((uint64_t)(x)) >> 32)))) | (((uint64_t)le32toh(((uint32_t)(x)))) << 32))
#       else
#           error Platform not supported
#       endif
#   endif
#elif defined(__APPLE__)
#   include <libkern/OSByteOrder.h>

#    define htobe16(x) OSSwapHostToBigInt16(x)
#    define htole16(x) OSSwapHostToLittleInt16(x)
#    define be16toh(x) OSSwapBigToHostInt16(x)
#    define le16toh(x) OSSwapLittleToHostInt16(x)

#    define htobe32(x) OSSwapHostToBigInt32(x)
#    define htole32(x) OSSwapHostToLittleInt32(x)
#    define be32toh(x) OSSwapBigToHostInt32(x)
#    define le32toh(x) OSSwapLittleToHostInt32(x)

#    define htobe64(x) OSSwapHostToBigInt64(x)
#    define htole64(x) OSSwapHostToLittleInt64(x)
#    define be64toh(x) OSSwapBigToHostInt64(x)
#    define le64toh(x) OSSwapLittleToHostInt64(x)

#    define __BYTE_ORDER    BYTE_ORDER
#    define __BIG_ENDIAN    BIG_ENDIAN
#    define __LITTLE_ENDIAN LITTLE_ENDIAN
#   define __PDP_ENDIAN PDP_ENDIAN
#elif defined(__OpenBSD__) ||  defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
#    include <sys/endian.h>
#   ifndef be16toh
#        define be16toh(x) betoh16(x)
#   endif
#   ifndef le16toh
#        define le16toh(x) letoh16(x)
#   endif
#   ifndef be32toh
#        define be32toh(x) betoh32(x)
#   endif
#   ifndef le32toh
#        define le32toh(x) letoh32(x)
#   endif
#   ifndef be64toh
#        define be64toh(x) betoh64(x)
#   endif
#   ifndef le64toh
#        define le64toh(x) letoh64(x)
#   endif
#elif defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
#   include <windows.h>
#    if BYTE_ORDER == LITTLE_ENDIAN
#       if defined(_MSC_VER)
#           include <stdlib.h>
#            define htobe16(x) _byteswap_ushort(x)
#            define htole16(x) (x)
#            define be16toh(x) _byteswap_ushort(x)
#            define le16toh(x) (x)

#            define htobe32(x) _byteswap_ulong(x)
#            define htole32(x) (x)
#            define be32toh(x) _byteswap_ulong(x)
#            define le32toh(x) (x)

#            define htobe64(x) _byteswap_uint64(x)
#            define htole64(x) (x)
#            define be64toh(x) _byteswap_uint64(x)
#            define le64toh(x) (x)
#       elif defined(__GNUC__) || defined(__clang__)
#            define htobe16(x) __builtin_bswap16(x)
#            define htole16(x) (x)
#            define be16toh(x) __builtin_bswap16(x)
#            define le16toh(x) (x)

#            define htobe32(x) __builtin_bswap32(x)
#            define htole32(x) (x)
#            define be32toh(x) __builtin_bswap32(x)
#            define le32toh(x) (x)

#            define htobe64(x) __builtin_bswap64(x)
#            define htole64(x) (x)
#            define be64toh(x) __builtin_bswap64(x)
#            define le64toh(x) (x)
#       else
#           error Platform not supported
#       endif
#   elif BYTE_ORDER == BIG_ENDIAN
#        define htobe16(x) (x)
#        define htole16(x) __builtin_bswap16(x)
#        define be16toh(x) (x)
#        define le16toh(x) __builtin_bswap16(x)

#        define htobe32(x) (x)
#        define htole32(x) __builtin_bswap32(x)
#        define be32toh(x) (x)
#        define le32toh(x) __builtin_bswap32(x)

#        define htobe64(x) (x)
#        define htole64(x) __builtin_bswap64(x)
#        define be64toh(x) (x)
#       define le64toh(x) __builtin_bswap64(x)
#   else
#        error Platform not supported
#   endif
#    define __BYTE_ORDER    BYTE_ORDER
#    define __BIG_ENDIAN    BIG_ENDIAN
#    define __LITTLE_ENDIAN LITTLE_ENDIAN
#    define __PDP_ENDIAN    PDP_ENDIAN
#elif defined(_NEWLIB_VERSION)
#   include <machine/endian.h>
#    if BYTE_ORDER == LITTLE_ENDIAN
#        define htobe16(x) __bswap16(x)
#        define htole16(x) (x)
#        define be16toh(x) __bswap16(x)
#        define le16toh(x) (x)

#        define htobe32(x) __bswap32(x)
#        define htole32(x) (x)
#        define be32toh(x) __bswap32(x)
#        define le32toh(x) (x)

#        define htobe64(x) __bswap64(x)
#        define htole64(x) (x)
#        define be64toh(x) __bswap64(x)
#        define le64toh(x) (x)
#    elif BYTE_ORDER == BIG_ENDIAN
#        define htobe16(x) (x)
#        define htole16(x) __bswap16(x)
#        define be16toh(x) (x)
#        define le16toh(x) __bswap16(x)

#        define htobe32(x) (x)
#        define htole32(x) __bswap32(x)
#        define be32toh(x) (x)
#        define le32toh(x) __bswap32(x)

#        define htobe64(x) (x)
#        define htole64(x) __bswap64(x)
#        define be64toh(x) (x)
#        define le64toh(x) __bswap64(x)
#    else
#        error Platform not supported
#   endif
#else
#   error Platform not supported
#endif

namespace asyncmsg {
namespace detail {

static uint16_t host_to_network_16(uint16_t x) {
    return htobe16(x);
}

static uint16_t network_to_host_16(uint16_t x) {
    return be16toh(x);
}

static uint32_t host_to_network_32(uint32_t x) {
    return htobe32(x);
}

static uint32_t network_to_host_32(uint32_t x) {
    return be32toh(x);
}

static uint64_t host_to_network_64(uint64_t x) {
    return htobe64(x);
}

static uint64_t network_to_host_64(uint64_t x) {
    return be64toh(x);
}

}
}
