#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
. ../setting/common_setting.sh
if [ -e built/built/$RUNEXECUTABLE ]; then
  cd built/built
  ./$RUNEXECUTABLE %RUNCMDLINE%
  cd ../..
fi
. ../setting/unset_setting.sh