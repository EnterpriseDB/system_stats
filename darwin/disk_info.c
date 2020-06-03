/*------------------------------------------------------------------------
 * disk_info.c
 *              System disk information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <regex.h>
#include <sys/param.h>
#include <sys/mount.h>

/* This function is used to ignore the file system types */
bool ignoreFileSystemTypes(char *fs_mnt)
{
	regex_t regex;
	int reg_return;
	bool ret_value = false;

	reg_return = regcomp(&regex, IGNORE_FILE_SYSTEM_TYPE_REGEX, REG_EXTENDED);
	if (reg_return)
	{
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Could not compile regex")));
		return ret_value;
	}

	/* Execute regular expression */
	reg_return = regexec(&regex, fs_mnt, 0, NULL, 0);
	if (!reg_return)
		ret_value = true;
	else if (reg_return == REG_NOMATCH)
		ret_value = false;
	else
	{
		ret_value = false;
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("regex match failed")));
	}

	/* Free memory allocated to the pattern buffer by regcomp() */
	regfree(&regex);

	return ret_value;
}

/* This function is used to ignore the mount points */
bool ignoreMountPoints(char *fs_mnt)
{
	regex_t regex;
	int reg_return;
	bool ret_value = false;

	reg_return = regcomp(&regex, IGNORE_MOUNT_POINTS_REGEX, REG_EXTENDED);
	if (reg_return)
	{
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Could not compile regex")));
		return ret_value;
	}

	/* Execute regular expression */
	reg_return = regexec(&regex, fs_mnt, 0, NULL, 0);
	if (!reg_return)
		ret_value = true;
	else if (reg_return == REG_NOMATCH)
		ret_value = false;
	else
	{
		ret_value = false;
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("regex match failed")));
	}

	/* Free memory allocated to the pattern buffer by regcomp() */
	regfree(&regex);

	return ret_value;
}

void ReadDiskInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_disk_info];
	bool       nulls[Natts_disk_info];
	char       file_system[MAXPGPATH];
	char       mount_point[MAXPGPATH];
	char       file_system_type[MAXPGPATH];
	uint64     used_space_bytes = 0;
	uint64     total_space_bytes = 0;
	uint64     available_space_bytes = 0;
	uint64     total_inodes = 0;
	uint64     used_inodes = 0;
	uint64     free_inodes = 0;

	memset(nulls, 0, sizeof(nulls));
	memset(file_system, 0, MAXPGPATH);
	memset(mount_point, 0, MAXPGPATH);
	memset(file_system_type, 0, MAXPGPATH);

	struct statfs *buf;
	int total_count = 0;
	int i = 0;
	total_count = getmntinfo(&buf, MNT_NOWAIT);
	for(i = 0; i < total_count; i++)
	{
		if (ignoreFileSystemTypes(buf[i].f_fstypename) || ignoreMountPoints(buf[i].f_mntonname))
			continue;

		total_space_bytes = (uint64_t)(buf->f_blocks  * buf->f_bsize);

		/* If total space of file system is zero, ignore that from list */
		if (total_space_bytes == 0)
			continue;

		used_space_bytes = (uint64_t)((buf->f_blocks - buf->f_bfree) * buf->f_bsize);
		available_space_bytes = (uint64_t)(buf->f_bavail * buf->f_bsize);
		total_inodes = (uint64_t)buf->f_files;
		free_inodes = (uint64_t)buf->f_ffree;
		used_inodes = (uint64_t)(total_inodes - free_inodes);

		memcpy(file_system, buf[i].f_fstypename, strlen(buf[i].f_fstypename));
		memcpy(mount_point, buf[i].f_mntonname, strlen(buf[i].f_mntonname));
		memcpy(file_system_type, buf[i].f_mntfromname, strlen(buf[i].f_mntfromname));

		/* not used for this platform so set to NULL */
		nulls[Anum_disk_drive_letter] = true;
		nulls[Anum_disk_drive_type] = true;

		values[Anum_disk_file_system] = CStringGetTextDatum(file_system);
		values[Anum_disk_file_system_type] = CStringGetTextDatum(file_system_type);
		values[Anum_disk_mount_point] = CStringGetTextDatum(mount_point);
		values[Anum_disk_total_space] = Int64GetDatumFast(total_space_bytes);
		values[Anum_disk_used_space] = Int64GetDatumFast(used_space_bytes);
		values[Anum_disk_free_space] = Int64GetDatumFast(available_space_bytes);
		values[Anum_disk_total_inodes] = Int64GetDatumFast(total_inodes);
		values[Anum_disk_used_inodes] = Int64GetDatumFast(used_inodes);
		values[Anum_disk_free_inodes] = Int64GetDatumFast(free_inodes);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		//reset the value again
		memset(file_system, 0, MAXPGPATH);
		memset(mount_point, 0, MAXPGPATH);
		memset(file_system_type, 0, MAXPGPATH);
		used_space_bytes = 0;
		total_space_bytes = 0;
		available_space_bytes = 0;
		total_inodes = 0;
		used_inodes = 0;
		free_inodes = 0;
	}
}
