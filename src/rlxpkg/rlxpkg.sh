_test_func() {
    echo "$1"
}

interrupted() {
    echo "interrupted detected"
    exit 5
}

APPCTL_SPECS="/etc/appctl.specs.sh"

[[ -f $APPCTL_SPECS ]] && . $APPCTL_SPECS

trap "interrupted" 1 2 3 15

# rlxpkg_download <rcp> <path>
# download every source file from <source> to <path>
# ENVIRONMENT:          REDOWNLOAD: redownload
#                       WGET_ARGS:  wget args
#
# Error Code:           5: invalid argumets
#                       6: failed to load recipe file
#                       7: failed to download source file
#                       8: failed to check downloaded files
rlxpkg_download() {

    [[ -z "${2}" ]] && return 5

    local rcp="${1}"
    local path="${2}"
    
    _debug() {
        [[ -z $DEBUG ]] && return
        echo "Debug: $@"
    }

    . ${rcp} || return 6

    for s in ${source[@]} ; do
        if echo $s | grep -Eq '::(http|https|ftp)://' ; then
            local filename=$(echo $s | awk -F '::' '{print $1}')
            local url=$(echo $s | awk -F '::' '{print $2}')
        else
            local filename="$(basename $s)"
            local url="${s}"
        fi

        if [[ "${filename}" != "${s}" ]] ; then
         if [[ ! -e "${path}/${filename}" ]] ; then
             wget -c --passive-ftp --no-directories --tries=3 --waitretry=3 --output-document="${path}/${filename}.part" "${url}" ${WGET_ARGS}
            if [[ $? != 0 ]] ; then
                _debug "failed to download ${filename} from ${url}"
                return 7
            fi
            mv ${path}/${filename}{.part,}
         fi
        fi
    done
    return 0
}

# rlxpkg_prepare <rcp_file> <spath> <path>
# prepare source files
# 
# Error Codes:              5: invalid arguments
#                           6: failed to load recipe file
#                           7: specified source file missing
#                           8: failed to prepare source file 
#                           9: failed to copying file
rlxpkg_prepare() {
    [[ -z "${3}" ]] && return 5

    local rcpfile="${1}"
    local spath="${2}"
    local path="${3}"
    local _rcp_dir=$(dirname $rcpfile)
    
    [[ ! -d ${path} ]] && mkdir -p $path

    . $rcpfile || return 6

    _debug() {
        [[ -z "$DEBUG" ]] && return
        echo "rlxpkg_install: $@"
    }


    for s in ${source[@]} ; do
        if echo $s | grep -Eq '::(http|https|ftp)://' ; then
            local filename=${spath}/$(echo $s | awk -F '::' '{print $1}')
        elif echo $s | grep -Eq '^(http|https|ftp)://' ; then
            local filename=${spath}/$(basename $s)
        else
            local filename=${_rcp_dir}/$(basename $s)
        fi

        for noext in ${noextract} ; do
            if [[ "$noext" = "$(basename ${filename})" ]] ; then
                nxt=1
                break
            fi
        done

        [[ ! -f "${filename}" ]] && {
            echo "${filename} is missing"
            return 7
        }

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
                    _debug "extracting $(basename $filename)"
                    tar -C "${path}" -${CMS} -xf "${filename}"
                    ;;
                
                *)
                    _debug "copying $(basename $filename)"
                    cp "${filename}" "${path}"
                    ;;
            esac
            if [[ "$?" != 0 ]] ; then
                return 8
            fi
        else
            _debug "copying $(basename $filename)"
            cp "${filename}" "${path}"
            if [[ "$?" != 0 ]] ; then
                return 9
            fi
        fi

    done
    return 0   
}



# rlxpkg_build <rcp_file> <src> <pkg>
# execute function build
# Error Code:               5: invalid arguments
#                           6: failed to load source file
#                           7: failed to compile source
rlxpkg_build() {
        
    [[ -z "${3}" ]] && return 5
    local _rcp_file="${1}"
    local src=${2}
    local pkg=${3}

    . $_rcp_file || return 6

    export src
    export pkg

    cd $src >/dev/null

    (set -e -x; build 2>&1)
    if [[ $? != 0 ]] ; then
        return 7
    fi

    return 0
}


