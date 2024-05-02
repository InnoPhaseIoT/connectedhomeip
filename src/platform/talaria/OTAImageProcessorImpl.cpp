/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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
#include <app/clusters/ota-requestor/OTADownloader.h>
#include <app/clusters/ota-requestor/OTARequestorInterface.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/CHIPDeviceEvent.h>
#include <platform/KeyValueStoreManager.h>
#include <platform/talaria/TalariaUtils.h>

#include "OTAImageProcessorImpl.h"
#include <unistd.h>

// /*fota code start*/


#ifdef __cplusplus
extern "C" {
#endif
#include "hio/matter.h"
#include "hio/matter_hio.h"
#ifdef __cplusplus
}
#endif
// /*fota code end*/

int ext_flash_en = 0;
char *fw_name;
char *fw_hash;

using namespace ::chip::DeviceLayer::Internal;

namespace chip {

CHIP_ERROR OTAImageProcessorImpl::PrepareDownload()
{
    DeviceLayer::PlatformMgr().ScheduleWork(HandlePrepareDownload, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Finalize()
{
    DeviceLayer::PlatformMgr().ScheduleWork(HandleFinalize, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Apply()
{
    DeviceLayer::PlatformMgr().ScheduleWork(HandleApply, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Abort()
{
    DeviceLayer::PlatformMgr().ScheduleWork(HandleAbort, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
};

CHIP_ERROR OTAImageProcessorImpl::ProcessBlock(ByteSpan & block)
{
    CHIP_ERROR err = SetBlock(block);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Cannot set block data: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }
    DeviceLayer::PlatformMgr().ScheduleWork(HandleProcessBlock, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

void OTAImageProcessorImpl::HandlePrepareDownload(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    else if (imageProcessor->mDownloader == nullptr)
    {
        ChipLogError(SoftwareUpdate, "mDownloader is null");
        return;
    }
#if (CHIP_ENABLE_SECUREBOOT == true)
    imageProcessor->mOTAUpdateparam = ota_secure_enable_matter();
#endif

#if (CHIP_ENABLE_EXT_FLASH == true)
    ext_flash_en = 1;
#endif

#if (CHIP_ENABLE_OTA_STORAGE_ON_HOST == false)
    imageProcessor->mOTAUpdateHandle = ota_init_matter(&imageProcessor->mOTAUpdateparam, ext_flash_en);
    if (!imageProcessor->mOTAUpdateHandle) {
        ChipLogError(SoftwareUpdate, "[APP]Error: Initialising fota");
        return;
    }
#endif
    ChipLogProgress(SoftwareUpdate, "[APP]Perform Fota");
    imageProcessor->mHeaderParser.Init();
    imageProcessor->mDownloader->OnPreparedForDownload(CHIP_NO_ERROR);
}

void OTAImageProcessorImpl::HandleFinalize(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }

    imageProcessor->ReleaseBlock();
}

void OTAImageProcessorImpl::HandleAbort(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    /* Deinitialize fota */
    ota_deinit_matter(imageProcessor->mOTAUpdateHandle);
    imageProcessor->mOTAUpdateHandle = NULL;
    imageProcessor->ReleaseBlock();
#if (CHIP_ENABLE_OTA_STORAGE_ON_HOST == false)
    free(fw_hash);
#endif
}

void OTAImageProcessorImpl::HandleProcessBlock(intptr_t context)
{
    ChipLogProgress(SoftwareUpdate, "HandleProcessBlock");
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    else if (imageProcessor->mDownloader == nullptr)
    {
        ChipLogError(SoftwareUpdate, "mDownloader is null");
        return;
    }
    ByteSpan block = ByteSpan(imageProcessor->mBlock.data(), imageProcessor->mBlock.size());

    CHIP_ERROR error = imageProcessor->ProcessHeader(block);
    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Failed to process OTA image header");
        imageProcessor->mDownloader->EndDownload(error);
        return;
    }
    int err;
    ByteSpan blockToWrite = block;
    imageProcessor->mOTAfwinfo.data = blockToWrite.data();
    imageProcessor->mOTAfwinfo.data_len = blockToWrite.size();
#if (CHIP_ENABLE_OTA_STORAGE_ON_HOST == true)
    err = matter_notify(OTA_SOFTWARE_UPDATE_REQUESTOR, FOTA_ANNOUNCE_OTAPROVIDER, blockToWrite.size(), (char *)blockToWrite.data());
    if (err != 0)
    {
        ChipLogError(SoftwareUpdate, "T2_ota_write failed");
        imageProcessor->mDownloader->EndDownload(CHIP_ERROR_WRITE_FAILED);
        return;
    }
#else
    err = ota_perform_matter(imageProcessor->mOTAUpdateHandle, imageProcessor->mOTAfwinfo, imageProcessor->mParams.totalFileBytes);
    if (err != 0)
    {
        ChipLogError(SoftwareUpdate, "T2_ota_write failed");
        imageProcessor->mDownloader->EndDownload(CHIP_ERROR_WRITE_FAILED);
        ota_deinit_matter(imageProcessor->mOTAUpdateHandle);
        imageProcessor->mOTAUpdateHandle = NULL;
        imageProcessor->mHeaderParser.Clear();
        return;
    }
#endif
    imageProcessor->mParams.downloadedBytes += blockToWrite.size();
    imageProcessor->mDownloader->FetchNextData();
}

void OTAImageProcessorImpl::HandleApply(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    
#if (CHIP_ENABLE_OTA_STORAGE_ON_HOST == false)
    if (imageProcessor->mOTAUpdateHandle == NULL)
        return;
    /* fota commit.  This will reset the system */
    if (!ota_commit_matter(imageProcessor->mOTAUpdateHandle, 1)) {
        ChipLogError(SoftwareUpdate,"[APP]Error: FOTA commit failed");
        return;
    }
    /* Deinitialize fota */
    ota_deinit_matter(imageProcessor->mOTAUpdateHandle);
#endif
    imageProcessor->mOTAUpdateHandle = NULL;
    imageProcessor->mHeaderParser.Clear();
    ChipLogProgress(SoftwareUpdate, "Commit Done!. de-init OTA");
#if (CHIP_ENABLE_OTA_STORAGE_ON_HOST == true)
    matter_notify(OTA_SOFTWARE_UPDATE_REQUESTOR, FOTA_ANNOUNCE_OTAPROVIDER, 0, NULL);
#endif
}

void OTAImageProcessorImpl::ota_get_firmware_name(char *str)
{
    if (str == NULL) {
        ChipLogError(SoftwareUpdate, "APP NAME is null");
        return;
    }

    fw_name = str;
}

CHIP_ERROR OTAImageProcessorImpl::SetBlock(ByteSpan & block)
{
    if (!IsSpanUsable(block))
    {
        ReleaseBlock();
        return CHIP_NO_ERROR;
    }
    if (mBlock.size() < block.size())
    {
        if (!mBlock.empty())
        {
            ReleaseBlock();
        }
        uint8_t * mBlock_ptr = static_cast<uint8_t *>(Platform::MemoryAlloc(block.size()));
        if (mBlock_ptr == nullptr)
        {
            return CHIP_ERROR_NO_MEMORY;
        }
        mBlock = MutableByteSpan(mBlock_ptr, block.size());
    }
    CHIP_ERROR err = CopySpanToMutableSpan(block, mBlock);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Cannot copy block data: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::ProcessHeader(ByteSpan & block)
{
    if (mHeaderParser.IsInitialized())
    {
        ChipLogProgress(SoftwareUpdate, "ProcessHeader");
        OTAImageHeader header;
        CHIP_ERROR error = mHeaderParser.AccumulateAndDecode(block, header);

        // Need more data to decode the header
        ReturnErrorCodeIf(error == CHIP_ERROR_BUFFER_TOO_SMALL, CHIP_NO_ERROR);
        ReturnErrorOnFailure(error);
#if (CHIP_ENABLE_OTA_STORAGE_ON_HOST == false)
        fw_hash = pvPortMalloc(header.mImageDigest.size());
        if (fw_hash == NULL) {
		    free(fw_hash);
            ReturnErrorOnFailure(error);
        }
        
        memcpy(fw_hash, header.mImageDigest.data(), header.mImageDigest.size());
        mOTAfwinfo.hash_len = static_cast<decltype(mOTAfwinfo.hash_len)>(header.mImageDigest.size());
        mOTAfwinfo.hash_str = fw_hash;
        mOTAfwinfo.fw_name = fw_name;
#endif
        ChipLogProgress(SoftwareUpdate, "Image Header software version: %d payload size: %lu", header.mSoftwareVersion,
                        (long unsigned int) header.mPayloadSize);
        mParams.totalFileBytes = header.mPayloadSize;
        mHeaderParser.Clear();
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::ReleaseBlock()
{
    if (mBlock.data() != nullptr)
    {
        chip::Platform::MemoryFree(mBlock.data());
    }
    mBlock = MutableByteSpan();
    return CHIP_NO_ERROR;
}

} // namespace chip
