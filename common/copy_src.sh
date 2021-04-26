#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
. ../setting/common_setting.sh
mkdir pre_src
cp -p -r $PROJECT_BASE* ./pre_src
rm -r -f src/
mv pre_src/ src/
