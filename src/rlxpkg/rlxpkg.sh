#!/bin/bash

APP_SH=${APP_SH:-"/usr/lib/appctl/app.sh"}

debug() {
    [[ -z $DEBUG ]] && return
    echo -e "/033[1;32mDebug/033[0m/033[1m: $@/033[0m"
}

. "${APP_SH}" || {
    debug "failed to source ${APP_SH}"
    exit 111
}

RLX_PKG_CONFIG=${RLX_PKG_CONFIG:-"/etc/rlxpkg.conf"}
[[ -f "${RLX_PKG_CONFIG}" ]] || RLX_PKG_CONFIG="/usr/etc/rlxpkg.conf"
[[ -f "${RLX_PKG_CONFIG}" ]] && . "${RLX_PKG_CONFIG}"


ERR_LOCK_APPCTL_FAILED=66
ERR_RECIPE_VERIFY_FAILED=67
ERR_FAILED_TO_DOWNLOAD=68
ERR_FILE_NOT_EXIST=69
ERR_FAILED_TO_EXTRACT=70
ERR_PREMISSION=71

# read_data <tarfile> <file> <variable>
# read data of <file> for <tarfile> and <variable> (':' seperated value)
# ENVIRONMENT:
#              COMPRESS_ALGO:  tar compression algo
#                             - gzip
#                             - xz
#                             - zstd
#                             - bzip2
#              WORK_DIR:       working directory
#
read_data() {
    [[ -z "${2}" ]] && return 101

    if ! lock_appctl ; then
        exit 64
    fi
    
    local tarfile="${1}"
    local file="${2}"
    local var="${3}"
    tar -C ${WORK_DIR} -xf ${tarfile} --"${COMPRESS_ALGO}" "${file}"

    cat "${WORK_DIR}/${file}" | grep "^${var}:" | awk -F ': ' '{print $2}'

    unlock_appctl
    return 0
}


# verify_pkg <rlxpkg>
# verify is <rlxpkg> is a vaild releax package
# Return:    0 - verification fail
#            1 - verification pass
verify_pkg() {
    local rlxpkg="${1}"
    [[ ! -e "${rlxpkg}" ]] && return 101

    pass=1

    for i in name version release description ; do
        local data=$(read_data ${rlxpkg} '.data/info' "${i}")
        if [[ -z "${data}" ]] ; then
            pass=0
        fi
    done

    return ${pass}
}

# execute_script <tarfile> <file> <args...>
# execute script from tarfile
# Environment:
#                       ROOT_DIR: root directory
#
execute_script() {
    local tarfile="${1}"
    shift

    local file="${2}"
    shift
    
    if ! lock_appctl ; then
        exit 64
    fi

    if tar -C ${WORK_DIR} -tf ${tarfile} --"${COMPRESS_ALGO}" "${file}" >/dev/null 2>&1 ; then
        
        tar -C ${WORK_DIR} -xf ${tarfile} --"${COMPRESS_ALGO}" "${file}"

        ROOT_DIR=${ROOT_DIR:-'/'}
        [[ "$ROOT_DIR" = '/' ]] && executor="bash" || executor="xchroot $ROOT_DIR"

        debug "executing ${file} via ${executor}"
        (cd "${ROOT_DIR}"; ${executor} "${WORK_DIR}/${file}" $@)

    fi

    unlock_appctl

    return 0

}


# rlxpkg_download <source> <path>
# download every source file from <source> to <path>
# ENVIRONMENT:          REDOWNLOAD: redownload
rlxpkg_download() {

    [[ -z "${2}" ]] && return 101

    local sources="${1}"
    local path="${2}"

    for s in ${sources} ; do
        if echo $s | grep -Eq '::(http|https|ftp)://' ; then
            local filename=$(echo $s | awk -F '::' '{print $1}')
            local url=$(echo $s | awk -F '::' '{print $2}')
        else
            local filename=$(basename $s)
            local url="${s}"
        fi

        if [[ "${filename}" != "${s}" ]] ; then
            appctl "download" "${url}" "${path}/${filename}"
            if [[ $? != 0 ]] ; then
                debug "failed to download ${filename}"
                return $ERR_FAILED_TO_DOWNLOAD
            fi
        else
            if [[ ! -f "${filename}" ]] && return $ERR_FILE_NOT_EXIST
        fi
    done
    return 0
}

