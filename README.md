# System Statistics
*system_stats* is a Postgres extension that provides functions to access system
level statistics that can be used for monitoring. It supports Linux, macOS and
Windows.

Note that not all values are relevant on all operating systems. In such cases
NULL is returned for affected values.

*Copyright (c) 2019 - 2020, EnterpriseDB Corporation. All Rights Reserved.*

## Building and Installing

### Linux and macOS
The module can be built using the PGXS framework:

- Unpack the file archive in a suitable directory.
- Ensure the PATH environment variable includes the directory containing the
  pg_config binary for the PostgreSQL installation you wish to build against.
- Compile and install the code.

For example:

    tar -zxvf system_stats-1.0.tar.gz
    cd system_stats-1.0
    PATH="/usr/local/pgsql/bin:$PATH" make USE_PGXS=1
    sudo PATH="/usr/local/pgsql/bin:$PATH" make install USE_PGXS=1

### Windows
The module built using the Visual Studio project file:

- Unpack the extensions files in $PGSRC/contrib/system_stats
- Set PG_INCLUDE_DIR and PG_LIB_DIR environment variables to make sure the
  PostgreSQL include and lib directories can be found for compilation. For
  example:

        PG_INCLUDE_DIR=C:\Program Files\PostgreSQL\12\include
        PG_LIB_DIR=C:\Program Files\PostgreSQL\12\lib

- Open the Visual Studio project file "system_stats.vcxproj" and build the
  project.

### Installing the Extension
Once the code has been built and installed, you can install the extension in
a database using the following SQL command:

    CREATE EXTENSION system_stats;

### Security
Due to the nature of the information returned by these functions, access is
restricted to superusers and members of the monitor_system_stats role which
will be created when the extension is installed. To allow users to access the
functions without granting them superuser access, add them to the
monitor_system_stats role. For example:

    GRANT monitor_system_stats to nagios;

## Functions
The following functions are provided to fetch system level statistics for all
platforms.

### pg_sys_os_info
This interface allows the user to get operating system statistics.

### pg_sys_cpu_info
This interface allows the user to get CPU information.

### pg_sys_cpu_usage_info
This interface allows the user to get CPU usage information. Values are a
percentage of time spent by CPUs for all operations.

### pg_sys_memory_info
This interface allows the user to get memory usage information. All the values
are in bytes.

### pg_sys_io_analysis_info
This interface allows the user to get an I/O analysis of block devices.

### pg_sys_disk_info
This interface allows the user to get the disk information.

### pg_sys_load_avg_info
This interface allows the user to get the average load of the system over 1, 5,
10 and 15 minute intervals.

### pg_sys_process_info
This interface allows the user to get process information.

### pg_sys_network_info
This interface allows the user to get network interface information.

### pg_sys_cpu_memory_by_process
This interface allows the user to get the CPU and memory information for each
process ID.

NOTE: macOS does not allow access to to process information for other users.
      e.g. If the database server is running as the postgres user, this function
      will fetch information only for processes owned by the postgres user.
      Other processes will be listed and include only the process ID and name;
      other columns will be NULL.


## Detailed output of each function

### pg_sys_os_info
- Name
- Version
- Host name
- Domain name
- Handle count
- Process count
- Thread count
- Architecture
- Last bootup time
- Uptime in seconds
    
### pg_sys_cpu_info
- Vendor
- Description
- Model name
- Processor type
- Logical processor
- Physical processor
- Number of cores
- Architecture
- Clock speed in hz
- CPU type
- CPU family
- Byte order
- L1d cache size
- L1i cache size
- L2 cache size
- L3 cache size

### pg_sys_cpu_usage_info
- Percent time spent in processing usermode normal process
- Percent time spent in processing usermode niced process
- Percent time spent in kernel mode process
- Percent time spent in idle mode
- Percent time spent in io completion
- Percent time spent in servicing interrupt
- Percent time spent in servicing software interrupt
- Percent user time spent
- Percent processor time spent
- Percent privileged time spent
- Percent interrupt time spent
    
### pg_sys_memory_info
- Total memory
- Used memory
- Free memory
- Total swap memory
- Used swap memory
- Free swap memory
- Total cache memory
- Total kernel memory
- Kernel paged memory
- Kernel non paged memory
- Total page file
- Available page file

### pg_sys_io_analysis_info
- Block device name
- Total number of reads
- Total number of writes
- Read bytes
- Written bytes
- Time spent in milliseconds for reading
- Time spent in milliseconds for writing

### pg_sys_disk_info
- File system of the disk
- File system type
- Mount point for the file system
- Drive letter
- Drive type
- Total space in bytes
- Used space in bytes
- Available space in bytes
- Number of total inodes
- Number of used inodes
- Number of free inodes

### pg_sys_load_avg_info
- 1 minute load average
- 5 minute load average
- 10 minute load average
- 15 minute load average

### pg_sys_process_info
- Number of total processes
- Number of running processes
- Number of sleeping processes
- Number of stopped processes
- Number of zombie processes

### pg_sys_network_info
- Name of the interface_name
- ipv4 address of the interface
- Number of total bytes transmitted
- Number of total packets transmitted
- Number of transmit errors by this network device
- Number of packets dropped during transmission
- Number of total bytes received
- Number of total packets received
- Number of receive errors by this network device
- Number of packets dropped by this network device
- Interface speed in mbps

### pg_sys_cpu_memory_by_process
- PID of the process
- Process name
- CPU usage in bytes
- Memory usage in bytes
- Total memory used in bytes
