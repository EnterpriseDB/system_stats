/* system statistics extension */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION system_stats" to load this file. \quit

-- role to be assigned while executing functions of system stats
CREATE ROLE monitor_system_stats WITH
	NOLOGIN
	NOSUPERUSER
	NOCREATEDB
	NOCREATEROLE
	INHERIT
	NOREPLICATION
	CONNECTION LIMIT -1;

/* ***********************************************
 * CPU information function
 * *********************************************** */
CREATE FUNCTION pg_sys_cpu_info(
	OUT active_cpu int,
	OUT byte_order int,
	OUT cpu_family int,
	OUT cpu_type int,
	OUT logical_cpu int,
	OUT physical_cpu int,
	OUT cpu_freq_mhz int8,
	OUT cpu_model text,
	OUT cpu_machine text,
	OUT cpu_l1dcache_size int8,
	OUT cpu_l1icache_size int8,
	OUT cpu_l2cache_size int8,
	OUT cpu_l3cache_size int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_cpu_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_cpu_info() TO monitor_system_stats;

/* ***********************************************
 * CPU usage information function
 * *********************************************** */
-- This function will fetch the CPU percentage in each mode
-- as described by arguments
CREATE FUNCTION pg_sys_cpu_usage_info(
	OUT usermode_normal_process float4,
	OUT usermode_niced_process float4,
	OUT kernelmode_process float4,
	OUT idle_mode float4
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_cpu_usage_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_cpu_usage_info() TO monitor_system_stats;

/* ***********************************************
 * Memory information function
 * *********************************************** */
CREATE FUNCTION pg_sys_memory_info(
	OUT total_memory int8,
	OUT used_memory int8,
	OUT free_memory int8,
	OUT swap_total int8,
	OUT swap_used int8,
	OUT swap_free int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_memory_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_memory_info() TO monitor_system_stats;

/* ***********************************************
 * IO analysis information function
 * *********************************************** */
CREATE FUNCTION pg_sys_io_analysis_info(
	OUT device_name text,
	OUT total_reads int8,
	OUT total_writes int8,
	OUT read_bytes int8,
	OUT write_bytes int8,
	OUT read_time_ms int8,
	OUT write_time_ms int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_io_analysis_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_io_analysis_info() TO monitor_system_stats;

/* ***********************************************
 * Disk information function
 * *********************************************** */
CREATE FUNCTION pg_sys_disk_info(
	OUT file_system text,
	OUT file_system_type text,
	OUT mount_point text,
	OUT total_space int8,
	OUT used_space int8,
	OUT available_space int8,
	OUT reserved_space int8,
	OUT total_inodes int8,
	OUT used_inodes int8,
	OUT free_inodes int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_disk_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_disk_info() TO monitor_system_stats;

/* ***********************************************
 * Load average information function
 * *********************************************** */
CREATE FUNCTION pg_sys_load_avg_info(
	OUT load_avg_one_minute float4,
	OUT load_avg_five_minutes float4,
	OUT load_avg_fifteen_minutes float4
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_load_avg_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_load_avg_info() TO monitor_system_stats;

/* ***********************************************
 * Operating system information function
 * *********************************************** */
CREATE FUNCTION pg_sys_os_info(
	OUT host_name text,
	OUT domain_name text,
	OUT os_name text,
	OUT os_release_level text,
	OUT os_version_level text,
	OUT os_architecture text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_os_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_os_info() TO monitor_system_stats;

/* ***********************************************
 * Process information function
 * *********************************************** */
CREATE FUNCTION pg_sys_process_info(
	OUT total_processes int8,
	OUT running_processes int8,
	OUT sleeping_processes int8,
	OUT stopped_processes int8,
	OUT zombie_processes int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_process_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_process_info() TO monitor_system_stats;

/* ***********************************************
 * Network information function
 * *********************************************** */
CREATE FUNCTION pg_sys_network_info(
	OUT interface_name text,
	OUT ipv4_address text,
	OUT speed_mbps int8,
	OUT tx_bytes int8,
	OUT tx_packets int8,
	OUT tx_errors int8,
	OUT tx_dropped int8,
	OUT rx_bytes int8,
	OUT rx_packets int8,
	OUT rx_errors int8,
	OUT rx_dropped int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_network_info() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_network_info() TO monitor_system_stats;

/* *************************************************
 * CPU and memory information by process id or name
 * ************************************************* */
CREATE FUNCTION pg_sys_cpu_memory_by_process(
	OUT pid int,
	OUT command text,
	OUT cpu_usage float4,
	OUT memory_usage float4
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;

REVOKE ALL ON FUNCTION pg_sys_cpu_memory_by_process() FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_sys_cpu_memory_by_process() TO monitor_system_stats;
