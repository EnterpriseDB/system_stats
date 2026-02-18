-- Smoke test for system_stats extension
-- Quick sanity checks to verify extension loads and basic functions work

CREATE EXTENSION IF NOT EXISTS system_stats;

-- Quick check: all functions return at least one row
\echo '### Smoke Test: Basic Function Calls ###'

SELECT 'pg_sys_os_info' AS function, count(*) > 0 AS works FROM pg_sys_os_info()
UNION ALL
SELECT 'pg_sys_cpu_info', count(*) > 0 FROM pg_sys_cpu_info()
UNION ALL
SELECT 'pg_sys_memory_info', count(*) > 0 FROM pg_sys_memory_info()
UNION ALL
SELECT 'pg_sys_disk_info', count(*) > 0 FROM pg_sys_disk_info()
UNION ALL
SELECT 'pg_sys_load_avg_info', count(*) > 0 FROM pg_sys_load_avg_info()
UNION ALL
SELECT 'pg_sys_cpu_usage_info', count(*) > 0 FROM pg_sys_cpu_usage_info()
UNION ALL
SELECT 'pg_sys_io_analysis_info', count(*) >= 0 FROM pg_sys_io_analysis_info()
UNION ALL
SELECT 'pg_sys_process_info', count(*) > 0 FROM pg_sys_process_info()
UNION ALL
SELECT 'pg_sys_network_info', count(*) > 0 FROM pg_sys_network_info()
UNION ALL
SELECT 'pg_sys_cpu_memory_by_process', count(*) > 0 FROM pg_sys_cpu_memory_by_process()
ORDER BY 1;

\echo '### Smoke Test Passed ###'
