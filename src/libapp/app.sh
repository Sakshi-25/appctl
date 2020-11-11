
debug() {
    [[ -z $DEBUG ]] && return
    echo -e "\n\033[1;34mDebug\033[0m\033[1m: $@\033[0m"
}

CONFIG_FILE=${CONFIG_FILE:-"/etc/appctl.conf"}

[[ -f "${CONFIG_FILE}" ]] || CONFIG_FILE="/usr/etc/appctl.conf"

[[ ! -f "${CONFIG_FILE}" ]] && {
    debug "$CONFIG_FILE not exist"
    exit 99
}


ERR_LOCK_APPCTL_FAILED=22

# read_config <file> <sec> <var> <?default>
# read value from configuration file
read_config() {
    [[ -z "${2}" ]] && return 101

    local file="${1}"
    local sec="${2}"
    local var="${3}"

    val=$(sed -nr "/^\[${sec}\]/ { :l /^${var}[ ]*=/ { s/.*=[ ]*//; p; q;}; n; b l;}" ${file})
    [[ -z ${val} ]] && echo "${4}" || echo "${val}"
}



CACHE_DIR=$(read_config "${CONFIG_FILE}" 'dir' 'cache' '/var/cache/appctl')

# lock_appctl <id>
# lock appctl for other process and get working place
lock_appctl() {
    [[ -d "${CACHE_DIR}" ]] || install -dm755 "${CACHE_DIR}"
    if [[ -f "${CACHE_DIR}/lock" ]] ; then
        return 1
    fi

    WORK_DIR=${WORK_DIR:-"${CACHE_DIR}/work"}
    PKG_DIR=${PKG_DIR:-"${CACHE_DIR}/pkg"}
    SRC_DIR=${SRC_DIR:-"${CACHE_DIR}/src"}

    for i in $WORK_DIR $PKG_DIR $SRC_DIR ; do
        [[ ! -d "$i" ]] && install -dm755 "${i}"
    done

    echo "${1}" > ${CACHE_DIR}/lock
    return 0
}

# unlock_appctl
# unlock appctl and clean working directories
unlock_appctl() {
    [[ ! -f "${CACHE_DIR}/lock" ]] && {
        return 1
    }

    rm -f "${CACHE_DIR}/lock"
    rm -rf "${CACHE_DIR}/work"

    return 0
}

# compress_package <dir> <aout>
# compress the content of <dir> into tar package with name <aout>
# ENVIRONMENT:
#              COMPRESS_ALGO:  tar compression algo
#                             - gzip
#                             - xz
#                             - zstd
#                             - bzip2
#              EXTRA_FILES:    extra files need to be included
#                              e.g. hidden files .data
compress_package() {
    [[ -z "$1" ]] && return 101
    [[ -z "$2" ]] && return 101

    local tar_dir="$1"
    local aout="$2"

    cd "${tar_dir}" >/dev/null

    debug "compressing ${aout}"

    COMPRESS_ALGO=${COMPRESS_ALGO:-"zstd"}
    case $COMPRESS_ALGO in
        gzip|xz|zstd|bzip2)
            ;;

        *) 
            debug "unsupported compression algo specified: ${COMPRESS_ALGO}"
            return 103
            ;;
    esac

    tar -cf "${aout}" --"${COMPRESS_ALGO}" * ${EXTRA_FILES}
    if [[ $? != 0 ]] ; then
        debug "failed to compress ${aout} ${dir} with ${COMPRESSION_ALGO}"
        return 200
    fi

    cd - >/dev/null

}


# extract_package <package> <loc>
# extract tar package into location
# ENVIRONMENT:
#              COMPRESS_ALGO:  tar compression algo
#                             - gzip
#                             - xz
#                             - zstd
#                             - bzip2
extract_package() {
    [[ -z "$1" ]] && return 101
    [[ -z "$2" ]] && return 101

    local tarfile="$1"
    local loc="$2"


    debug "extracting ${tarfile}"

    COMPRESS_ALGO=${COMPRESS_ALGO:-"zstd"}
    case $COMPRESS_ALGO in
        gzip|xz|zstd|bzip2)
            ;;

        *) 
            debug "unsupported compression algo specified: ${COMPRESS_ALGO}"
            return 103
            ;;
    esac

    tar -xf "${tarfile}" --"${COMPRESS_ALGO}" -C ${loc}
    if [[ $? != 0 ]] ; then
        debug "failed to extract ${tarfile}"
        return 200
    fi
}

# strip_package <dir>
# strip all binaries and libraries in <dir>
# ENVIRONMENT: 
#               CROSS_COMPILE: cross-compiler pre
#                              - e.g: x86_64-linux-gnu-
strip_package() {
    [[ -z "$1" ]] && return 101
    [[ -d "$1" ]] || return 101
    cd $1 >/dev/null
    debug "stripping package dir '$1'"

    [[ "$SKIP_EMPTY" ]] || {
        debug "deleting empty directories"
        find . -type d -empty -delete
    }

    [[ "$SKIP_LIBTOOL" ]] || {
        debug "deleting libtool"
        find . ! -type d -name "*.la" -delete
    }


    if [[ -z "$NO_STRIP" ]] ; then
        FILTER="cat"
    else
        for i in "$NO_STRIP" ; do
            xstrip="$xstrip -e $i"
        done
        FILTER="grep -v $xstrip"
    fi

    debug "stripping $name"
    find . -type f -printf "%P\n" 2>/dev/null | $FILTER | while read -r binary ; do
		case "$(file -bi "$binary")" in
			*application/x-sharedlib*)  # Libraries (.so)
				${CROSS_COMPILE}strip --strip-unneeded "$binary" 2>/dev/null ;;
			*application/x-pie-executable*)  # Libraries (.so)
				${CROSS_COMPILE}strip --strip-unneeded "$binary" 2>/dev/null ;;
			*application/x-archive*)    # Libraries (.a)
				${CROSS_COMPILE}strip --strip-debug "$binary" 2>/dev/null ;;
			*application/x-object*)
				case "$binary" in
					*.ko)                   # Kernel module
						${CROSS_COMPILE}strip --strip-unneeded "$binary" 2>/dev/null ;;
					*)
						continue;;
				esac;;
			*application/x-executable*) # Binaries
				strip --strip-all "$binary" 2>/dev/null 
				;;
			*)
				continue ;;
		esac
	done

    cd - >/dev/null
}

