#!/bin/ash
# Note: run on target

# Exit immediately on SIGHUP, SIGINT or SIGTERM.
trap 'printf "\nINTERRUPT RECEIVED. EXITING.\n" ; exit 1' 1 2 15

source /tmp/yellow_testing/configuration.cfg

#=== FUNCTION =============================================================================
#
#        NAME: prompt_enter
# DESCRIPTION: Prompt a message to terminal and wait for ENTER.
# PARAMETER 1: message
#
#==========================================================================================
prompt_enter() {

    run_time=$(date +"%Y-%m-%d-%H:%M:%S:")
    printf "$run_time: $1, then press ENTER "

    if ! read prompt_input
    then
        echo "ERROR READING INPUT!"
        exit 1
    fi
}


#=== FUNCTION =============================================================================
#
#        NAME: prompt_yes_no
# DESCRIPTION: Prompt a message to terminal and wait for 'y' or 'n'
# PARAMETER 1: message
#      RETURN: 0 if 'y', 1 if 'n'
#
#==========================================================================================
prompt_yes_no() {

    run_time=$(date +"%Y-%m-%d-%H:%M:%S:")
    printf "$run_time: $1 (Y/N) "

    local resp=""

    while [ "$resp" != "Y" -a "$resp" != "N" ]
    do
        if ! read prompt_input
        then
            echo "ERROR READING INPUT!"
            exit 1
        fi

        local resp=$(echo $prompt_input | tr 'a-z' 'A-Z')
    done

    test "$resp" = "Y"
}


#=== FUNCTION ==================================================================
#
#        NAME: triLED
# DESCRIPTION: Turn on/off tri-LED
# PARAMETER 1: led name (red/green/blue)
# PARAMETER 2: led state (on/off)
#
#    RETURNS: None
#
#===============================================================================
triLED() {
    local triLEDRed="/sys/devices/platform/expander.0/tri_led_red"
    local triLEDGreen="/sys/devices/platform/expander.0/tri_led_grn"
    local triLEDBlue="/sys/devices/platform/expander.0/tri_led_blu"

    local ledFile=""
    if [ "$1" = "red" ]
    then
        local ledFile=$triLEDRed

    elif [ "$1" = "green" ]
    then
        local ledFile=$triLEDGreen

    elif [ "$1" = "blue" ]
    then
        local ledFile=$triLEDBlue
    else
        echo "Unknown tri-LED"
        return
    fi

    if [ "$2" = "on" ]
    then
        echo "1" > "$ledFile"

    elif [ "$2" = "off" ]
    then
        echo "0" > "$ledFile"
    else
        echo "Unknown LED state"
    fi
}

#=== FUNCTION ==================================================================
#
#        NAME: genericLED
# DESCRIPTION: Turn on/off generic LED
# PARAMETER 1: led state (on/off)
#
#    RETURNS: None
#
#===============================================================================
genericLED() {
    local genericLEDPath="/sys/devices/platform/expander.0/generic_led"

    if [ "$1" = "on" ]
    then
        echo 1 > "$genericLEDPath"
    else
        if [ "$1" = "off" ]
        then
            echo 0 > "$genericLEDPath"
        else
            echo "Unknown Generic LED state"
        fi
    fi
}


#=== FUNCTION ==================================================================
#
#        NAME: test_buzzer
# DESCRIPTION: Test the buzzer
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
test_buzzer() {

    # Connect button to buzzer in the Data Hub.
    if ! /legato/systems/current/bin/dhub set source /app/buzzer/enable /app/button/value > /dev/null
    then
        failure_msg="Buzzer test failed - Failed to connect button to buzzer"
        test_result="FAILED"
        return 1
    fi

    prompt_yes_no "Press generic button and listen for buzzer. Do you hear the buzzer's sound when pressing button?"
    local result=$?

    if [ $result -ne 0 ]
    then
        failure_msg="Buzzer test failed"
        test_result="FAILED"
        return 1
    fi

    failure_msg=""
    test_result="PASSED"
    return 0
}


#=== FUNCTION ==================================================================
#
#        NAME: read_light_sensor
# DESCRIPTION: Read value of light sensor
#   PARAMETER: None
#
#      PRINTS: Light sensor value
#     RETURNS: 0 on success, 1 on failure
#
#===============================================================================
read_light_sensor() {
    local light_sensor_path="/sys/bus/iio/devices/iio:device1/in_intensity_input"
    if ! local light_value=$(cat $light_sensor_path)
    then
        echo "ERROR: can't read $light_sensor_path" >&2
        return 1
    fi

    # Drop the decimal places.
    local light_value=$(echo "$light_value" | sed 's/\..*$//')

    echo "Light Sensor Value: '$light_value'" >&2

    echo "$light_value"
}

