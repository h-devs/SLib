#ifdef _MSC_VER
#pragma warning (disable: 4996)
#pragma warning (disable: 4267)
#endif

#include "trees.c"
#include "adler32.c"
#include "compress.c"
#undef DO1
#undef DO8
#include "crc32.c"
#include "deflate.c"
#undef GZIP
#include "gzclose.c"
#include "gzlib.c"
#include "gzread.c"
#include "gzwrite.c"
#undef COPY
#include "infback.c"
#include "inffast.c"
#define fixedtables inflate_fixedtables
#undef PULLBYTE
#include "inflate.c"
#include "inftrees.c"
#include "uncompr.c"
#include "zutil.c"

#include "contrib/minizip/ioapi.c"
#include "contrib/minizip/unzip.c"
#include "contrib/minizip/zip.c"
