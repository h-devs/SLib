if (NOT ANDROID)

set (TARGET_NAME rocksdb)
set (ROOT_DIR "${SLIB_PATH}/external/src/rocksdb")

add_definitions(-DSNAPPY -DZLIB -DLZ4 -DZSTD -DOS_LINUX -DROCKSDB_PLATFORM_POSIX -DROCKSDB_LIB_IO_POSIX)

set (
 SRC_LIST
 "${ROOT_DIR}/cache/cache.cc"
 "${ROOT_DIR}/cache/clock_cache.cc"
 "${ROOT_DIR}/cache/lru_cache.cc"
 "${ROOT_DIR}/cache/sharded_cache.cc"
 "${ROOT_DIR}/db/arena_wrapped_db_iter.cc"
 "${ROOT_DIR}/db/blob/blob_file_addition.cc"
 "${ROOT_DIR}/db/blob/blob_file_builder.cc"
 "${ROOT_DIR}/db/blob/blob_file_cache.cc"
 "${ROOT_DIR}/db/blob/blob_file_garbage.cc"
 "${ROOT_DIR}/db/blob/blob_file_meta.cc"
 "${ROOT_DIR}/db/blob/blob_file_reader.cc"
 "${ROOT_DIR}/db/blob/blob_log_format.cc"
 "${ROOT_DIR}/db/blob/blob_log_sequential_reader.cc"
 "${ROOT_DIR}/db/blob/blob_log_writer.cc"
 "${ROOT_DIR}/db/builder.cc"
 "${ROOT_DIR}/db/c.cc"
 "${ROOT_DIR}/db/column_family.cc"
 "${ROOT_DIR}/db/compacted_db_impl.cc"
 "${ROOT_DIR}/db/compaction/compaction.cc"
 "${ROOT_DIR}/db/compaction/compaction_iterator.cc"
 "${ROOT_DIR}/db/compaction/compaction_job.cc"
 "${ROOT_DIR}/db/compaction/compaction_picker.cc"
 "${ROOT_DIR}/db/compaction/compaction_picker_fifo.cc"
 "${ROOT_DIR}/db/compaction/compaction_picker_level.cc"
 "${ROOT_DIR}/db/compaction/compaction_picker_universal.cc"
 "${ROOT_DIR}/db/compaction/sst_partitioner.cc"
 "${ROOT_DIR}/db/convenience.cc"
 "${ROOT_DIR}/db/db_filesnapshot.cc"
 "${ROOT_DIR}/db/db_impl/db_impl.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_compaction_flush.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_debug.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_experimental.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_files.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_open.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_readonly.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_secondary.cc"
 "${ROOT_DIR}/db/db_impl/db_impl_write.cc"
 "${ROOT_DIR}/db/db_info_dumper.cc"
 "${ROOT_DIR}/db/db_iter.cc"
 "${ROOT_DIR}/db/dbformat.cc"
 "${ROOT_DIR}/db/error_handler.cc"
 "${ROOT_DIR}/db/event_helpers.cc"
 "${ROOT_DIR}/db/experimental.cc"
 "${ROOT_DIR}/db/external_sst_file_ingestion_job.cc"
 "${ROOT_DIR}/db/file_indexer.cc"
 "${ROOT_DIR}/db/flush_job.cc"
 "${ROOT_DIR}/db/flush_scheduler.cc"
 "${ROOT_DIR}/db/forward_iterator.cc"
 "${ROOT_DIR}/db/import_column_family_job.cc"
 "${ROOT_DIR}/db/internal_stats.cc"
 "${ROOT_DIR}/db/logs_with_prep_tracker.cc"
 "${ROOT_DIR}/db/log_reader.cc"
 "${ROOT_DIR}/db/log_writer.cc"
 "${ROOT_DIR}/db/malloc_stats.cc"
 "${ROOT_DIR}/db/memtable.cc"
 "${ROOT_DIR}/db/memtable_list.cc"
 "${ROOT_DIR}/db/merge_helper.cc"
 "${ROOT_DIR}/db/merge_operator.cc"
 "${ROOT_DIR}/db/output_validator.cc"
 "${ROOT_DIR}/db/periodic_work_scheduler.cc"
 "${ROOT_DIR}/db/range_del_aggregator.cc"
 "${ROOT_DIR}/db/range_tombstone_fragmenter.cc"
 "${ROOT_DIR}/db/repair.cc"
 "${ROOT_DIR}/db/snapshot_impl.cc"
 "${ROOT_DIR}/db/table_cache.cc"
 "${ROOT_DIR}/db/table_properties_collector.cc"
 "${ROOT_DIR}/db/transaction_log_impl.cc"
 "${ROOT_DIR}/db/trim_history_scheduler.cc"
 "${ROOT_DIR}/db/version_builder.cc"
 "${ROOT_DIR}/db/version_edit.cc"
 "${ROOT_DIR}/db/version_edit_handler.cc"
 "${ROOT_DIR}/db/version_set.cc"
 "${ROOT_DIR}/db/wal_edit.cc"
 "${ROOT_DIR}/db/wal_manager.cc"
 "${ROOT_DIR}/db/write_batch.cc"
 "${ROOT_DIR}/db/write_batch_base.cc"
 "${ROOT_DIR}/db/write_controller.cc"
 "${ROOT_DIR}/db/write_thread.cc"
 "${ROOT_DIR}/env/env.cc"
 "${ROOT_DIR}/env/env_chroot.cc"
 "${ROOT_DIR}/env/env_encryption.cc"
 "${ROOT_DIR}/env/env_hdfs.cc"
 "${ROOT_DIR}/env/file_system.cc"
 "${ROOT_DIR}/env/file_system_tracer.cc"
 "${ROOT_DIR}/env/mock_env.cc"
 "${ROOT_DIR}/file/delete_scheduler.cc"
 "${ROOT_DIR}/file/file_prefetch_buffer.cc"
 "${ROOT_DIR}/file/file_util.cc"
 "${ROOT_DIR}/file/filename.cc"
 "${ROOT_DIR}/file/random_access_file_reader.cc"
 "${ROOT_DIR}/file/read_write_util.cc"
 "${ROOT_DIR}/file/readahead_raf.cc"
 "${ROOT_DIR}/file/sequence_file_reader.cc"
 "${ROOT_DIR}/file/sst_file_manager_impl.cc"
 "${ROOT_DIR}/file/writable_file_writer.cc"
 "${ROOT_DIR}/logging/auto_roll_logger.cc"
 "${ROOT_DIR}/logging/event_logger.cc"
 "${ROOT_DIR}/logging/log_buffer.cc"
 "${ROOT_DIR}/memory/arena.cc"
 "${ROOT_DIR}/memory/concurrent_arena.cc"
 "${ROOT_DIR}/memory/jemalloc_nodump_allocator.cc"
 "${ROOT_DIR}/memory/memkind_kmem_allocator.cc"
 "${ROOT_DIR}/memtable/alloc_tracker.cc"
 "${ROOT_DIR}/memtable/hash_linklist_rep.cc"
 "${ROOT_DIR}/memtable/hash_skiplist_rep.cc"
 "${ROOT_DIR}/memtable/skiplistrep.cc"
 "${ROOT_DIR}/memtable/vectorrep.cc"
 "${ROOT_DIR}/memtable/write_buffer_manager.cc"
 "${ROOT_DIR}/monitoring/histogram.cc"
 "${ROOT_DIR}/monitoring/histogram_windowing.cc"
 "${ROOT_DIR}/monitoring/in_memory_stats_history.cc"
 "${ROOT_DIR}/monitoring/instrumented_mutex.cc"
 "${ROOT_DIR}/monitoring/iostats_context.cc"
 "${ROOT_DIR}/monitoring/perf_context.cc"
 "${ROOT_DIR}/monitoring/perf_level.cc"
 "${ROOT_DIR}/monitoring/persistent_stats_history.cc"
 "${ROOT_DIR}/monitoring/statistics.cc"
 "${ROOT_DIR}/monitoring/thread_status_impl.cc"
 "${ROOT_DIR}/monitoring/thread_status_updater.cc"
 "${ROOT_DIR}/monitoring/thread_status_util.cc"
 "${ROOT_DIR}/monitoring/thread_status_util_debug.cc"
 "${ROOT_DIR}/options/cf_options.cc"
 "${ROOT_DIR}/options/configurable.cc"
 "${ROOT_DIR}/options/customizable.cc"
 "${ROOT_DIR}/options/db_options.cc"
 "${ROOT_DIR}/options/options.cc"
 "${ROOT_DIR}/options/options_helper.cc"
 "${ROOT_DIR}/options/options_parser.cc"
 "${ROOT_DIR}/port/stack_trace.cc"
 "${ROOT_DIR}/table/adaptive/adaptive_table_factory.cc"
 "${ROOT_DIR}/table/block_based/binary_search_index_reader.cc"
 "${ROOT_DIR}/table/block_based/block.cc"
 "${ROOT_DIR}/table/block_based/block_based_filter_block.cc"
 "${ROOT_DIR}/table/block_based/block_based_table_builder.cc"
 "${ROOT_DIR}/table/block_based/block_based_table_factory.cc"
 "${ROOT_DIR}/table/block_based/block_based_table_iterator.cc"
 "${ROOT_DIR}/table/block_based/block_based_table_reader.cc"
 "${ROOT_DIR}/table/block_based/block_builder.cc"
 "${ROOT_DIR}/table/block_based/block_prefetcher.cc"
 "${ROOT_DIR}/table/block_based/block_prefix_index.cc"
 "${ROOT_DIR}/table/block_based/data_block_hash_index.cc"
 "${ROOT_DIR}/table/block_based/data_block_footer.cc"
 "${ROOT_DIR}/table/block_based/filter_block_reader_common.cc"
 "${ROOT_DIR}/table/block_based/filter_policy.cc"
 "${ROOT_DIR}/table/block_based/flush_block_policy.cc"
 "${ROOT_DIR}/table/block_based/full_filter_block.cc"
 "${ROOT_DIR}/table/block_based/hash_index_reader.cc"
 "${ROOT_DIR}/table/block_based/index_builder.cc"
 "${ROOT_DIR}/table/block_based/index_reader_common.cc"
 "${ROOT_DIR}/table/block_based/parsed_full_filter_block.cc"
 "${ROOT_DIR}/table/block_based/partitioned_filter_block.cc"
 "${ROOT_DIR}/table/block_based/partitioned_index_iterator.cc"
 "${ROOT_DIR}/table/block_based/partitioned_index_reader.cc"
 "${ROOT_DIR}/table/block_based/reader_common.cc"
 "${ROOT_DIR}/table/block_based/uncompression_dict_reader.cc"
 "${ROOT_DIR}/table/block_fetcher.cc"
 "${ROOT_DIR}/table/cuckoo/cuckoo_table_builder.cc"
 "${ROOT_DIR}/table/cuckoo/cuckoo_table_factory.cc"
 "${ROOT_DIR}/table/cuckoo/cuckoo_table_reader.cc"
 "${ROOT_DIR}/table/format.cc"
 "${ROOT_DIR}/table/get_context.cc"
 "${ROOT_DIR}/table/iterator.cc"
 "${ROOT_DIR}/table/merging_iterator.cc"
 "${ROOT_DIR}/table/meta_blocks.cc"
 "${ROOT_DIR}/table/persistent_cache_helper.cc"
 "${ROOT_DIR}/table/plain/plain_table_bloom.cc"
 "${ROOT_DIR}/table/plain/plain_table_builder.cc"
 "${ROOT_DIR}/table/plain/plain_table_factory.cc"
 "${ROOT_DIR}/table/plain/plain_table_index.cc"
 "${ROOT_DIR}/table/plain/plain_table_key_coding.cc"
 "${ROOT_DIR}/table/plain/plain_table_reader.cc"
 "${ROOT_DIR}/table/sst_file_dumper.cc"
 "${ROOT_DIR}/table/sst_file_reader.cc"
 "${ROOT_DIR}/table/sst_file_writer.cc"
 "${ROOT_DIR}/table/table_factory.cc"
 "${ROOT_DIR}/table/table_properties.cc"
 "${ROOT_DIR}/table/two_level_iterator.cc"
 "${ROOT_DIR}/test_util/sync_point.cc"
 "${ROOT_DIR}/test_util/sync_point_impl.cc"
 "${ROOT_DIR}/test_util/transaction_test_util.cc"
 "${ROOT_DIR}/tools/dump/db_dump_tool.cc"
 "${ROOT_DIR}/trace_replay/trace_replay.cc"
 "${ROOT_DIR}/trace_replay/block_cache_tracer.cc"
 "${ROOT_DIR}/trace_replay/io_tracer.cc"
 "${ROOT_DIR}/util/coding.cc"
 "${ROOT_DIR}/util/compaction_job_stats_impl.cc"
 "${ROOT_DIR}/util/comparator.cc"
 "${ROOT_DIR}/util/compression_context_cache.cc"
 "${ROOT_DIR}/util/concurrent_task_limiter_impl.cc"
 "${ROOT_DIR}/util/crc32c.cc"
 "${ROOT_DIR}/util/dynamic_bloom.cc"
 "${ROOT_DIR}/util/hash.cc"
 "${ROOT_DIR}/util/murmurhash.cc"
 "${ROOT_DIR}/util/random.cc"
 "${ROOT_DIR}/util/rate_limiter.cc"
 "${ROOT_DIR}/util/slice.cc"
 "${ROOT_DIR}/util/file_checksum_helper.cc"
 "${ROOT_DIR}/util/status.cc"
 "${ROOT_DIR}/util/string_util.cc"
 "${ROOT_DIR}/util/thread_local.cc"
 "${ROOT_DIR}/util/threadpool_imp.cc"
 "${ROOT_DIR}/util/xxhash.cc"
 "${ROOT_DIR}/utilities/backupable/backupable_db.cc"
 "${ROOT_DIR}/utilities/blob_db/blob_compaction_filter.cc"
 "${ROOT_DIR}/utilities/blob_db/blob_db.cc"
 "${ROOT_DIR}/utilities/blob_db/blob_db_impl.cc"
 "${ROOT_DIR}/utilities/blob_db/blob_db_impl_filesnapshot.cc"
 "${ROOT_DIR}/utilities/blob_db/blob_file.cc"
 "${ROOT_DIR}/utilities/cassandra/cassandra_compaction_filter.cc"
 "${ROOT_DIR}/utilities/cassandra/cassandra_format.cc"
 "${ROOT_DIR}/utilities/cassandra/cassandra_merge_operator.cc"
 "${ROOT_DIR}/utilities/checkpoint/checkpoint_impl.cc"
 "${ROOT_DIR}/utilities/compaction_filters/remove_emptyvalue_compactionfilter.cc"
 "${ROOT_DIR}/utilities/debug.cc"
 "${ROOT_DIR}/utilities/env_mirror.cc"
 "${ROOT_DIR}/utilities/env_timed.cc"
 "${ROOT_DIR}/utilities/fault_injection_env.cc"
 "${ROOT_DIR}/utilities/fault_injection_fs.cc"
 "${ROOT_DIR}/utilities/leveldb_options/leveldb_options.cc"
 "${ROOT_DIR}/utilities/memory/memory_util.cc"
 "${ROOT_DIR}/utilities/merge_operators/max.cc"
 "${ROOT_DIR}/utilities/merge_operators/put.cc"
 "${ROOT_DIR}/utilities/merge_operators/sortlist.cc"
 "${ROOT_DIR}/utilities/merge_operators/string_append/stringappend.cc"
 "${ROOT_DIR}/utilities/merge_operators/string_append/stringappend2.cc"
 "${ROOT_DIR}/utilities/merge_operators/uint64add.cc"
 "${ROOT_DIR}/utilities/merge_operators/bytesxor.cc"
 "${ROOT_DIR}/utilities/object_registry.cc"
 "${ROOT_DIR}/utilities/option_change_migration/option_change_migration.cc"
 "${ROOT_DIR}/utilities/options/options_util.cc"
 "${ROOT_DIR}/utilities/persistent_cache/block_cache_tier.cc"
 "${ROOT_DIR}/utilities/persistent_cache/block_cache_tier_file.cc"
 "${ROOT_DIR}/utilities/persistent_cache/block_cache_tier_metadata.cc"
 "${ROOT_DIR}/utilities/persistent_cache/persistent_cache_tier.cc"
 "${ROOT_DIR}/utilities/persistent_cache/volatile_tier_impl.cc"
 "${ROOT_DIR}/utilities/simulator_cache/cache_simulator.cc"
 "${ROOT_DIR}/utilities/simulator_cache/sim_cache.cc"
 "${ROOT_DIR}/utilities/table_properties_collectors/compact_on_deletion_collector.cc"
 "${ROOT_DIR}/utilities/trace/file_trace_reader_writer.cc"
 "${ROOT_DIR}/utilities/transactions/lock/lock_manager.cc"
 "${ROOT_DIR}/utilities/transactions/lock/point/point_lock_tracker.cc"
 "${ROOT_DIR}/utilities/transactions/lock/point/point_lock_manager.cc"
 "${ROOT_DIR}/utilities/transactions/optimistic_transaction.cc"
 "${ROOT_DIR}/utilities/transactions/optimistic_transaction_db_impl.cc"
 "${ROOT_DIR}/utilities/transactions/pessimistic_transaction.cc"
 "${ROOT_DIR}/utilities/transactions/pessimistic_transaction_db.cc"
 "${ROOT_DIR}/utilities/transactions/snapshot_checker.cc"
 "${ROOT_DIR}/utilities/transactions/transaction_base.cc"
 "${ROOT_DIR}/utilities/transactions/transaction_db_mutex_impl.cc"
 "${ROOT_DIR}/utilities/transactions/transaction_util.cc"
 "${ROOT_DIR}/utilities/transactions/write_prepared_txn.cc"
 "${ROOT_DIR}/utilities/transactions/write_prepared_txn_db.cc"
 "${ROOT_DIR}/utilities/transactions/write_unprepared_txn.cc"
 "${ROOT_DIR}/utilities/transactions/write_unprepared_txn_db.cc"
 "${ROOT_DIR}/utilities/ttl/db_ttl_impl.cc"
 "${ROOT_DIR}/utilities/write_batch_with_index/write_batch_with_index.cc"
 "${ROOT_DIR}/utilities/write_batch_with_index/write_batch_with_index_internal.cc"
 "${ROOT_DIR}/env/env_posix.cc"
 "${ROOT_DIR}/env/fs_posix.cc"
 "${ROOT_DIR}/env/io_posix.cc"
 "${ROOT_DIR}/port/port_posix.cc"
)

include ("${CMAKE_CURRENT_LIST_DIR}/common.inc.cmake")

target_include_directories (
 ${TARGET_NAME}
 PRIVATE "${ROOT_DIR}"
 PRIVATE "${ROOT_DIR}/../../include"
)

target_compile_options (
 ${TARGET_NAME}
 PRIVATE -Os -g0
)

endif ()
