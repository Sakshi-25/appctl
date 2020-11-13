#!/bin/bash

. test/.common.sh
APP_SH=${APP_SH:-"src/libapp/app.sh"}
CONFIG_FILE=${CONFIG_FILE:-"test/data/app.conf"}
RLX_PKG_CONIFG=${RLX_PKG_CONIFG:-"test/data/rlxpkg.conf"}

. src/rlxpkg/rlxpkg.sh

# create_wdir <id>
create_wdir() {
    _w_dir="$PWD/test/cache/_${1}"
    [[ -d $_w_dir ]] && rm -r $_w_dir
    mkdir -p "${_w_dir}"
    echo $_w_dir
}

prepare_case() {
    # prepare data
    
    _w_dir=$(create_wdir case_1)

    cd $_w_dir &>/dev/null

    mkdir -p tardir/{bin,.data}
    echo -e "name: var1\nversion: var2\n" > tardir/.data/info
    echo -e '#!/bin/bash\n echo "prescript $@"' > tardir/.data/install
    
    pushd tardir &>/dev/null
    tar -cf ../tarfile.tar.zstd --zstd * .data
    popd &>/dev/null
    rm -r tardir

}

test_read_data() {

    

    case_1() {

        prepare_case

        _name=$(read_data tarfile.tar.zstd .data/info name)
        _ver=$(read_data tarfile.tar.zstd .data/info version)

        cd - &>/dev/null
        rm -r $_w_dir
        assert_equal $_name "var1" || {
            return 1
        }

        assert_equal $_ver "var2" || {
            return 1
        }

        assert_equal $_ver "var3" && {
            return 1
        }

        return 0
    }

    test_case "for valid read_data     " case_1


    case_2() {
        prepare_case

        _name=$(read_data tarfile.tar.zstd .no_folder/im_not_exist no_var)
        cd - &>/dev/null
        rm -r $_w_dir

        if [[ "$?" != 0 ]] ; then
            return 0
        fi

        assert_equal $_name ""

        
    }

    test_case "for non existing file   " case_2
}


test_execute_script() {

    case_1() {
        prepare_case
        
        #ROOT_DIR=$PWD \
        rtn=$(execute_script tarfile.tar.zstd '.data/install' 'hello' 'world')
        cd - &>/dev/null
        rm -r $_w_dir

        echo " rtn: $rtn"
        
        assert_equal "${rtn}" "prescript hello world"
    }

    test_case "test execute_script validity" case_1

    case_2() {
        prepare_case

        execute_script tarfile.tar.zstd 'no_dir/no_file' 'hello' 'world'
        rtn=$?
        cd - &>/dev/null
        rm -r $_w_dir

        if [[ $rtn == 5 ]] ; then
            return 0
        fi
        return 1
    }

    test_case "for non existing file   " case_2
}

test_rlxpkg_download() {

    case_1() {
        _w_dir=$(create_wdir '_1')
        PATH=$PATH:build/ \
        rlxpkg_download "https://releax.in"  $_w_dir/
        _sum=$(sha1sum $_w_dir/releax.in | awk '{print $1}')
        if [[ $? != 0 ]] ; then
            rm -r $_w_dir
            return 1
        fi

        rm -r $_w_dir
        assert_equal "$_sum" "f59f2ca2922e9ea7ee2309ad4f4e2d1c9be49840"
    }

    test_case "for validity of rlxpkg_download" case_1

    case_2() {
        _w_dir=$(create_wdir '_1')
        PATH=$PATH:build/ \
        rlxpkg_download "https://i_am_not_exist/no_me"  $_w_dir/

        if [[ $? != 0 ]] ; then
            rm -r $_w_dir
            return 0
        fi

        rm -r $_w_dir
        return 1
    }

    test_case "for validating invalid url" case_2
}



test_read_data

test_execute_script

test_rlxpkg_download