# rlxpkg_prepare <source> <path>
# prepare source files
rlxpkg_prepare() {
    [[ -z "${2}" ]] && return 101

    local sources="${1}"
    local path="${2}"

    for s in ${sources} ; do
        if echo $s | grep -Eq '::(http|https|ftp)://' ; then
            local filename=$(echo $s | awk -F '::' '{print $1}')
            local url=$(echo $s | awk -F '::' '{print $2}')
        else
            local filename=$(basename $s)
            local url="${s}"
        fi

        for noext in ${noextract} ; do
            if [[ "$noext" = "$(basename ${filename})" ]] ; then
                nxt=1
                break
            fi
        done

        if [[ ! -f "${filename}" ]] && return $ERR_FILE_NOT_EXIST

        if [[ "${filename}" != "${file}" ]] && [[ "${nxt}" != 1 ]] ; then
            case "${filename}" in
                *.tar|*.tar.*|*.tgz|*.tbz2|*.txz|*.zip|*.rpm)
                    case "${filename}" in
                        *bz2|*bzip2)
                            CMS=j
                            ;;
                        
                        *gz)
                            CMS=z
                            ;;

                        *xz)
                            CMS=J
                            ;;
                    esac
                    debug "extracting $(basename $filename)"
                    tar -C "${path}" -${CMS} -xf "${filename}"
                    ;;
                
                *)
                    debug "copying $(basename $filename)"
                    cp "${filename}" "${path}"
                    ;;
            esac
            if [[ "$?" != 0 ]] ; then
                return $ERR_FAILED_TO_EXTRACT
            fi
        else
            debug "copying $(basname $filename)"
            cp "${filename}" "${path}"
            if [[ "$?" != 0 ]] ; then
                return $ERR_FAILED_TO_EXTRACT
            fi
        fi

    done
    return 0   
}



# rlxpkg_build
# execute function build
rlxpkg_build() {
    [[ "$(id -u)" == 0 ]] || return $ERR_PREMISSION
    debug "compiling $name-$version"
    (set -e -x; build 2>&1)
    rtn=$?
    
    cd $src >/dev/null

    if [[ "$rtn" != 0 ]] ; then
        debug "compilation error"
    fi

    cd - >/dev/null

    return $rtn
}

