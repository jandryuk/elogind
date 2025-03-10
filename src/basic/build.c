/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "build.h"

const char* const elogind_features =

        /* PAM and MAC frameworks */

#if HAVE_PAM
        "+PAM"
#else
        "-PAM"
#endif

#if HAVE_AUDIT
        " +AUDIT"
#else
        " -AUDIT"
#endif

#if HAVE_SELINUX
        " +SELINUX"
#else
        " -SELINUX"
#endif

#if HAVE_APPARMOR
        " +APPARMOR"
#else
        " -APPARMOR"
#endif

#if 0 /// UNNEEDED by elogind
#if ENABLE_IMA
        " +IMA"
#else
        " -IMA"
#endif
#endif // 0

#if ENABLE_SMACK
        " +SMACK"
#else
        " -SMACK"
#endif

#if HAVE_SECCOMP
        " +SECCOMP"
#else
        " -SECCOMP"
#endif

        /* crypto libraries */

#if HAVE_GCRYPT
        " +GCRYPT"
#else
        " -GCRYPT"
#endif

#if 0 /// UNNEEDED by elogind
#if HAVE_GNUTLS
        " +GNUTLS"
#else
        " -GNUTLS"
#endif

#if HAVE_OPENSSL
        " +OPENSSL"
#else
        " -OPENSSL"
#endif
#endif // 0
        /* all other libraries, sorted alphabetically */

#if HAVE_ACL
        " +ACL"
#else
        " -ACL"
#endif

#if HAVE_BLKID
        " +BLKID"
#else
        " -BLKID"
#endif

#if 0 /// UNNEEDED by elogind
#if HAVE_LIBCURL
        " +CURL"
#else
        " -CURL"
#endif

#if HAVE_ELFUTILS
        " +ELFUTILS"
#else
        " -ELFUTILS"
#endif

#if HAVE_LIBFIDO2
        " +FIDO2"
#else
        " -FIDO2"
#endif

#if HAVE_LIBIDN2
        " +IDN2"
#else
        " -IDN2"
#endif

#if HAVE_LIBIDN
        " +IDN"
#else
        " -IDN"
#endif

#if HAVE_LIBIPTC
        " +IPTC"
#else
        " -IPTC"
#endif
#endif // 0

#if HAVE_KMOD
        " +KMOD"
#else
        " -KMOD"
#endif

#if 0 /// UNNEEDED by elogind
#if HAVE_LIBCRYPTSETUP
        " +LIBCRYPTSETUP"
#else
        " -LIBCRYPTSETUP"
#endif

#if HAVE_LIBFDISK
        " +LIBFDISK"
#else
        " -LIBFDISK"
#endif

#if HAVE_PCRE2
        " +PCRE2"
#else
        " -PCRE2"
#endif

#if HAVE_PWQUALITY
        " +PWQUALITY"
#else
        " -PWQUALITY"
#endif

#if HAVE_P11KIT
        " +P11KIT"
#else
        " -P11KIT"
#endif

#if HAVE_QRENCODE
        " +QRENCODE"
#else
        " -QRENCODE"
#endif

        /* compressors */

#if HAVE_BZIP2
        " +BZIP2"
#else
        " -BZIP2"
#endif

#if HAVE_LZ4
        " +LZ4"
#else
        " -LZ4"
#endif

#if HAVE_XZ
        " +XZ"
#else
        " -XZ"
#endif

#if HAVE_ZLIB
        " +ZLIB"
#else
        " -ZLIB"
#endif

#if HAVE_ZSTD
        " +ZSTD"
#else
        " -ZSTD"
#endif

        /* other stuff that doesn't fit above */

#if HAVE_XKBCOMMON
        " +XKBCOMMON"
#else
        " -XKBCOMMON"
#endif
#endif // 0

#if ENABLE_UTMP
        " +UTMP"
#else
        " -UTMP"
#endif

#if 0 /// UNNEEDED by elogind
#if HAVE_SYSV_COMPAT
        " +SYSVINIT"
#else
        " -SYSVINIT"
#endif
#endif // 0

        " default-hierarchy=" DEFAULT_HIERARCHY_NAME
        ;
