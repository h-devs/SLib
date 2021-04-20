#define HAVE_SNAPPY 1

#ifdef WIN32
#pragma warning (disable: 4996)
#pragma warning (disable: 4267)
#pragma warning (disable: 4244)
#define LEVELDB_PLATFORM_WINDOWS
#else
#define LEVELDB_PLATFORM_POSIX
#endif

#include "db/builder.cc"
#include "db/c.cc"
#include "db/db_impl.cc"
#include "db/db_iter.cc"
#include "db/dbformat.cc"
#include "db/dumpfile.cc"
#include "db/filename.cc"
#include "db/log_reader.cc"
#include "db/log_writer.cc"
#include "db/memtable.cc"
#include "db/repair.cc"
#include "db/table_cache.cc"
#include "db/version_edit.cc"
#include "db/version_set.cc"
#include "db/write_batch.cc"
#include "table/block_builder.cc"
#include "table/block.cc"
#include "table/filter_block.cc"
#include "table/format.cc"
#include "table/iterator.cc"
#include "table/merger.cc"
#include "table/table_builder.cc"
#include "table/table.cc"
#include "table/two_level_iterator.cc"
#include "util/arena.cc"
#include "util/bloom.cc"
#include "util/cache.cc"
#include "util/coding.cc"
#include "util/comparator.cc"
#include "util/crc32c.cc"
#include "util/env.cc"
#include "util/filter_policy.cc"
#include "util/hash.cc"
#include "util/logging.cc"
#include "util/options.cc"
#include "util/status.cc"

#ifdef WIN32
#include "util/env_windows.cc"
#else
#include "util/env_posix.cc"
#endif