# rlxpkg_compile <name> <version> <release> <pkgfile>
# read recipe file and generate <pkgfile>
# ENVIRONMENT: 
#                       RECIPE_FILE:    recipe file with absolute path
#                       CONFIG_FILE:    configuration file
rlxpkg_compile() {

    [[ -z "${4}" ]] && return 101
    
    local _name="${1}"
    local _ver="${2}"
    local _rel="${3}"
    local pkgfile="${4}"

    RECIPE_FILE=${RECIPE_FILE:-"$PWD/recipe"}

    if [[ ! -f ${RECIPE_FILE} ]] ; then
        debug "failed to find $RECIPE_FILE"
        return 1
    fi

    if ! lock_appctl &>/dev/null ; then
        debug "failed to lock appctl"
        return $ERR_LOCK_APPCTL_FAILED
    fi

    RCP_DIR=$(dirname ${RECIPE_FILE})
    cd "${RCP_DIR}" &>/dev/null

    . recipe || {
        unlock_appctl

        return $ERR_RECIPE_MISSING
    }

    if [[ "$_name" != "$name" ]] || [[ "$_ver" != "$version" ]] || [[ "$_rel" != "$release" ]] ; then
        unlock_appctl
        return $ERR_RECIPE_VERIFY_FAILED
    fi

    src="${WORK_DIR}/${name}/src"
    pkg="${WORK_DIR}/${name}/pkg"

    mkdir -p "${src}" "${pkg}"

    rlxpkg_download "${source}" "${src}"
    rtn=$?
    if [[ $rtn != 0 ]] ; then
        unlock_appctl
        return $rtn
    fi

    rlxpkg_prepare "${source}" "${src}"
    rtn=$?
    if [[ "$rtn" != 0 ]] ; then
        unlock_appctl
        return $rtn
    fi

    rlxpkg_build
    rtn=$?
    if [[ "$rtn" != 0 ]] ; then
        unlock_appctl
        return $rtn
    fi

    [[ $nostrip ]] || {
        strip_package "${pkg}"
    }

    # generate app data
    local _app_data="${pkg}/.data"
    mkdir -pv "${_app_data}"
    
echo "
name: ${name}
version: ${version}
release: ${release}
description: ${description}
" > "${_app_data}/info"
    

    for i in install update remove usrgrp data ; do
        [[ -f $RCP_DIR/$i ]] && cp $RCP_DIR/$i ${_app_data}/$i
    done


    EXTRA_FILES=" $pkg/.data" \
    compress_package "${pkg}" "${pkgfile}"
    rtn=$?
    if [[ "$rtn" != 0 ]] ; then
        unlock_appctl
        return $rtn
    fi

    cd - &>/dev/null

    unlock_appctl
    return 0
}



# rlxpkg_install <pkgfile>
# install <pkgfile> in ROOT_DIR
# ENVIRONMENT:
#                   ROOT_DIR:  install roots
#                   SKIP_EXECS: skip pre post executions scripts of installations
rlxpkg_install() {
    [[ -z "${1}" ]] && return 101

    local pkgfile="${1}"

    [[ ! -f "${pkgfile}" ]] && {
        debug "${pkgfile} not exist"
        return $ERR_FILE_NOT_EXIST
    }

    if ! lock_appctl ; then
        debug "failed to lock appctl"
        return $ERR_LOCK_APPCTL_FAILED
    fi

    name=$(read_data ${pkgfile} ".data/info" "name")
    version=$(read_data ${pkgfile} ".data/info" "version")
    release=$(read_data ${pkgfile} ".data/info" "release")

    [[ "$SKIP_EXECS" ]] || execute_script "${pkgfile}" ".data/install" "pre" "$version" "$release"
    rtn=$?
    if [[ $rtn != 0 ]] ; then
        debug "failed to execute pre install script"
        unlock_appctl
        return $rtn
    fi

    # TODO add option to check conficting files

    tar --keep-directory-symlink -pxf "${pkgfile}" --"${COMPRESS_ALGO}" -C "${ROOT_DIR}" | while read -r line ; do
        if [[ "$line" = "${line%.*}.new" ]] ; then
            line="${line%.*}"

            mv "${ROOT_DIR}/${line}.new" "${ROOT_DIR}/${line}"
        fi
        debug "extracted $line"
        echo "$line" >> $WORK_DIR/$name.files
    done

    local _data_dir=$(read_config "${CONFIG_FILE}" "dir" "data")

    rm -rf $_data_dir/$name

    tar -xf "${pkgfile}" -C $WORK_DIR .data
    mv $WORK_DIR/.data $_data_dir/${name}

    mv $WORK_DIR/${name}.files $_data_dir/${name}/files

    echo -e "\ninstalled on: $(date +'%I:%M:%S %p %D:%m:%Y')" >> $_data_dir/${name}/info

    [[ "$SKIP_EXECS" ]] || execute_script "${pkgfile}" ".data/install" "post" "$version" "$release"

    if [[ -x "${ROOT_DIR}/sbin/ldconfig" ]] ; then
        "$ROOT_DIR/sbin/ldconfig" -r "${ROOT_DIR}/"
    fi

    unlock_appctl
    return 0
}