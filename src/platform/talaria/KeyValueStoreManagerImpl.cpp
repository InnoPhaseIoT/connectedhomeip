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
#include <platform/talaria/Config.h>

#define DATA_PART_KEY_STORAGE "/data/kvs"

#ifdef __cplusplus
extern "C" {
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <utils.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fs_utils.h>
}
#endif

namespace chip {
namespace DeviceLayer {
namespace PersistedStorage {

using namespace chip::DeviceLayer::Internal;

int ret_code;
int replaceSlahWithUnderscore(const char * key, char * configfile)
{
	if (key == NULL || configfile == NULL) {
		return -1;
	}

	memcpy(configfile, key, strlen(key));
	for (uint8_t i = 0; i < strlen(key); i++)
	{
		if (configfile[i] == '/') {
			configfile[i] = '_';
		}
	}
	configfile[strlen(key)] = '\0';
	return 0;
}

// using namespace std;
KeyValueStoreManagerImpl KeyValueStoreManagerImpl::sInstance;

CHIP_ERROR KeyValueStoreManagerImpl::_Put(const char * key, const void * value, size_t value_size)
{
	CHIP_ERROR error;
	char configfile[TalariaConfig::kMaxConfigKeyNameLength];
	int retval = 0;
	FILE *sample_file;
    struct stat st = {0};
	retval = replaceSlahWithUnderscore(key,configfile);
	TalariaConfig::Key key_file = {TalariaConfig::kConfigNamespace_KVS, configfile};

	/* Write to file */
	error = TalariaConfig::WriteConfigValueBin(key_file, (const uint8_t *) value, value_size);
	return error;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Get(const char * key, void * value, size_t value_size, size_t * read_bytes_size,
		size_t offset_bytes)
{
	size_t size = 0;
	char configfile[TalariaConfig::kMaxConfigKeyNameLength];
	CHIP_ERROR retval = CHIP_NO_ERROR;
	FILE *sample_file;
	uint8_t *read;
	replaceSlahWithUnderscore(key,configfile);
	TalariaConfig::Key key_file = {TalariaConfig::kConfigNamespace_KVS, configfile};

	retval = TalariaConfig::ReadConfigValueBin(key_file, value, value_size, size);
	if (retval != CHIP_NO_ERROR || size <= 0) {
		*read_bytes_size = 0;
		memset(value, 0, value_size);
		return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
	}

	if (value_size < (size - 1)) {
		if (offset_bytes != 0) {
			memcpy(value, value + offset_bytes, value_size);
			/* TODO: Handle the case when read offset is non-zero better
			* As with current handling the bytes after the value_size will not be passed
			* in the current memcpy. Till now we haven't observed any request with
			* offset_bytes value as non-zero. Asserting it here to detect the scenario if
			* there is any detected.
			*/
			os_printf("ASSERT: Offset_bytes value non-zero is not handled properly\n");
			assert(false);
		}
		*read_bytes_size = value_size;
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	if (offset_bytes != 0) {
		memcpy(value, value + offset_bytes, size);
	}

        *read_bytes_size = size;

	return CHIP_NO_ERROR;
}

CHIP_ERROR KeyValueStoreManagerImpl::_Delete(const char * key)
{
	/*
	 *  Each Key are hash computed using function djb2_order_sensitive_hash
	 *  This hash-value is used here to Delete the file.
	 */
	char configfile[TalariaConfig::kMaxConfigKeyNameLength];
	int retval = 0;
	FILE *sample_file;
	retval = replaceSlahWithUnderscore(key,configfile);
	TalariaConfig::Key key_file = {TalariaConfig::kConfigNamespace_KVS, configfile};

	/* Delete file */
	return TalariaConfig::ClearConfigValue(key_file);
}

CHIP_ERROR KeyValueStoreManagerImpl::EraseAll(void)
{
	DIR * dir;
	struct dirent * entry;
	const char * folderPath = DATA_PART_KEY_STORAGE;
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
			/* Delete file */
			if ((sample_file = fopen(filePath, "r")) != NULL)
			{
				fclose(sample_file);
				ret_code = unlink(filePath);
				if(ret_code < 0)
				{
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
