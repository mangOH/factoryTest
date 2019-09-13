#!/bin/bash

# Clean up background processes and exit immediately on SIGHUP, SIGINT or SIGTERM.
trap 'printf "\nINTERRUPT RECEIVED. EXITING.\n" ; kill $console_cat_pid ; kill $log_monitor_pid ; exit 1' 1 2 15

COLOR_TITLE=""
COLOR_ERROR=""
COLOR_WARN=""
COLOR_PASS=""
COLOR_RESET=""

# Enable colors if stdout is a tty
#if [ -t 1 ]; then
#    COLOR_TITLE="\\033[1;94m"
#    COLOR_ERROR="\\033[0;31m"
#    COLOR_WARN="\\033[0;93m"
#    COLOR_PASS="\\033[0;32m"
#    COLOR_RESET="\\033[0m"
#fi

# Load configuration settings
source ./configuration.cfg

# Load utility functions
source ./lib/common.sh

# Find the PIDs of the child processes of a given PID.
find_children()
{
    ps -f | awk "\$3 == $1 { print \$2 }"
}

target_setup() {

    # 1. Plug in SIM, microSD card, IoT test card, and expansion-connector test board;
    # 2. Connect power jumper;
    # 3. Confirm "battery protect" switch is ON (preventing the device from booting on battery power);
    # 4. Connect battery;
    # 5. Switch "battery protect" switch OFF (allowing the device to boot on battery power);

    prompt_enter "Confirm power switch is closest to corner of the board"

    prompt=$(echo "Plug in"
             echo " 1. SIM"
             echo " 2. IoT test card"
             echo " 3. Battery"
             echo " 4. Power jumper (on pins closest to edge of board)"
             echo "Then switch power switch (away from corner of the board)")
    prompt_enter "$prompt"

    # Record this as the start time of the test.
    run_time=$(date +"%Y-%m-%d-%H:%M:%S")

    if ! prompt_yes_no "Did the hardware-controlled LED turn green or yellow?"
    then
        echo "Hardware-controlled LED didn't go green/yellow."
        failure_msg="hardware-controlled LED or power switch has a problem"
        test_result="f"
        return 1
    fi

    prompt_enter "Connect unit to PC USB ports (CON first, then USB)"

    (stty 115200 -echo -echonl && cat) < /dev/ttyUSB0 > console.log &
    console_bash_pid=$!
    sleep 1
    console_cat_pid=$(find_children $console_bash_pid)
    echo "Console capture process IDs: $console_bash_pid $console_cat_pid"

    echo ""
    echo "========================================================"
    echo "**** Programming and automated testing starting now ****"
    echo "========================================================"
    echo "This will take a long time."
    echo ""

    WaitForDevice "Up" "$rbTimer"

    #remove and generate ssh key
    ssh-keygen -R $TARGET_IP

    # Check connection, log modem info, and stop the blinking of the software-controlled LED
    if ! SshToTarget "/legato/systems/current/bin/cm info"
    then
        echo "Failed to get cellular modem info from device."
        return 1
    fi

    # Install .spk
    # install by swiflash is faster than fwupdate download
    echo -e "${COLOR_TITLE}Flash Image${COLOR_RESET}"
    swiflash -m "$TARGET_TYPE" -i "./firmware/yellow_final_$TARGET_TYPE.spk"
    WaitForDevice "Up" "$rbTimer"
    WaitForProcessToExist updateDaemon

    # Stop the board from blinking.
    if ! SshToTarget "/legato/systems/current/bin/config set helloYellow:/enableInstantGrat false bool"
    then
        echo "Failed to disable blinking lights on device."
        return 1
    fi

    # Switch to the external SIM slot.
    if ! SshToTarget '/bin/echo > /dev/ttyAT && /bin/echo "at!uims=0" > /dev/ttyAT'
    then
        echo "Failed to switch the device to external SIM slot."
        return 1
    fi

    # install system
    testingSysIndex=$(($(GetCurrentSystemIndex) + 1))
    echo -e "${COLOR_TITLE}Installing testing system${COLOR_RESET}"
    if ! cat "./system/yellow_factory_test.$TARGET_TYPE.update" | SshToTarget "/legato/systems/current/bin/update" > /dev/null
    then
        echo "Failed to load test software system onto the device."
        return 1
    fi
    WaitForSystemToStart $testingSysIndex

    # We need to do the reset button test here because a bunch of stuff doesn't work
    # until a hardware reset happens, for some reason.  This needs to be fixed in the
    # on-board software so that this reset is not necessary.
    # NOTE: It may be possible to avoid having the human press the reset button here if we
    #       assert GPIO 6 (to do a system reset, hard resetting everything outside the module)
    #       followed by a reboot.  But, this idea has not yet been tested.
    if ! test_reset_button
    then
        TEST_RESULT="f"
        return 1
    fi

    WaitForDevice "Up" "$rbTimer"

    # create test folder
    echo -e "${COLOR_TITLE}Creating testing folder${COLOR_RESET}"
    SshToTarget "mkdir -p /tmp/yellow_testing"

    # push test script to the device under test and run it.
    echo -e "${COLOR_TITLE}Pushing test scripts${COLOR_RESET}"
    ScpToTarget "./configuration.cfg" "/tmp/yellow_testing/"
    ScpToTarget "./test_scripts/yellow_test.sh" "/tmp/yellow_testing/"

    imei="$(SshToTarget "/legato/systems/current/bin/cm info imei")"

    echo "Fetched device IMEI: $imei"

    return 0
}

