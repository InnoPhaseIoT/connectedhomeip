/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Platform-specific implementatiuon of KVS for linux.
 */

#include <platform/KeyValueStoreManager.h>

#include <iterator>
#include <string>
#include <map>

#include <algorithm>
#include <string.h>

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
// #include <platform/Linux/CHIPLinuxStorage.h>

#define DATA_PART_KEY_STORAGE "/data/"

#ifdef __cplusplus
extern "C" {
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>

/* Ideally we should be including the header file but that's a manual
   process as of now, hence declaring required function here.
   Header file name: <components/utils.h> */
char * utils_file_get(const char * path, int * len);
}
#endif

namespace chip {
namespace DeviceLayer {
namespace PersistedStorage {
int ret_code;
int replaceSlahWithUnderscore(const char * key, char * configfile)
{
	if (key == NULL || configfile == NULL) {
		return -1;
	}

	memset(configfile, 0, sizeof(configfile));
	strcat(configfile, DATA_PART_KEY_STORAGE);
	char *key_place_ptr = configfile + strlen(configfile);
	strcat(configfile, key);
	for (uint8_t i = 0; i < strlen(key); i++)
	{
		if (key_place_ptr[i] == '/') {
			key_place_ptr[i] = '_';
		}
	}
	return 0;
}

// using namespace std;
KeyValueStoreManagerImpl KeyValueStoreManagerImpl::sInstance;

CHIP_ERROR KeyValueStoreManagerImpl::_Put(const char * key, const void * value, size_t value_size)
{
	/*
	 * Storing key-value pair into /data partition
	 *
	 *  Each Key are hash-value computed using function djb2_order_sensitive_hash
	 *  This hash-value have been used for file creation and storing
	 *
	 *
	 */
	char configfile[100];
	int retval = 0;
	FILE *sample_file;
	retval = replaceSlahWithUnderscore(key,configfile);
	/* TODO: retval unhandled */
	/* Write to file */
	if ((sample_file = fopen(configfile, "w")) != NULL)
	{
		fwrite(value, 1, value_size, sample_file);
		fclose(sample_file);
	}
	else
	{
		// ChipLogError(DeviceLayer, "Cannot open the file to store key");
		return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Get(const char * key, void * value, size_t value_size, size_t * read_bytes_size,
		size_t offset_bytes)
{
	int size=0;

	/*
	 *  Storing key-value pair into /data partition
	 *
	 *  Each Key are hash-value computed using function djb2_order_sensitive_hash
	 *  This hash-value have been used for retrival of value which was Injected from _Put function
	 *
	 *
	 */
	char configfile[100];
	int retval = 0;
	FILE *sample_file;
	retval = replaceSlahWithUnderscore(key,configfile);
	/* Read File */
	char * read;
	sample_file = fopen(configfile, "r");
	if (sample_file)
	{
		read = utils_file_get(configfile, &size);
		fclose(sample_file);
		if(size != 0)
		{
			if (value_size < (size - 1))
			{
				memcpy(value, read + offset_bytes, value_size);
				*read_bytes_size = value_size;
				if (read != NULL)
					free(read);

				return CHIP_ERROR_BUFFER_TOO_SMALL;
			}
			/* -1 from read size is for removing EOF
Check: currently offset_bytes always is 0,
So does it required to be consided for (size - 1) value? */

		        memcpy(value, read + offset_bytes, size - 1);
		        *read_bytes_size = size - 1;
		}
		else if (size == 0){
			*read_bytes_size = size ;
		}
	}
	else
	{
		memset(value, 0, value_size);
		return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
	}
	if (read != NULL)
		free(read);
	return CHIP_NO_ERROR;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Delete(const char * key)
{
	/*
	 *  Each Key are hash computed using function djb2_order_sensitive_hash
	 *  This hash-value is used here to Delete the file.
	 */
	char configfile[100];
	int retval = 0;
	FILE *sample_file;
	retval = replaceSlahWithUnderscore(key,configfile);
	/* Delete file */
	if ((sample_file = fopen(configfile, "r")) != NULL)
	{
		fclose(sample_file);
		ret_code = unlink(configfile);
		return CHIP_NO_ERROR;
	}
	else
	{
		return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
	}
}

CHIP_ERROR KeyValueStoreManagerImpl::EraseAll(void)
{
	/* TODO: This erases all the contents of DATA partition,
	   if at all you are storing something else in the partition that will go.
Resolution: create separate directory for matter in /data/ parition and use the same
*/
	DIR * dir;
	struct dirent * entry;
	const char * folderPath = "/data";
	dir                     = opendir(folderPath);
	if (dir == NULL)
	{
		return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
	}
	FILE *sample_file;
	int err_count =0;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
		{
			char filePath[100];
			snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
			/* TODO: remove()*/
			/* Delete file */
			/* Delete file */
			if ((sample_file = fopen(filePath, "r")) != NULL)
			{
				fclose(sample_file);
				ret_code = unlink(filePath);
				if(ret_code < 0)
				{
					// os_printf("Not Deleted  \n");
					err_count++;
					return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
				}
			}
		}
	}
	closedir(dir);
	if (err_count == 0) 
	{
		return CHIP_NO_ERROR;
	}
	else
	{
		return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
	}

}

} // namespace PersistedStorage
} // namespace DeviceLayer
} // namespace chip
