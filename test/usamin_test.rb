require "test_helper"

class UsaminTest < Minitest::Test
    def test_that_it_has_a_version_number
        refute_nil ::Usamin::VERSION
    end
end
