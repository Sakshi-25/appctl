#ifndef __DEFAULT_HH__
#define __DEFAULT_HH__

#define CONFIG_FILE                 "/etc/appctl.conf"
#define ROOT_DIR                    "/"

#define CACHE_DIR       ROOT_DIR    "var/cache/appctl/"
#define WORK_DIR        CACHE_DIR   "work/"
#define SOURCE_DIR      CACHE_DIR   "sources/"
#define PACKAGES_DIR    CACHE_DIR   "packages/"

#define RECIPES_DIR     CACHE_DIR   "recipes/"
#define DATA_DIR        ROOT_DIR    "var/lib/app/index/"
#define APPS_DIR        ROOT_DIR    "apps/"
#define LIBEXEC_DIR     ROOT_DIR    "usr/libexec/appctl/"

#define SPEC_FILE       LIBEXEC_DIR "spec.sh"

#define URL_SOURCE      "https://raw.githubusercontent.com/rlxos/"
#define URL_BINARY      "https://manjeet.cloudtb.online/apps"

#define DEFAULT_REPO    "core apps"
#define _VERSION         "0.6.b"

#define MODULES_RECIPE  LIBEXEC_DIR "modules/librlxpkg"

#endif