if [ ! -z "${ZSH_VERSION}" ]; then
    BASE_DIR="${0:a:h}"
elif [ ! -z "${BASH_VERSION}" ]; then
    BASE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
else
    echo "MUST BE BASH OR ZSH"
    return
fi

if [ -z "${TIDL_PLSDK}" ]; then
    TIDL_PLSDK="${BASE_DIR}/ti-processor-sdk"
    if [ ! -d "${TIDL_PLSDK}" ]; then
        echo "TIDL_PLSDK is not set and ${TIDL_PLSDK} is missing. Please acquire and install the latest PLSDK and symlink it to ./ti-processor-sdk"
        return 1
    fi
    echo "Set TIDL_PLSDK to ${TIDL_PLSDK}"
    export TIDL_PLSDK
    export PSDK_LINUX="${TIDL_PLSDK}"
    echo "Source ${PSDK_LINUX}/linux-devkit/environment-setup"
    source "${PSDK_LINUX}/linux-devkit/environment-setup"
    for PKGCONFIG in $(find "${PSDK_LINUX}/linux-devkit" -type d -a -name pkgconfig); do
        echo "Add ${PKGCONFIG} to PKG_CONFIG_PATH"
        export PKG_CONFIG_PATH="${PKGCONFIG}:${PKG_CONFIG_PATH}"
    done
fi
