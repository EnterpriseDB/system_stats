-- Upgrade from 3.0 to 4.0
-- Adds virtual_memory_bytes, swap_usage_bytes, io_read_bytes, io_write_bytes
-- to pg_sys_cpu_memory_by_process
--
-- NOTE: This takes an AccessExclusiveLock on the function.
-- Run during a maintenance window if the function is actively queried.
-- Any views or materialized views that depend on the old function
-- signature must be dropped before running this upgrade.

DROP FUNCTION IF EXISTS pg_sys_cpu_memory_by_process();

CREATE FUNCTION pg_sys_cpu_memory_by_process(
    OUT pid int,
    OUT name text,
    OUT running_since_seconds int8,
    OUT cpu_usage float4,
    OUT memory_usage float4,
    OUT memory_bytes int8,
    OUT virtual_memory_bytes int8,
    OUT swap_usage_bytes int8,
    OUT io_read_bytes int8,
    OUT io_write_bytes int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_cpu_memory_by_process() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_cpu_memory_by_process() TO monitor_system_stats;
