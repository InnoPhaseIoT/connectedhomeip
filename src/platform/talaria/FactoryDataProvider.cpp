/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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
#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/Base64.h>
#include <platform/talaria/Config.h>
#include <platform/talaria/FactoryDataProvider.h>

namespace chip {
namespace DeviceLayer {

using namespace chip::Credentials;
using namespace chip::DeviceLayer::Internal;

namespace {
static constexpr uint32_t kDACPrivateKeySize = 32;
static constexpr uint32_t kDACPublicKeySize  = 65;

CHIP_ERROR LoadKeypairFromRaw(ByteSpan privateKey, ByteSpan publicKey, Crypto::P256Keypair & keypair)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    Crypto::P256SerializedKeypair serializedKeypair;
    ReturnErrorOnFailure(serializedKeypair.SetLength(privateKey.size() + publicKey.size()));
    memcpy(serializedKeypair.Bytes(), publicKey.data(), publicKey.size());
    memcpy(serializedKeypair.Bytes() + publicKey.size(), privateKey.data(), privateKey.size());
    err = keypair.Deserialize(serializedKeypair);
    return err;
}
} // namespace

CHIP_ERROR TalariaFactoryDataProvider::GetSetupPasscode(uint32_t & setupPasscode)
{
    CHIP_ERROR err;

    return TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_SetupPinCode, setupPasscode);
}

CHIP_ERROR TalariaFactoryDataProvider::GetSetupDiscriminator(uint16_t & setupDiscriminator)
{
    uint32_t setupDiscriminator32;
    ReturnErrorOnFailure(TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_SetupDiscriminator, setupDiscriminator32));
    VerifyOrReturnLogError(setupDiscriminator32 <= kMaxDiscriminatorValue, CHIP_ERROR_INVALID_ARGUMENT);
    setupDiscriminator = static_cast<uint16_t>(setupDiscriminator32);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::GetSpake2pIterationCount(uint32_t & iterationCount)
{
    return TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_Spake2pIterationCount, iterationCount);
}

