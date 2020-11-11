#!/bin/bash

APP_SH=${APP_SH:-"src/libapp/app.sh"}
CONFIG_FILE=${CONFIG_FILE:-"test/data/app.conf"}
. $APP_SH

# assert_equal <val1> <val2>
# assertion test for equality
assert_equal() {
    if [[ "${1}" != "${2}" ]] ; then
        echo -e "\033[1;31m ASSERTION FAIL:\033[0;1m '${1}' != '${2}'" 1>&3
        return 1
    fi
    return 0
}

# test_case <mesg> <func>
# test the function for output
test_case() {
    [[ -z "${2}" ]] && return 101

    local mesg="${1}"
    local func="${2}"

    echo -ne "\033[1mTesting ${1}"
    [[ "$DEBUG" ]] && {
        $func
    } || {
        $func &>/dev/null
    }

    rtn="${?}"
    
    if [[ "$rtn" == "0" ]] ; then
        echo -e "\t\t[\033[1;32mPass\033[0;1m]\033[0m"
    else
        echo -e "\t\t[\033[1;31mFail\033[0;1m]\033[0m"
        exit $rtn
    fi
}


test_read_config() {

    local CONFIG_FILE="test/data/test_config.conf"
    # case_1
    # test case for vaild result
    case_1() {
        val=$(read_config "${CONFIG_FILE}" "sec1" "var1" "noval")
        assert_equal "${val}" "sec1_val1"
    }

    test_case "case for valid result case 1" case_1

    case_2() {
        val=$(read_config "${CONFIG_FILE}" "sec2" "var1" "noval")
        assert_equal "${val}" "sec2_val1"
    }

    test_case "case for valid result case 2" case_2

    case_3() {
        val=$(read_config "${CONFIG_FILE}" "sec3" "var1" "default_val")
        assert_equal "${val}" "default_val"
    }

    test_case "case for fallback value    " case_3
}


test_lock_appctl() {

    # verifing valid result
    case_1() {
        CACHE_DIR="test/cache_case1/"
        mkdir -p "${CACHE_DIR}"
        if ! lock_appctl "test_case" &>/dev/null ; then
            debug "failed to lock appctl"
            return 2
        fi
        
        [[ -f $CACHE_DIR/lock ]] || {
            rm -rf "${CACHE_DIR}"
            return 12
        }

        [[ "${WORK_DIR}" == "${CACHE_DIR}/work" ]] || {
            rm -rf "${CACHE_DIR}"
            return 3
        }
        [[ -d "${WORK_DIR}" ]] || {
            rm -rf "${CACHE_DIR}"
            return 4
        }

        [[ "${PKG_DIR}" == "${CACHE_DIR}/pkg"   ]] || {
            rm -rf "${CACHE_DIR}"
            return 5
        }
        [[ -d "${PKG_DIR}"  ]] || {
            rm -rf "${CACHE_DIR}"
            return 6
        }

        [[ "${SRC_DIR}" == "${CACHE_DIR}/src"   ]] || {
            rm -rf "${CACHE_DIR}"
            return 7
        }
        [[ -d "${SRC_DIR}"  ]] || {
            rm -rf "${CACHE_DIR}"
            return 8
        }

        rm -rf "${CACHE_DIR}"
    }

    test_case "for verifying valid result" case_1

    case_2() {
        CACHE_DIR="test/cache_case2"
        mkdir -p "${CACHE_DIR}"
        if ! lock_appctl "test_case_2" &>/dev/null ; then
            debug "failed to lock appctl"
            return 2
        fi

        val=$(cat "${CACHE_DIR}/lock")
        assert_equal "$val" "test_case_2" 

        rm -r "${CACHE_DIR}"
    }

    test_case "for verifying valid lock id" case_2

    case_3() {
        CACHE_DIR="test/cache_case3"
        mkdir -p "${CACHE_DIR}"

        if ! lock_appctl "test_case_2" &>/dev/null ; then
            debug "failed to lock appctl"
            return 2
        fi

        if ! lock_appctl "test_case_3" ; then
            rm -r "${CACHE_DIR}"
            return 0
        else
            rm -r "${CACHE_DIR}"
            return 1
        fi
    }

    test_case "for verifying lock of appctl" case_3

}


test_unlock_appctl() {

    case_1() {
        CACHE_DIR="test/cache_case1"
        mkdir -p "${CACHE_DIR}"
        echo "case_1" > "${CACHE_DIR}/lock"

        if ! unlock_appctl ; then
            return 3
        fi

        [[ -f "${CACHE_DIR}/lock" ]] && return 5
        [[ -d "${CACHE_DIR}/work" ]] && return 6
        return 0
    }

    test_case "for validity of unlock_appctl" case_1

    case_2() {
        if ! unlock_appctl ; then
            return 0
        fi
        return 1
    }

    test_case "for validity of faliure     " case_2

}

test_read_config

test_lock_appctl

test_unlock_appctl