#=== FUNCTION ==================================================================
#
#        NAME: test_light_sensor
# DESCRIPTION: Test the light sensor
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
test_light_sensor() {

    if ! uncovered_light=$(read_light_sensor)
    then
        failure_msg="Can't read light sensor"
        echo "$failure_msg"
        test_result="FAILED"
        return 1
    fi

    prompt_enter "Please cover the light sensor with your finger"

    if ! covered_light=$(read_light_sensor)
    then
        failure_msg="Can't read light sensor after it was covered"
        echo "$failure_msg"
        test_result="FAILED"
        return 1
    fi

    # The difference in the reading should be more than twice brighter when
    # uncovered vs when covered with a finger.
    covered_times_two=$(dc $covered_light 2 * p)
    if [ $covered_times_two -ge $uncovered_light ]
    then
        failure_msg="Light sensor has a problem"
        echo "$failure_msg"
        test_result="FAILED"
        return 1
    fi

    failure_msg=""
    return 0
}

#=== FUNCTION ==================================================================
#
#        NAME: yellowManualTest_initial
# DESCRIPTION: Perform the initial test
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
yellowManualTest_initial() {

    failure_msg=""

    triLED "red" "off"
    triLED "green" "off"
    triLED "blue" "off"

    if ! prompt_yes_no "Is the HARDware-controlled LED yellow?"
    then
        failure_msg="Wrong hardware-controlled LED state"
        echo "$failure_msg"
        test_result="FAILED"
        return 1
    fi

    triLED "red" "on"
    triLED "green" "on"
    triLED "blue" "on"

    if ! prompt_yes_no "Is the SOFTware-controlled LED white?"
    then
        failure_msg="Software-controlled LED has a problem"
    fi

    triLED "red" "off"
    triLED "green" "off"
    triLED "blue" "off"

    if [ "$failure_msg" ]
    then
        echo "$failure_msg"
        test_result="FAILED"
        return 1
    fi

    test_result="PASSED"
    return 0
}

#=== FUNCTION ==================================================================
#
#        NAME: yellowTest_WifiScan
# DESCRIPTION: Scan WiFi and see dedicated AP
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
yellowTest_WifiScan() {

    /legato/systems/current/bin/app start wifiService

    /legato/systems/current/bin/wifi client start
    if [ $? -eq 0 ]
    then
        echo 'WiFi started successfully'
    else
        failure_msg='Unable to start WiFi'
        echo "$failure_msg"
        result=1
    fi

    sleep 2

    local result=0
    failure_msg=""

    if /legato/systems/current/bin/wifi client scan | grep "$WIFI_ACCESSPOINT"
    then
        echo "Found WiFi Accesspoint $WIFI_ACCESSPOINT"
    else
        failure_msg="Unable to find WiFi Accesspoint $WIFI_ACCESSPOINT"
        echo "$failure_msg"
        result=1
    fi

    /legato/systems/current/bin/wifi client stop

    return $result
}



#=== FUNCTION ==================================================================
#
#        NAME: yellowTest_uSD
# DESCRIPTION: Test Read Write uSD Card
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
yellowTest_uSD() {

    failure_msg=""

    if /bin/mkdir /tmp/sd
    then
        echo 'Created directory for SD card successfully'
    else
        echo 'Failed to create directory for SD card.'
        return 1
    fi

    if /bin/mount -ofmask=0111 -odmask=0000 -osmackfsdef=sd /dev/mmcblk0p1 /tmp/sd
    then
        echo 'Mounted SD card successfully'
    else
        echo 'Failed to mount SD card.'
        return 1
    fi

    if /bin/touch /tmp/sd/log.txt
    then
        echo 'Create file on SD card successfully'
    else
        echo 'Failed to create file on SD card'
        return 1
    fi

    if /bin/echo "foo" >> /tmp/sd/log.txt
    then
        echo 'Wrote to file on SD card successfully'
    else
        echo 'Failed to write to file on SD card'
        return 1
    fi

    if local sdContent=$(/bin/cat /tmp/sd/log.txt)
    then
        if [ "$sdContent" = "foo" ]
        then
            echo 'SD card content read back successfully'
        else
            echo "SD card contents didn't match what was written"
            return 1
        fi
    else
        echo 'Failed to read Read file on SDcard unsuccessfully'
        return 1
    fi

    return 0
}

