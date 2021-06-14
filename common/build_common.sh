#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
. ../setting/common_setting.sh
if [ "$PROJECT_COPY" = "true" ]; then
. ../common/copy_src.sh
fi
cd built
$RUN_GENERATOR
cd ..
. ../setting/unset_setting.sh