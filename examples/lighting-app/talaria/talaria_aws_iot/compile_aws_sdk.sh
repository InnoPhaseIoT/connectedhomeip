#!/bin/bash

# freertos_sdk directory path
TOP=../../../third_party/talaria/repo/freertos_sdk

# Check if the talaria_two_aws directory exists
if [ ! -e "${TOP}/apps/talaria_two_aws" ]; then

    echo "aws_iot_sdk compilation failed, \"<sdk_x.y>/apps/talaria_two_aws\" not found..."
    echo "please refer to README or \"<sdk_x.y>/apps/iot_aws/doc/Application_for_IoT_AWS.pdf\" to setup AWS IoT SDK for Talaria TWO Platform"
    exit 1
fi

# Change directory to compile libaws_iot_sdk_t2 and and libaws_iot_sdk_t2_pal
pushd "$TOP/apps/talaria_two_aws/sample_apps/sdk_3.x/subscribe_publish_sample" || { echo "aws_iot_sdk compilation failed, invalid directory path"; exit 1; }

# Run the make command
make || { echo "aws_iot_sdk compilation failed"; popd; exit 1; }

# Restore the original directory
popd || { echo "Failed to restore original directory"; exit 1; }

# Make command successfull
echo "aws_iot_sdk compiled successfully"
exit 0
