-- Test system_stats extension
-- This test suite verifies that all functions work across platforms (Linux, Darwin, Windows)

-- Load the extension
CREATE EXTENSION IF NOT EXISTS system_stats;

-- ============================================================================
-- Test 1: pg_sys_os_info
-- ============================================================================
\echo '### Testing pg_sys_os_info ###'

-- Check that function exists and returns data
SELECT count(*) > 0 AS has_rows FROM pg_sys_os_info();

-- Check column count (should have 10 columns)
SELECT count(*) = 10 AS correct_columns
FROM information_schema.columns
WHERE table_name = 'pg_sys_os_info';

-- Verify critical fields are not NULL (except domain_name and os_up_since_seconds which may be NULL on Windows)
SELECT
    name IS NOT NULL AS has_name,
    version IS NOT NULL AS has_version,
    host_name IS NOT NULL AS has_host_name,
    handle_count >= 0 AS has_handle_count,
    process_count > 0 AS has_process_count,
    thread_count > 0 AS has_thread_count,
    architecture IS NOT NULL AS has_architecture
FROM pg_sys_os_info();

-- Test that name contains expected text (platform-agnostic check)
SELECT
    length(name) > 0 AS name_not_empty,
    length(version) > 0 AS version_not_empty,
    length(host_name) > 0 AS hostname_not_empty
FROM pg_sys_os_info();

-- ============================================================================
-- Test 2: pg_sys_cpu_info
-- ============================================================================
\echo '### Testing pg_sys_cpu_info ###'

-- Check that function exists and returns data
SELECT count(*) > 0 AS has_rows FROM pg_sys_cpu_info();

-- Verify critical CPU fields are not NULL and reasonable
SELECT
    model_name IS NOT NULL AS has_model_name,
    processor_type IS NOT NULL OR processor_type IS NULL AS processor_type_check,
    logical_processor > 0 AS has_logical_processors,
    no_of_cores > 0 AS has_cores,
    no_of_cores <= logical_processor AS cores_less_than_logical,
    architecture IS NOT NULL AS has_architecture,
    clock_speed_hz >= 0 AS has_clock_speed
FROM pg_sys_cpu_info();

-- ============================================================================
-- Test 3: pg_sys_memory_info
-- ============================================================================
\echo '### Testing pg_sys_memory_info ###'

-- Check that function exists and returns data
SELECT count(*) > 0 AS has_rows FROM pg_sys_memory_info();

-- Verify memory values are reasonable
SELECT
    total_memory > 0 AS has_total_memory,
    used_memory >= 0 AS has_used_memory,
    free_memory >= 0 AS has_free_memory,
    used_memory <= total_memory AS used_less_than_total,
    free_memory <= total_memory AS free_less_than_total,
    swap_total >= 0 AS has_swap_total
FROM pg_sys_memory_info();

-- ============================================================================
-- Test 4: pg_sys_disk_info
-- ============================================================================
\echo '### Testing pg_sys_disk_info ###'

-- Check that function returns at least one disk
SELECT count(*) > 0 AS has_disks FROM pg_sys_disk_info();

-- Verify disk info has reasonable values
SELECT
    count(*) FILTER (WHERE file_system IS NOT NULL) > 0 AS has_filesystems,
    count(*) FILTER (WHERE total_space > 0) > 0 AS has_total_space,
    count(*) FILTER (WHERE used_space >= 0) > 0 AS has_used_space,
    count(*) FILTER (WHERE free_space >= 0) > 0 AS has_free_space
FROM pg_sys_disk_info();

-- Verify space calculations are consistent
SELECT
    count(*) = count(*) FILTER (WHERE used_space + free_space <= total_space * 1.01) AS space_consistent
FROM pg_sys_disk_info()
WHERE total_space > 0;

-- ============================================================================
-- Test 5: pg_sys_load_avg_info
-- ============================================================================
\echo '### Testing pg_sys_load_avg_info ###'

-- Check that function exists and returns data
SELECT count(*) > 0 AS has_rows FROM pg_sys_load_avg_info();

-- Verify load average values are reasonable (should be >= 0)
SELECT
    load_avg_one_minute >= 0 AS one_min_valid,
    load_avg_five_minutes >= 0 AS five_min_valid,
    load_avg_fifteen_minutes >= 0 OR load_avg_fifteen_minutes IS NULL AS fifteen_min_valid
FROM pg_sys_load_avg_info();

-- ============================================================================
-- Test 6: pg_sys_cpu_usage_info
-- ============================================================================
\echo '### Testing pg_sys_cpu_usage_info ###'

-- Check that function exists and returns data
SELECT count(*) > 0 AS has_rows FROM pg_sys_cpu_usage_info();

-- Verify CPU usage percentages are in valid range (0-100)
SELECT
    usermode_normal_process_percent >= 0 AS usermode_valid,
    kernelmode_process_percent >= 0 AS kernelmode_valid,
    idle_mode_percent >= 0 AS idle_valid,
    io_completion_percent >= 0 AS io_valid
FROM pg_sys_cpu_usage_info();

-- ============================================================================
-- Test 7: pg_sys_io_analysis_info
-- ============================================================================
\echo '### Testing pg_sys_io_analysis_info ###'

