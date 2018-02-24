require 'mkmf'

have_library('stdc++')
dir_config('rapidjson')
$CPPFLAGS << ' -O3 -Wall -Wextra -Wvla -std=c++14'
create_makefile('usamin/usamin')