#=== FUNCTION ==================================================================
#
#        NAME: yellowTest_USB
# DESCRIPTION: Test Read USB
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
yellowTest_USB() {

    if [ -d "/sys/devices/7c00000.hsic_host/usb1/1-1/1-1.1" ]
    then
        echo "USB device 1-1.1 exists"
    else
        failure_msg="USB device 1-1.1 doesn't exist"
        echo "$failure_msg"
        return 1
    fi

    if [ -d "/sys/devices/7c00000.hsic_host/usb1/1-1/1-1.2" ]
    then
        echo "USB device 1-1.2 exists"
    else
        failure_msg="USB device 1-1.2 doesn't exist"
        echo "$failure_msg"
        return 1
    fi

    if [ -d "/sys/devices/7c00000.hsic_host/usb1/1-1/1-1.3" ]
    then
        echo "USB device 1-1.3 exists"
    else
        failure_msg="USB device 1-1.3 doesn't exist"
        echo "$failure_msg"
        return 1
    fi

    if [ -d "/sys/devices/7c00000.hsic_host/usb1/1-1/1-1:1.0" ]
    then
        echo "USB device 1-1:1.0 exists"
    else
        failure_msg="USB device 1-1:1.0 doesn't exist"
        echo "$failure_msg"
        return 1
    fi

    failure_msg=""
    return 0
}

#=== FUNCTION ==================================================================
#
#        NAME: yellowTest_I2CDetect
# DESCRIPTION: Perform the I2C Detect Address test
#   PARAMETER: None
#
#   RETURNS 1: PASSED/FAILED
#   RETURNS 2: Failure message
#
#===============================================================================
yellowTest_I2CDetect() {
    echo "Stop legato ..."
    if /legato/systems/current/bin/legato stop
    then
        echo 'Stopped Legato successfully'
    else
        echo 'Unable to stop Legato'
        return 1
    fi

    echo "Enable all the ports on the I2C hub"
    /usr/sbin/i2cset -y 4 0x71 0x0f

    # Do the I2C bus scan
    echo "Scanning the I2C bus..."
    i2c_scan=$(/usr/sbin/i2cdetect -y -r 4)
    echo "$i2c_scan"

    local result=0
    failure_msg=""

    for address in 08 50 71 68 76 44 6b 3e 51
    do
        if echo "$i2c_scan" | grep " $address " > /dev/null
        then
            echo "PASS: Detected I2C address $address"
        else
            echo "FAIL: I2C address $address missing!"
            result=1
        fi
    done

    # This is to stop scary error messages from appearing when Legato starts.
    /bin/touch /etc/resolv.conf

    # Restart legato
    /legato/systems/current/bin/legato start
    sleep 10

    return $result
}


#=== FUNCTION ==================================================================
#
#        NAME: self_test
# DESCRIPTION: Perform the automated self tests implemented by the yellowtest command
#   PARAMETER: None
#
#   RETURNS 1: 0 Success
#   RETURNS 2: 1 Failure
#
#===============================================================================
self_test() {

    echo "Starting automated tests daemon..."

    /legato/systems/current/bin/app start YellowTestService

    echo "Running Automated Tests..."

    if /legato/systems/current/bin/yellowtest $BOARD_REV
    then
        failure_msg=""
        test_result="PASSED"
        return 0
    else
        failure_msg="On-device automated tests failed"
    fi

    test_result="FAILED"
    return 1
}

# Main Test
echo '+------------------------------------------------------------------------------+'
echo "|                          mangOH Yellow Test Program                          |"
echo '+------------------------------------------------------------------------------+'

fail_count=0
failure_msg=""
test_result=""

/legato/systems/current/bin/app start spiService

# automation test
echo "=== Start Wifi testing ==="
if ! yellowTest_WifiScan
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
else
    echo "$test_result"
fi
echo '======================================================================='

echo "=== Start USB testing ==="
if ! yellowTest_USB
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
else
    echo "$test_result"
fi
echo '======================================================================='

echo "=== Start self-testing ==="
if ! self_test
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
else
    echo "$test_result"
fi
echo '======================================================================='

# echo "=== Start uSD Read Write testing ==="
# if ! yellowTest_uSD
# then
#   fail_count=$(($fail_count + 1))
#   echo "----->               FAILURE           <-----"
#   echo "$failure_msg"
# else
#   echo "$test_result"
# fi
# echo '======================================================================='

echo "=== Start I2C testing ==="
if ! yellowTest_I2CDetect
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
else
    echo "$test_result"
fi
echo '======================================================================='

# Manual tests start here
echo "=== yellowManualTest_initial ==="
if ! yellowManualTest_initial
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
else
    echo "$test_result"
fi
echo '======================================================================='

echo "=== test_buzzer ==="
if ! test_buzzer
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
    echo "--------------------------------------------"
else
    echo "$test_result"
fi

echo "=== test_light_sensor ==="
if ! test_light_sensor
then
    fail_count=$(($fail_count + 1))
    echo "----->               FAILURE           <-----"
    echo "$failure_msg"
else
    echo "$test_result"
fi
echo '======================================================================='


# export test result
echo '-----------------------------------------------------------------------'
if [ $fail_count -ne 0 ]
then
    echo "FAILURE: $fail_count tests failed"
fi
echo ""

exit $fail_count
