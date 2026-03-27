-- ============================================================================
-- Upgrade path test: system_stats 3.0 -> 4.0
-- ============================================================================
\echo '### Testing upgrade path 3.0 -> 4.0 ###'

-- Clean slate
DROP EXTENSION IF EXISTS system_stats CASCADE;

-- Install version 3.0
CREATE EXTENSION system_stats VERSION '3.0';

-- Verify old function has 6 output columns
SELECT array_length(proargnames, 1) = 6 AS v3_has_6_columns
FROM pg_proc WHERE proname = 'pg_sys_cpu_memory_by_process';

-- Verify current version
SELECT extversion = '3.0' AS is_version_3
FROM pg_extension WHERE extname = 'system_stats';

-- Upgrade to 4.0
ALTER EXTENSION system_stats UPDATE TO '4.0';

-- Verify new version
SELECT extversion = '4.0' AS is_version_4
FROM pg_extension WHERE extname = 'system_stats';

-- Verify function now has 10 output columns
SELECT array_length(proargnames, 1) = 10 AS v4_has_10_columns
FROM pg_proc WHERE proname = 'pg_sys_cpu_memory_by_process';

-- Verify new columns exist with correct types
SELECT
    pg_typeof(virtual_memory_bytes) = 'bigint'::regtype AS virtual_memory_type_ok,
    pg_typeof(swap_usage_bytes) = 'bigint'::regtype AS swap_type_ok,
    pg_typeof(io_read_bytes) = 'bigint'::regtype AS io_read_type_ok,
    pg_typeof(io_write_bytes) = 'bigint'::regtype AS io_write_type_ok
FROM pg_sys_cpu_memory_by_process()
LIMIT 1;

-- Verify first 6 columns preserved (backward-compatible order)
SELECT proargnames[1:6] = ARRAY['pid','name','running_since_seconds',
    'cpu_usage','memory_usage','memory_bytes'] AS first_6_columns_match
FROM pg_proc WHERE proname = 'pg_sys_cpu_memory_by_process';

-- Clean up
DROP EXTENSION system_stats;
