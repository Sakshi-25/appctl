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