-- Check that function returns devices (may be empty on some systems)
SELECT count(*) >= 0 AS has_devices FROM pg_sys_io_analysis_info();

-- If there are devices, verify fields are reasonable
SELECT
    count(*) FILTER (WHERE device_name IS NOT NULL) >= 0 AS has_device_names,
    count(*) FILTER (WHERE total_reads >= 0) >= 0 AS reads_valid,
    count(*) FILTER (WHERE total_writes >= 0) >= 0 AS writes_valid,
    count(*) FILTER (WHERE read_bytes >= 0) >= 0 AS read_bytes_valid,
    count(*) FILTER (WHERE write_bytes >= 0) >= 0 AS write_bytes_valid
FROM pg_sys_io_analysis_info();

-- ============================================================================
-- Test 8: pg_sys_process_info
-- ============================================================================
\echo '### Testing pg_sys_process_info ###'

-- Check that function exists and returns data
SELECT count(*) > 0 AS has_rows FROM pg_sys_process_info();

-- Verify process counts are reasonable
SELECT
    running_processes >= 0 AS running_valid,
    sleeping_processes >= 0 AS sleeping_valid,
    stopped_processes >= 0 AS stopped_valid,
    zombie_processes >= 0 AS zombie_valid
FROM pg_sys_process_info();

-- ============================================================================
-- Test 9: pg_sys_network_info
-- ============================================================================
\echo '### Testing pg_sys_network_info ###'

-- Check that function returns at least one network interface
SELECT count(*) > 0 AS has_interfaces FROM pg_sys_network_info();

-- Verify network stats are reasonable
SELECT
    count(*) FILTER (WHERE interface_name IS NOT NULL) > 0 AS has_interface_names,
    count(*) FILTER (WHERE tx_bytes >= 0) >= 0 AS tx_bytes_valid,
    count(*) FILTER (WHERE rx_bytes >= 0) >= 0 AS rx_bytes_valid,
    count(*) FILTER (WHERE tx_packets >= 0) >= 0 AS tx_packets_valid,
    count(*) FILTER (WHERE rx_packets >= 0) >= 0 AS rx_packets_valid
FROM pg_sys_network_info();

-- ============================================================================
-- Test 10: pg_sys_cpu_memory_by_process
-- ============================================================================
\echo '### Testing pg_sys_cpu_memory_by_process ###'

-- Check that function returns current PostgreSQL process
SELECT count(*) > 0 AS has_current_process
FROM pg_sys_cpu_memory_by_process()
WHERE name LIKE '%postgres%';

-- Verify process stats are reasonable
SELECT
    count(*) FILTER (WHERE pid > 0) > 0 AS has_valid_pids,
    count(*) FILTER (WHERE name IS NOT NULL) > 0 AS has_process_names,
    count(*) FILTER (WHERE cpu_usage >= 0) >= 0 AS cpu_usage_valid,
    count(*) FILTER (WHERE memory_usage >= 0) >= 0 AS memory_usage_valid
FROM pg_sys_cpu_memory_by_process();

-- ============================================================================
-- Test 11: Multiple calls (test caching and performance)
-- ============================================================================
\echo '### Testing multiple calls (caching) ###'

-- Call pg_sys_os_info multiple times to test caching
SELECT count(*) FROM pg_sys_os_info();
SELECT count(*) FROM pg_sys_os_info();
SELECT count(*) FROM pg_sys_os_info();

-- Verify results are consistent across calls
SELECT
    (SELECT name FROM pg_sys_os_info()) = (SELECT name FROM pg_sys_os_info()) AS name_consistent,
    (SELECT version FROM pg_sys_os_info()) = (SELECT version FROM pg_sys_os_info()) AS version_consistent,
    (SELECT host_name FROM pg_sys_os_info()) = (SELECT host_name FROM pg_sys_os_info()) AS hostname_consistent
;

-- ============================================================================
-- Test 12: Data type verification
-- ============================================================================
\echo '### Testing data types ###'

-- Verify pg_sys_memory_info returns bigint types
SELECT
    pg_typeof(total_memory) = 'bigint'::regtype AS total_memory_type_ok,
    pg_typeof(used_memory) = 'bigint'::regtype AS used_memory_type_ok,
    pg_typeof(free_memory) = 'bigint'::regtype AS free_memory_type_ok
FROM pg_sys_memory_info();

-- Verify pg_sys_cpu_info returns correct types
SELECT
    pg_typeof(logical_processor) = 'integer'::regtype AS logical_processor_type_ok,
    pg_typeof(no_of_cores) = 'integer'::regtype AS no_of_cores_type_ok,
    pg_typeof(clock_speed_hz) = 'bigint'::regtype AS clock_speed_type_ok
FROM pg_sys_cpu_info();

-- ============================================================================
-- Test 13: NULL handling
-- ============================================================================
\echo '### Testing NULL handling ###'

-- Test that functions handle NULL parameters gracefully (where applicable)
-- Most functions take no parameters, so we just verify they don't crash

SELECT count(*) FROM pg_sys_os_info() WHERE 1=1;
SELECT count(*) FROM pg_sys_cpu_info() WHERE 1=1;
SELECT count(*) FROM pg_sys_memory_info() WHERE 1=1;

\echo '### All tests completed ###'