# rlxpkg_genpkg <rcp_file> <spkg> <pkgdir>
# ENVIRONMENT:              COMPRESS_ALGO: compression algo
#                           BACKUP: file to take backup
#
# Error Code:               5: invalid args
#                           6: failed to source recipe file
#                           7: failed to compress package
rlxpkg_genpkg() {
    [[ -z "${3}" ]] && return 5

    local _rcp_file="${1}"
    local pkg="${2}"
    local pkgdir="${3}"
    
    local rcp_dir="$(dirname $_rcp_file)"

    COMPRESS_ALGO=${COMPRESS_ALOG:-"zstd"}

    . $_rcp_file || return 6

    local pkgfile="${name}-${version}-${release}-$(uname -m).rlx"

    cd $pkg &>/dev/null

    desc=$(cat "${_rcp_file}" | grep '# Description: ' | sed 's|# Description: ||g')
    _dps=$(cat "${_rcp_file}" | grep '# Runtime: ' | sed 's|# Runtime: ||g')

    mkdir -p .data
    echo "name: $name
version: $version
release: $release
description: $desc
size: $(du -hs | cut -f1)
depends: $_dps" > .data/info

    for _i in install remove update usrgrp data ; do
        [[ -f "$rcp_dir/$_i" ]] && cp "$rcp_dir/$i" .data/
    done

    # backup conf
    for _f in ${BACKUP} ; do
        mv "${_f}" "${_f}.new"
    done

    tar -cf "${pkgdir}/${pkgfile}" --"${COMPRESS_ALGO}" * .data || return  7

    return 0
}


# strip_package <dir> <rcp_file>
# strip binaries, libaries and clean empty directories etc
# ENVIRONMENT:
#                   NO_STRIP:   list of files to skip stripping
#                   CROSS_COMPILE: cross-compiler if useds
#                   DEBUG: debug 
#                   REMOVE_DOCS: remove docs
#   
# Error Codes:
#                   5: invalid arguments
#                   6: failed to source recipe file                
strip_package() {
    # taken for scartchpkg https://github.com/venomlinux/scratchpkg/blob/master/pkgbuild
    [[ -z "${2}" ]] && return 5
    local _dir=${1}
    local _rcpfile=${2}

    _debug() {
        [[ -z $DEBUG ]] && return
        echo "Debug: $@"
    }

    source $_rcpfile || return 6
    
    cd "$_dir" &>/dev/null

    if [ "$NO_STRIP" ]; then
		for i in $NO_STRIP; do
			xstrip="$xstrip -e $i"
		done
		FILTER="grep -v $xstrip"
	else
		FILTER="cat"
	fi
			
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
				${CROSS_COMPILE}strip --strip-all "$binary" 2>/dev/null ;;
			*)
				continue ;;
		esac
	done

    # compress manpages

    find . -type f -path "*/man/man*/*" | while read -r file; do
		if [ "$file" = "${file%%.gz}" ]; then
			gzip -9 -f "$file"
		fi
	done
	find . -type l -path "*/man/man*/*" | while read -r file; do
		FILE="${file%%.gz}.gz"
		TARGET="$(readlink $file)"
		TARGET="${TARGET##*/}"
		TARGET="${TARGET%%.gz}.gz"
		DIR=$(dirname "$FILE")
		rm -f $file
		if [ -e "$DIR/$TARGET" ]; then
			ln -sf $TARGET $FILE
		fi
	done
	if [ -d usr/share/info ]; then
		(cd usr/share/info
			for file in $(find . -type f); do
				if [ "$file" = "${file%%.gz}" ]; then
					gzip -9 "$file"
				fi
			done
		)
	fi

    if [[ "${REMOVE_DOCS}" ]] ; then
        _debug "removing doc file"
        for i in doc gtk-doc info ; do
            rm -rf usr/share/${i} usr/${i} usr/local/${i} usr/local/share/${i}
        done
    fi

}


