/* system statistics extension */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION system_stats" to load this file. \quit


DO $$
DECLARE
  os_platform text;
BEGIN
    -- First find the operating system and according to install the functions
    SELECT CASE
            WHEN version.platform ILIKE '%w64%' THEN 'windows'
            WHEN version.platform ILIKE '%w32%' THEN 'windows'
            WHEN version.platform ILIKE '%mingw%' THEN 'windows'
            WHEN version.platform ILIKE '%visual%' THEN 'windows'
            WHEN version.platform ILIKE '%linux%' THEN 'linux'
            WHEN version.platform ILIKE '%mac%' THEN 'mac'
            WHEN version.platform ILIKE '%darwin%' THEN 'mac'
            ELSE
            'UNKNOWN'
         END INTO os_platform
    FROM (SELECT version() as platform)
    as version;

    IF os_platform IS NULL OR os_platform = '' OR os_platform = 'UNKNOWN' THEN
        RETURN;
    END IF;

    -- role to be assigned while executing functions of system stats
    CREATE ROLE monitor_system_stats WITH
	    NOLOGIN
    	NOSUPERUSER
    	NOCREATEDB
    	NOCREATEROLE
    	INHERIT
    	NOREPLICATION
    	CONNECTION LIMIT -1;

    -- If platform is linux, installed its respective functions
    IF os_platform = 'linux' THEN

        -- CPU information function
        CREATE FUNCTION pg_sys_cpu_info(
        	OUT processor int,
        	OUT vendor_id text,
        	OUT cpu_family text,
        	OUT model text,
        	OUT model_name text,
        	OUT cpu_mhz text,
        	OUT cpu_cache_size text
        )
        RETURNS SETOF record
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

        REVOKE ALL ON FUNCTION pg_sys_cpu_info() FROM PUBLIC;
        GRANT EXECUTE ON FUNCTION pg_sys_cpu_info() TO monitor_system_stats;

        -- CPU usage information function
        -- This function will fetch the time spent in milliseconds by CPU in each mode
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
        GRANT EXECUTE ON FUNCTION pg_sys_cpu_usage_info() TO monitor_system_stats;

        --Memory information function
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
        GRANT EXECUTE ON FUNCTION pg_sys_memory_info() TO monitor_system_stats;

        -- IO analysis information function
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
        GRANT EXECUTE ON FUNCTION pg_sys_io_analysis_info() TO monitor_system_stats;

        -- Disk information function
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

        -- Load average information function
        CREATE FUNCTION pg_sys_load_avg_info(
        	OUT load_avg_one_minute float4,
        	OUT load_avg_five_minutes float4,
        	OUT load_avg_ten_minutes float4
        )
        RETURNS SETOF record
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

        REVOKE ALL ON FUNCTION pg_sys_load_avg_info() FROM PUBLIC;
        GRANT EXECUTE ON FUNCTION pg_sys_load_avg_info() TO monitor_system_stats;

        -- Operating system information function
        CREATE FUNCTION pg_sys_os_info(
        	OUT host_name text,
        	OUT domain_name text,
        	OUT kernel_version text,
        	OUT architecture text,
        	OUT os_description text,
        	OUT os_release_version text,
        	OUT os_code_name text
        )
        RETURNS SETOF record
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

        REVOKE ALL ON FUNCTION pg_sys_os_info() FROM PUBLIC;
        GRANT EXECUTE ON FUNCTION pg_sys_os_info() TO monitor_system_stats;

        -- Process information function
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
        GRANT EXECUTE ON FUNCTION pg_sys_process_info() TO monitor_system_stats;

        -- Network information function
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
        GRANT EXECUTE ON FUNCTION pg_sys_network_info() TO monitor_system_stats;

        -- CPU and memory information by process id or name
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

    END IF;

    IF os_platform = 'mac' THEN

        -- CPU information function
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

        -- CPU usage information function
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

        -- Memory information function
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

        -- IO analysis information function
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

         -- Disk information function
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

        -- Load average information function
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

        -- Operating system information function
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

        -- Process information function
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

        -- Network information function
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

        -- CPU and memory information by process id or name
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

    END IF;

    -- If platform is windows, installed its respective functions
    IF os_platform = 'windows' THEN

        -- Memory information function
        CREATE FUNCTION pg_sys_memory_info(
            OUT total_physical_memory int8,
            OUT avail_physical_memory int8,
            OUT memory_load_percentage int8,
            OUT total_page_file int8,
            OUT avail_page_file int8,
            OUT total_virtual_memory int8,
		    OUT avail_virtual_memory int8,
		    OUT avail_ext_virtual_memory int8
        )
        RETURNS SETOF record
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

        REVOKE ALL ON FUNCTION pg_sys_memory_info() FROM PUBLIC;
        GRANT EXECUTE ON FUNCTION pg_sys_memory_info() TO monitor_system_stats;

        -- Operating system information function
        CREATE FUNCTION pg_sys_os_info(
            OUT name text,
            OUT version text,
            OUT build_version text,
            OUT servicepack_major_version text,
            OUT servicepack_minor_version text,
            OUT host_name text,
            OUT num_of_users int,
            OUT num_of_licensed_users int,
            OUT architecture text,
            OUT installed_time text,
            OUT last_bootup_time text
        )
        RETURNS SETOF record
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

        REVOKE ALL ON FUNCTION pg_sys_os_info() FROM PUBLIC;
        GRANT EXECUTE ON FUNCTION pg_sys_os_info() TO monitor_system_stats;

		-- System CPU information function
        CREATE FUNCTION pg_sys_cpu_info(
            OUT device_id text,
            OUT description text,
            OUT manufacturer text,
            OUT name text,
            OUT processor_type int,
            OUT architecture int,
            OUT max_clock_speed int,
            OUT current_clock_speed int,
            OUT address_width int,
            OUT cpu_status int,
            OUT l2cache_size int,
            OUT l3cache_size int,
            OUT no_of_cores int,
            OUT no_of_enabled_cores int,
            OUT no_of_logical_processor int,
            OUT status text,
            OUT status_info int,
            OUT thread_count int,
            OUT last_error_code int
        )
        RETURNS SETOF record
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

        REVOKE ALL ON FUNCTION pg_sys_cpu_info() FROM PUBLIC;
        GRANT EXECUTE ON FUNCTION pg_sys_cpu_info() TO monitor_system_stats;

	END IF;

END
$$ language 'plpgsql';