test_reset_button() {

    prompt_enter "Press Reset button"

    if ! prompt_yes_no "Did the hardware-controlled LED go green or yellow?"
    then
        failure_msg="Reset button has a problem"
        echo "$failure_msg"
        test_result="f"
        return 1
    fi

    return 0
}

target_self_test() {

    SshToTarget "/bin/ash /tmp/yellow_testing/yellow_test.sh 2>&1"
}

target_cleanup() {

    echo -e "${COLOR_TITLE}Get System Log ${COLOR_RESET}"
    GetSysLog $imei $run_time
    echo -e "${COLOR_TITLE}Restoring target${COLOR_RESET}"

    # restore golden legato
    echo -e "${COLOR_TITLE}Restoring Legato${COLOR_RESET}"
    if ! RestoreGoldenLegato
    then
        TEST_RESULT="f"
        echo -e "${COLOR_ERROR}Failed to restore Legato to Golden state${COLOR_RESET}"
    fi

    echo -e "${COLOR_TITLE}Test is finished${COLOR_RESET}"

    # Stop recording the console output.
    echo "Killing console logging processes."
    kill $console_bash_pid
    kill $console_cat_pid

    prompt_enter "Unplug the USB cables and switch the power switch (closer to the corner of the board)"
    prompt_enter "Remove the battery, SIM and IoT card (leave power jumper installed)"
}

program_eeprom () {

    if [ "$TEST_RESULT" != "p" ]
    then
        return 1
    fi

    local eeprom_path=""

    echo -e "${COLOR_TITLE}Programming EEPROM${COLOR_RESET}"

    if [ "$TARGET_TYPE" = "wp85" ]
    then
        local eeprom_path="/sys/bus/i2c/devices/0-0050/eeprom"
    else
        if [ "$TARGET_TYPE" = "wp76xx" -o "$TARGET_TYPE" = "wp77xx" ]
        then
            local eeprom_path="/sys/bus/i2c/devices/4-0050/eeprom"
        fi
    fi

    local time_str=$(date +"%Y-%m-%d-%H:%M")

    local msg="mangOH Yellow\nRev: $BOARD_REV\nDate: $time_str\nMfg: $MANUFACTURER\n\0"

    if SshToTarget "printf '$msg' > $eeprom_path"
    then
        if SshToTarget "cat '$eeprom_path' | grep 'mangOH Yellow'" > /dev/null
        then
            return 0
        fi
    fi

    echo "Failed to program EEPROM!"

    return 1
}

run_test()
{
    if ! target_setup
    then
        TEST_RESULT="f"
        echo -e "${COLOR_ERROR}Failed to setup target${COLOR_RESET}"

    elif ! target_self_test
    then
        TEST_RESULT="f"
        echo -e "${COLOR_ERROR}Testing Failed${COLOR_RESET}"
    else
        if ! program_eeprom
        then
            TEST_RESULT="f"
        fi
    fi

    if ! target_cleanup
    then
        TEST_RESULT="f"
        echo -e "${COLOR_ERROR}Failed to cleanup target${COLOR_RESET}"
    fi
}

prompt_enter "Make sure your PC system clock is set to the correct time"

while true
do
    run_time=
    imei=
    log_monitor_pid=
    console_cat_pid=
    console_bash_pid=

    TEST_RESULT="p"

    # Create a log file and start a background process to copy its contents
    # to stdout.  This will be what the human tester actually sees while the
    # test is running. All of the test output goes through this file so we
    # can save it to the results directory after the test completes.
    touch test.log
    tail -f test.log &
    log_monitor_pid=$!
    echo "Log monitoring process ID: $log_monitor_pid"

    run_test > test.log 2>&1

    echo "Killing log monitoring process"
    kill $log_monitor_pid

    if [ "$TEST_RESULT" = "p" ]
    then
        printf "\nFinal result: [PASSED]\n" >> test.log
    else
        printf "\nFinal result: [FAILED]\n" >> test.log
    fi

    # Move the test log and the console log into the results directory.
    mv test.log "results/$imei/testlog_$run_time"
    mv console.log "results/$imei/console_$run_time"

    if [ "$TEST_RESULT" = "p" ]
    then
        printf "\nFinal result: [PASSED]\n"
    else
        printf "\nFinal result: [FAILED]\n"
    fi

    if ! prompt_yes_no "More units to test?"
    then
        exit 0
    fi
done
