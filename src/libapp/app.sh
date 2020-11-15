
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

