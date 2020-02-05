/*------------------------------------------------------------------------
 * cpu_info.c
 *              System CPU information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	char       *output;
	char       *new_line_token;
	Datum      values[Natts_cpu_info];
	bool       nulls[Natts_cpu_info];
	char       architecture[MAXPGPATH];
	char       cpu_op_mode[MAXPGPATH];
	char       cpu_byte_order[MAXPGPATH];
	char       vendor_id[MAXPGPATH];
	char       cpu_family[MAXPGPATH];
	char       model[MAXPGPATH];
	char       model_name[MAXPGPATH];
	char       cpu_mhz[MAXPGPATH];
	char       cpu_l1d_cache_size[MAXPGPATH];
	char       cpu_l1i_cache_size[MAXPGPATH];
	char       cpu_l2_cache_size[MAXPGPATH];
	char       cpu_l3_cache_size[MAXPGPATH];
	char       cpu_l4_cache_size[MAXPGPATH];
	int        no_of_cpu = 0;
	int        threads_per_core = 0;
	int        core_per_socket = 0;
	int        no_of_sockets = 0;

	memset(nulls, 0, sizeof(nulls));
	memset(architecture, 0, MAXPGPATH);
	memset(cpu_op_mode, 0, MAXPGPATH);
	memset(cpu_byte_order, 0, MAXPGPATH);
	memset(vendor_id, 0, MAXPGPATH);
	memset(cpu_family, 0, MAXPGPATH);
	memset(model, 0, MAXPGPATH);
	memset(model_name, 0, MAXPGPATH);
	memset(cpu_mhz, 0, MAXPGPATH);
	memset(cpu_l1d_cache_size, 0, MAXPGPATH);
	memset(cpu_l1i_cache_size, 0, MAXPGPATH);
	memset(cpu_l2_cache_size, 0, MAXPGPATH);
	memset(cpu_l3_cache_size, 0, MAXPGPATH);
	memset(cpu_l4_cache_size, 0, MAXPGPATH);

	/* Run the command and read the output */
	output = runCommand(CPU_STATS_COMMAND);

	if (!output)
		return;
	else
	{
		/* Parse the command output and find CPU informations */
		for (new_line_token = strtok(output,"\n"); new_line_token != NULL; new_line_token = strtok(NULL, "\n"))
		{
			char *found = strstr(new_line_token, ":");
			found = trimStr((found+1));

			if (strstr(new_line_token, "Architecture") != NULL)
				memcpy(architecture, found, strlen(found));

				if (strstr(new_line_token, "CPU op-mode") != NULL)
				memcpy(cpu_op_mode, found, strlen(found));

			if (strstr(new_line_token, "Byte Order") != NULL)
				memcpy(cpu_byte_order, found, strlen(found));

			if (strstr(new_line_token, "CPU(s):") != NULL &&
				(found-new_line_token) == strlen("CPU(s):"))
				no_of_cpu = atoi(found);

			if (strstr(new_line_token, "Thread(s) per core") != NULL)
				threads_per_core = atoi(found);

			if (strstr(new_line_token, "Core(s) per socket") != NULL)
				core_per_socket = atoi(found);

			if (strstr(new_line_token, "Socket(s)") != NULL)
				no_of_sockets = atoi(found);

			if (strstr(new_line_token, "Vendor ID") != NULL)
				memcpy(vendor_id, found, strlen(found));

			if (strstr(new_line_token, "CPU family") != NULL)
				memcpy(cpu_family, found, strlen(found));

			if (strstr(new_line_token, "Model:") != NULL)
				memcpy(model, found, strlen(found));

			if (strstr(new_line_token, "Model name") != NULL)
				memcpy(model_name, found, strlen(found));

			if (strstr(new_line_token, "CPU MHz") != NULL)
				memcpy(cpu_mhz, found, strlen(found));

			if (strstr(new_line_token, "L1d cache") != NULL)
				memcpy(cpu_l1d_cache_size, found, strlen(found));

			if (strstr(new_line_token, "L1i cache") != NULL)
				memcpy(cpu_l1i_cache_size, found, strlen(found));

			if (strstr(new_line_token, "L2 cache") != NULL)
				memcpy(cpu_l2_cache_size, found, strlen(found));

			if (strstr(new_line_token, "L3 cache") != NULL)
				memcpy(cpu_l3_cache_size, found, strlen(found));

			if (strstr(new_line_token, "L4 cache") != NULL)
				memcpy(cpu_l4_cache_size, found, strlen(found));
		}
	}

	values[Anum_cpu_architecture] = CStringGetTextDatum(architecture);
	values[Anum_cpu_op_mode] = CStringGetTextDatum(cpu_op_mode);
	values[Anum_cpu_byte_order] = CStringGetTextDatum(cpu_byte_order);
	values[Anum_no_of_cpu] = Int32GetDatum(no_of_cpu);
	values[Anum_threads_per_core] = Int32GetDatum(threads_per_core);
	values[Anum_core_per_socket] = Int32GetDatum(core_per_socket);
	values[Anum_no_of_sockets] = Int32GetDatum(no_of_sockets);
	values[Anum_vendor_id] = CStringGetTextDatum(vendor_id);
	values[Anum_cpu_family] = CStringGetTextDatum(cpu_family);
	values[Anum_model] = CStringGetTextDatum(model);
	values[Anum_model_name] = CStringGetTextDatum(model_name);
	values[Anum_cpu_mhz] = CStringGetTextDatum(cpu_mhz);
	values[Anum_cpu_l1d_cache_size] = CStringGetTextDatum(cpu_l1d_cache_size);
	values[Anum_cpu_l1i_cache_size] = CStringGetTextDatum(cpu_l1i_cache_size);
	values[Anum_cpu_l2_cache_size] = CStringGetTextDatum(cpu_l2_cache_size);
	values[Anum_cpu_l3_cache_size] = CStringGetTextDatum(cpu_l3_cache_size);
	values[Anum_cpu_l4_cache_size] = CStringGetTextDatum(cpu_l4_cache_size);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);

	/* Free the allocated buffer) */
	if (output != NULL)
	{
		pfree(output);
		output = NULL;
	}
}
