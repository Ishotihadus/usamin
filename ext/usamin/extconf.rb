require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CXX'] = ENV['CXX'] if ENV['CXX']
have_library('stdc++')
dir_config('rapidjson')
append_cppflags('-O3')
append_cppflags('-Wall')
append_cppflags('-Wextra')
append_cppflags('-Wvla')
$CXXFLAGS << ' -std=c++11'
create_makefile('usamin/usamin')
