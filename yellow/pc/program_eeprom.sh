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

    msg=$(echo "" &&
          echo "1. Confirm power switch is closest to corner of the board" &&
          echo "2. Connect 'USB' port on unit to PC USB port")

    prompt_enter "$msg"

    WaitForDevice "Up" "$rbTimer"

    #remove and generate ssh key
    ssh-keygen -R $TARGET_IP

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
#    if ! test_reset_button
#    then
#        TEST_RESULT="f"
#        return 1
#    fi

#    WaitForDevice "Up" "$rbTimer"
}

target_cleanup() {

    echo -e "${COLOR_TITLE}Restoring Legato${COLOR_RESET}"
    if ! RestoreGoldenLegato
    then
        TEST_RESULT="f"
        echo -e "${COLOR_ERROR}Failed to restore Legato to Golden state${COLOR_RESET}"
    fi

    prompt_enter "Unplug the USB cable"
}

program_eeprom()
{
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

    WaitForFileToExist "$eeprom_path" || return 1

    local time_str=$(date +"%Y-%m-%d-%H:%M")

    local msg="mangOH Yellow\nRev: $BOARD_REV\nDate: $time_str\nMfg: $MANUFACTURER\n\0"

    if SshToTarget "printf '$msg' > $eeprom_path"
    then
        if SshToTarget "cat '$eeprom_path' | grep 'mangOH Yellow'" > /dev/null
        then
            return 0
        fi
    fi

    echo "Rebooting to make EEPROM writeable..."
    SshToTarget "/sbin/reboot"
    sleep 10
    WaitForDevice "Up" "$rbTimer" || return 1
    WaitForFileToExist "$eeprom_path" || return 1

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

run_programming()
{
    if ! target_setup
    then
        TEST_RESULT="f"
        echo -e "${COLOR_ERROR}Failed to setup target${COLOR_RESET}"

    elif ! program_eeprom
    then
        TEST_RESULT="f"
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
    TEST_RESULT="p"

    run_programming

    if [ "$TEST_RESULT" = "p" ]
    then
        printf "\nFinal result: [PASSED]\n"
    else
        printf "\nFinal result: [FAILED]\n"
    fi

    if ! prompt_yes_no "More units to program?"
    then
        exit 0
    fi
done