# rlxpkg_install <pkgfile>
# install <pkgfile> in ROOT_DIR
# ENVIRONMENT:
#                   ROOT_DIR:      install roots
#                   SKIP_EXECS:    skip pre post executions scripts of installations
#                   DATA_DIR:      data dir
#                   COMPRESS_ALOG: compression algo
#
# Error Code:       5: invalid args
#                   6: pkgfile not found
#                   7: invalid package
#                   8: information data missing
#                   9: failed to execute pre install script
rlxpkg_install() {
    [[ -z "${1}" ]] && return 5

    local pkgfile="${1}"
    local COMPRESS_ALGO=${COMPRESS_ALGO:-"zstd"}
    local WORK_DIR=${WORK_DIR:-'/var/cache/'}
    local ROOT_DIR=${ROOT_DIR:-'/'}
    local DATA_DIR=${DATA_DIR:-"/var/lib/app/index/"}
    local SKIP_EXECS=${SKIP_EXECS:-1}
    local DEBUG=0

    _debug() {
        [[ "$DEBUG" ]] && return
        echo "rlxpkg_install: $@"
    }

    [[ ! -f "${pkgfile}" ]] && {
        _debug "${pkgfile} not exist"
        return 6
    }

    [[ -d "${WORK_DIR}" ]] && rm -r "${WORK_DIR}"
    mkdir -p "${WORK_DIR}"
    
    tar -xf "${pkgfile}" --"${COMPRESS_ALOG}" -C "$WORK_DIR/" .data/info &>/dev/null || {
        return 7
    }

    _read_data() {
        cat "${WORK_DIR}/.data/info" 2>/dev/null | grep "^${1}:" | awk -F ': ' '{print $2}'
    }

    name=$(_read_data "name")
    version=$(_read_data "version")
    release=$(_read_data "release")


    if [[ -z "$name" ]] || [[ -z "$version" ]] || [[ -z "$release" ]] ; then
        _data=$(cat "${WORK_DIR}/.data/info")
        _debug "${pkgfile} is not a valid rlxpkg, meta data is invalid"
        return 8
    fi

    _execute_script() {
        if [[ "${ROOT_DIR}" == "/" ]] ; then
            _xectr="bash"
        else
            _xectr="xchroot '${ROOT_DIR}'"
        fi

        _xectr $@
    }

    tar -xf "${pkgfile}" --"${COMPRESS_ALGO}" -C "${WORK_DIR}/" .data/install &>/dev/null && {
        $_xectr "${WORK_DIR}/.data/install" "pre" "$version" "$release"
        if [[ $? != 0 ]] ; then
            _debug "failed to execute pre install script"
            return 9
        fi
    }

    # TODO add option to check conficting files
    
    [[ ! -d "${ROOT_DIR}" ]] && mkdir -p "${ROOT_DIR}"


    tar --keep-directory-symlink -pxvf "${pkgfile}" --"${COMPRESS_ALGO}" -C "${ROOT_DIR}/" | while read -r line ; do
        [[ "${line:0:1}" == "." ]] && {
            _debug "skipping ${line} hidden data"
            continue
        }
        if [[ "$line" == "${line%.*}.new" ]] ; then
            line="${line%.*}"
            mv "${ROOT_DIR}/${line}.new" "${ROOT_DIR}/${line}"
        fi
        echo "$line" >> "${WORK_DIR}/${name}.files"
    done

    
    [[ -d ${DATA_DIR} ]] || mkdir -p "$DATA_DIR"
    rm -rf ${DATA_DIR}/${name}

    tar -xf "${pkgfile}" -C $WORK_DIR .data
    mv $WORK_DIR/.data "$DATA_DIR/${name}"

    mv "$WORK_DIR/${name}.files" "$DATA_DIR/${name}/files"

    echo -e "\ninstalled: $(date +'%I:%M:%S %p %D:%m:%Y')" >> "$DATA_DIR/${name}/info"

    tar -xf "${pkgfile}" --"${COMPRESS_ALGO}" -C "${WORK_DIR}/" .data/install &>/dev/null && {
        $_xectr "${WORK_DIR}/.data/install" "post" "$version" "$release"
        if [[ $? != 0 ]] ; then
            _debug "failed to execute post install script"
            return 9
        fi
    }

    if [[ -x "${ROOT_DIR}/usr/sbin/ldconfig" ]] ; then
        "$ROOT_DIR/"usr/sbin/ldconfig "${ROOT_DIR}/" &>/dev/null || return 0
    fi

    return 0
}

# rlxpkg_remove <app_id>
# remove app from system
# ENVIRONMENT:          ROOT_DIR:   root directory
#                       DATA_DIR:   data base dir
#                       SKIP_EXEC:  to skip pre or post removal expr
rlxpkg_remove() {
    [[ -z "${1}" ]] && return 101

    local _app_id="${1}"

    DATA_DIR=${DATA_DIR:-'/var/lib/app/index'}
    [[ ! -f "${DATA_DIR}/${_app_id}/info" ]] && {
        return 15
    }

    _read_info() {
        cat "${DATA_DIR}/${_app_id}/info" 2>/dev/null | grep "^${1}:" | awk -F ': ' '{print $2}'
    }

    _name=$(_read_info 'name')    
    _ver=$(_read_info 'version')
    _rel=$(_read_info 'release')

    [[ ! -f "${DATA_DIR}/${_app_id}/files" ]] && {
        return 16
    }

    if [[ "${ROOT_DIR}" == '/' ]] ; then
        _xectr="bash"
    else
        _xectr="xchroot ${ROOT_DIR}"
    fi

    if [[ -f "${DATA_DIR}/${_app_id}/remove" ]] ; then
        (cd "${ROOT_DIR}"; $_xectr "${DATA_DIR}/${_app_id}/remove" "pre" "$_name" "$_ver" "$_rel")
    fi

    for i in $(cat ${DATA_DIR}/${_app_id}/files) ; do
        rm -f "${ROOT_DIR}/${i}" &>/dev/null
    done

    if [[ -f "${DATA_DIR}/${_app_id}/remove" ]] ; then
        (cd "${ROOT_DIR}"; $_xectr "${DATA_DIR}/${_app_id}/remove" "post" "$_name" "$_ver" "$_rel")
    fi

    rm -r "${DATA_DIR}/${_name}"

}
