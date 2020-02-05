/* system statistics extension */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION system_stats" to load this file. \quit

/* ***********************************************
 * CPU information function
 * *********************************************** */
CREATE FUNCTION pg_sys_cpu_info(
	OUT architecture text,
	OUT cpu_op_mode text,
	OUT byte_order text,
	OUT no_of_cpu int,
	OUT threads_per_core int,
	OUT core_per_socket int,
	OUT no_of_sockets int,
	OUT vendor_id text,
	OUT cpu_family text,
	OUT model text,
	OUT model_name text,
	OUT cpu_mhz text,
	OUT cpu_l1d_cache_size text,
	OUT cpu_l1i_cache_size text,
	OUT cpu_l2_cache_size text,
	OUT cpu_l3_cache_size text,
	OUT cpu_l4_cache_size text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_cpu_info() FROM PUBLIC;

/* ***********************************************
 * CPU usage information function
 * *********************************************** */
-- This function will fetch the time spent in miliseconds by CPU in each mode
-- as described by arguments
CREATE FUNCTION pg_sys_cpu_usage_info(
	OUT cpu_name text,
	OUT usermode_normal_process int8,
	OUT usermode_niced_process int8,
	OUT kernelmode_process int8,
	OUT idle_mode int8,
	OUT IO_completion int8,
	OUT servicing_irq int8,
	OUT servicing_softirq int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_cpu_usage_info() FROM PUBLIC;

/* ***********************************************
 * Memory information function
 * *********************************************** */
CREATE FUNCTION pg_sys_memory_info(
	OUT total_memory int8,
	OUT free_memory int8,
        OUT available_memory int8,
        OUT buffers int8,
        OUT cached int8,
        OUT swap_cached int8,
        OUT swap_total int8,
        OUT swap_free int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_memory_info() FROM PUBLIC;

/* ***********************************************
 * IO analysis information function
 * *********************************************** */
CREATE FUNCTION pg_sys_io_analysis_info(
	OUT major_no int8,
	OUT minor_no int8,
	OUT device_name text,
	OUT read_completed int8,
        OUT read_merged int8,
        OUT sector_read int8,
        OUT time_spent_reading_ms int8,
        OUT write_completed int8,
        OUT write_merged int8,
        OUT sector_written int8,
        OUT time_spent_writing_ms int8,
        OUT io_in_progress int8,
        OUT time_spent_io_ms int8,
        OUT weighted_time_spent_io_ms int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_io_analysis_info() FROM PUBLIC;

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

/* ***********************************************
 * Load average information function
 * *********************************************** */
CREATE FUNCTION pg_sys_load_avg_info(
	OUT load_avg_one_minute float4,
        OUT load_avg_five_minutes float4,
        OUT load_avg_ten_minutes float4
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_load_avg_info() FROM PUBLIC;

/* ***********************************************
 * Operating system information function
 * *********************************************** */
CREATE FUNCTION pg_sys_os_info(
	OUT host_name text,
	OUT domain_name text,
        OUT kernel_version text,
        OUT architecture text,
        OUT os_distribution_id text,
        OUT os_description text,
        OUT os_release_version text,
        OUT os_code_name text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_os_info() FROM PUBLIC;

/* ***********************************************
 * Process information function
 * *********************************************** */
CREATE FUNCTION pg_sys_process_info(
	OUT active_processes int8,
        OUT running_processes int8,
        OUT sleeping_processes int8,
        OUT stopped_processes int8,
        OUT zombie_processes int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_process_info() FROM PUBLIC;

/* ***********************************************
 * Network information function
 * *********************************************** */
CREATE FUNCTION pg_sys_network_info(
	OUT interface_name text,
        OUT ipv4_address text,
        OUT ipv6_address text,
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

/* *************************************************
 * CPU and memory information by process id or name
 * ************************************************* */
CREATE FUNCTION pg_sys_cpu_memory_by_process(
	IN process_id_or_name text,
	OUT pid int,
	OUT user_name text,
	OUT cpu_usage float4,
        OUT memory_usage float4,
        OUT command text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C;
REVOKE ALL ON FUNCTION pg_sys_cpu_memory_by_process(text) FROM PUBLIC;
