# frozen_string_literal: true

require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CXX'] = ENV['CXX'] if ENV['CXX']
have_library('stdc++')
have_library('m')
dir_config('rapidjson')
append_cppflags('-O3')
append_cppflags('-Wall')
append_cppflags('-Wextra')
append_cppflags('-Wno-missing-field-initializers')
append_cppflags('-Wvla')

if checking_for('whether -march=native is accepted as CPPFLAGS'){try_cppflags('-march=native')}
  if checking_for('whether -msse4.2 is accepted as CPPFLAGS'){try_cppflags('-msse4.2')}
    $CPPFLAGS << ' -msse4.2 -march=native'
  elsif checking_for('whether -msse2 is accepted as CPPFLAGS'){try_cppflags('-msse2')}
    $CPPFLAGS << ' -msse2 -march=native'
  end
end

$CPPFLAGS << ' -std=c++11'

$CPPFLAGS << ' -DRAPIDJSON_IS_HEAD' if arg_config('--rapidjson-is-head')

create_makefile('usamin/usamin')