CHIP_ERROR TalariaFactoryDataProvider::GetSpake2pSalt(MutableByteSpan & saltBuf)
{
    static constexpr size_t kSpake2pSalt_MaxBase64Len = BASE64_ENCODED_LEN(chip::Crypto::kSpake2p_Max_PBKDF_Salt_Length) + 1;

    CHIP_ERROR err                          = CHIP_NO_ERROR;
    char saltB64[kSpake2pSalt_MaxBase64Len] = { 0 };
    size_t saltB64Len                       = 0;

    err = TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_Spake2pSalt, saltB64, sizeof(saltB64), saltB64Len);
    ReturnErrorOnFailure(err);

    size_t saltLen = chip::Base64Decode32(saltB64, saltB64Len, reinterpret_cast<uint8_t *>(saltB64));
    ReturnErrorCodeIf(saltLen > saltBuf.size(), CHIP_ERROR_BUFFER_TOO_SMALL);

    memcpy(saltBuf.data(), saltB64, saltLen);
    saltBuf.reduce_size(saltLen);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::GetSpake2pVerifier(MutableByteSpan & verifierBuf, size_t & verifierLen)
{
    static constexpr size_t kSpake2pSerializedVerifier_MaxBase64Len =
        BASE64_ENCODED_LEN(chip::Crypto::kSpake2p_VerifierSerialized_Length) + 1;

    CHIP_ERROR err                                            = CHIP_NO_ERROR;
    char verifierB64[kSpake2pSerializedVerifier_MaxBase64Len] = { 0 };
    size_t verifierB64Len                                     = 0;

    err =
        TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_Spake2pVerifier, verifierB64, sizeof(verifierB64), verifierB64Len);
    ReturnErrorOnFailure(err);

    verifierLen = chip::Base64Decode32(verifierB64, verifierB64Len, reinterpret_cast<uint8_t *>(verifierB64));
    ReturnErrorCodeIf(verifierLen > verifierBuf.size(), CHIP_ERROR_BUFFER_TOO_SMALL);

    memcpy(verifierBuf.data(), verifierB64, verifierLen);
    verifierBuf.reduce_size(verifierLen);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::GetCertificationDeclaration(MutableByteSpan & outBuffer)
{
    size_t certSize;
    ReturnErrorOnFailure(
        TalariaConfig::ReadConfigValueBin(TalariaConfig::kConfigKey_CertDeclaration, outBuffer.data(), outBuffer.size(), certSize));
    outBuffer.reduce_size(certSize);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer)
{
    /* Not supported */
    out_firmware_info_buffer.reduce_size(0);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::GetDeviceAttestationCert(MutableByteSpan & outBuffer)
{
    size_t certSize;
    ReturnErrorOnFailure(
        TalariaConfig::ReadConfigValueBin(TalariaConfig::kConfigKey_DACCert, outBuffer.data(), outBuffer.size(), certSize));
    outBuffer.reduce_size(certSize);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::GetProductAttestationIntermediateCert(MutableByteSpan & outBuffer)
{
    size_t certSize;
    ReturnErrorOnFailure(
        TalariaConfig::ReadConfigValueBin(TalariaConfig::kConfigKey_PAICert, outBuffer.data(), outBuffer.size(), certSize));
    outBuffer.reduce_size(certSize);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaFactoryDataProvider::SignWithDeviceAttestationKey(const ByteSpan & messageToSign, MutableByteSpan & outSignBuffer)
{
    Crypto::P256ECDSASignature signature;
    Crypto::P256Keypair keypair;

    VerifyOrReturnError(!outSignBuffer.empty(), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(!messageToSign.empty(), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(outSignBuffer.size() >= signature.Capacity(), CHIP_ERROR_BUFFER_TOO_SMALL);

    uint8_t privKeyBuf[kDACPrivateKeySize];
    uint8_t pubKeyBuf[kDACPublicKeySize];
    size_t privKeyLen = sizeof(privKeyBuf);
    size_t pubKeyLen  = sizeof(pubKeyBuf);

    ReturnErrorOnFailure(
        TalariaConfig::ReadConfigValueBin(TalariaConfig::kConfigKey_DACPrivateKey, privKeyBuf, privKeyLen, privKeyLen));
    ReturnErrorOnFailure(TalariaConfig::ReadConfigValueBin(TalariaConfig::kConfigKey_DACPublicKey, pubKeyBuf, pubKeyLen, pubKeyLen));

    ReturnErrorOnFailure(LoadKeypairFromRaw(ByteSpan(privKeyBuf, privKeyLen), ByteSpan(pubKeyBuf, pubKeyLen), keypair));
    ReturnErrorOnFailure(keypair.ECDSA_sign_msg(messageToSign.data(), messageToSign.size(), signature));

    return CopySpanToMutableSpan(ByteSpan{ signature.ConstBytes(), signature.Length() }, outSignBuffer);
}

CHIP_ERROR TalariaFactoryDataProvider::GetVendorName(char * buf, size_t bufSize)
{
    size_t vendorNameLen = 0; // without counting null-terminator
    return TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_VendorName, buf, bufSize, vendorNameLen);
}

CHIP_ERROR TalariaFactoryDataProvider::GetVendorId(uint16_t & vendorId)
{
    ChipError err   = CHIP_NO_ERROR;
    uint32_t valInt = 0;

    err = TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_VendorId, valInt);
    ReturnErrorOnFailure(err);
    vendorId = static_cast<uint16_t>(valInt);
    return err;
}

CHIP_ERROR TalariaFactoryDataProvider::GetProductName(char * buf, size_t bufSize)
{
    size_t productNameLen = 0; // without counting null-terminator
    return TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_ProductName, buf, bufSize, productNameLen);
}

CHIP_ERROR TalariaFactoryDataProvider::GetProductId(uint16_t & productId)
{
    ChipError err   = CHIP_NO_ERROR;
    uint32_t valInt = 0;

    err = TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_ProductId, valInt);
    ReturnErrorOnFailure(err);
    productId = static_cast<uint16_t>(valInt);
    return err;
}

CHIP_ERROR TalariaFactoryDataProvider::GetProductURL(char * buf, size_t bufSize)
{
    return TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_ProductURL, buf, bufSize, bufSize);
}

CHIP_ERROR TalariaFactoryDataProvider::GetProductLabel(char * buf, size_t bufSize)
{
    return TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_ProductLabel, buf, bufSize, bufSize);
}

CHIP_ERROR TalariaFactoryDataProvider::GetHardwareVersionString(char * buf, size_t bufSize)
{
    size_t hardwareVersionStringLen = 0; // without counting null-terminator
    return TalariaConfig::ReadConfigValueStr(TalariaConfig::kConfigKey_HardwareVersionString, buf, bufSize, hardwareVersionStringLen);
}

CHIP_ERROR TalariaFactoryDataProvider::GetRotatingDeviceIdUniqueId(MutableByteSpan & uniqueIdSpan)
{
    return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
}

CHIP_ERROR TalariaFactoryDataProvider::GetSerialNumber(char * buf, size_t bufSize)
{
    return GenericDeviceInstanceInfoProvider<TalariaConfig>::GetSerialNumber(buf, bufSize);
}

CHIP_ERROR TalariaFactoryDataProvider::GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & day)
{
    return GenericDeviceInstanceInfoProvider<TalariaConfig>::GetManufacturingDate(year, month, day);
}

CHIP_ERROR TalariaFactoryDataProvider::GetHardwareVersion(uint16_t & hardwareVersion)
{
    return GenericDeviceInstanceInfoProvider<TalariaConfig>::GetHardwareVersion(hardwareVersion);
}

CHIP_ERROR TalariaFactoryDataProvider::GetPartNumber(char * buf, size_t bufSize)
{
    return GenericDeviceInstanceInfoProvider<TalariaConfig>::GetPartNumber(buf, bufSize);
}

} // namespace DeviceLayer
} // namespace chip
