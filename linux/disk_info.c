/*------------------------------------------------------------------------
 * disk_info.c
 *              System disk information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"

#include <mntent.h>
#include <regex.h>
#include <sys/statvfs.h>

#include "system_stats.h"

void ReadDiskInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

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
	FILE       *fp = NULL;
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

	/* get the file system descriptor */
	fp = setmntent(FILE_SYSTEM_MOUNT_FILE_NAME, "r");

	if (!fp)
	{
		char file_name[MAXPGPATH];
		snprintf(file_name, MAXPGPATH, "%s", FILE_SYSTEM_MOUNT_FILE_NAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading file system information",
						file_name)));
		return;
	}
	else
	{
		struct mntent  *ent;
		struct statvfs   buf;

		while ((ent = getmntent(fp)) != NULL)
		{
			if (ignoreFileSystemTypes(ent->mnt_fsname) || ignoreMountPoints(ent->mnt_dir))
				continue;

			memset(&buf, 0, sizeof(buf));
			/*
			 * If statvfs() fails, just report zeroes.  It's better to still
			 * report statistics for filesystems that we are able to stat,
			 * rather than failing the whole data.
			 */
			if (statvfs(ent->mnt_dir, &buf) != 0)
			{
				ereport(WARNING,
					(errcode_for_file_access(),
						errmsg("statvfs failed: %s", ent->mnt_dir)));
			}

			total_space_bytes = (uint64_t)(buf.f_blocks  * buf.f_bsize);

			/* If total space of file system is zero, ignore that from list */
			if (total_space_bytes == 0)
				continue;

			used_space_bytes = (uint64_t)((buf.f_blocks - buf.f_bfree) * buf.f_bsize);
			available_space_bytes = (uint64_t)(buf.f_bavail * buf.f_bsize);
			total_inodes = (uint64_t)buf.f_files;
			free_inodes = (uint64_t)buf.f_ffree;
			used_inodes = (uint64_t)(total_inodes - free_inodes);
			memcpy(file_system, ent->mnt_fsname, strlen(ent->mnt_fsname));
			memcpy(mount_point, ent->mnt_dir, strlen(ent->mnt_dir));
			memcpy(file_system_type, ent->mnt_type, strlen(ent->mnt_type));

			values[Anum_disk_file_system] = CStringGetTextDatum(file_system);
			values[Anum_disk_file_system_type] = CStringGetTextDatum(file_system_type);
			values[Anum_disk_mount_point] = CStringGetTextDatum(mount_point);
			values[Anum_disk_total_space] = Int64GetDatumFast(total_space_bytes);
			values[Anum_disk_used_space] = Int64GetDatumFast(used_space_bytes);
			values[Anum_disk_free_space] = Int64GetDatumFast(available_space_bytes);
			values[Anum_disk_total_inodes] = Int64GetDatumFast(total_inodes);
			values[Anum_disk_used_inodes] = Int64GetDatumFast(used_inodes);
			values[Anum_disk_free_inodes] = Int64GetDatumFast(free_inodes);

			nulls[Anum_disk_drive_letter] = true;
			nulls[Anum_disk_drive_type] = true;

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
		endmntent(fp);
	}
}
