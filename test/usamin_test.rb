# frozen_string_literal: true

require 'json'
require 'minitest/autorun'
require_relative 'test_helper'

class UsaminTest < Minitest::Test
  def setup
    @data = { str: 'string', int: 3, float: 5.2, array: [0, 'str', false], bool: true, null: nil, object: { key: 'value' } }
    @json = JSON.generate(@data)
  end

  def test_that_it_has_a_version_number
    refute_nil ::Usamin::VERSION
  end

  def test_data_validation_load
    assert_equal(Usamin.load(@json), JSON.parse(@json))
  end

  def test_data_validation_parse
    assert_equal(Usamin.parse(@json), JSON.parse(@json))
  end

  def test_invalid_load
    assert_raises(Usamin::ParserError){Usamin.parse('[')}
  end

  def test_data_validation_generate
    assert_equal(Usamin.generate(@data), JSON.generate(@data))
  end

  def test_data_validation_pretty_generate
    assert_equal(Usamin.pretty_generate(@data), JSON.pretty_generate(@data))
  end

  def test_pretty_generate_with_indent
    assert_equal(Usamin.pretty_generate(@data, indent: "\t"), JSON.pretty_generate(@data, indent: "\t"))
  end

  def test_pretty_generate_single_line_array
    assert_equal(Usamin.pretty_generate({ a: [0, 1] }, single_line_array: true), "{\n  \"a\": [0, 1]\n}")
  end

  def test_pretty_generate_single_line_array_with_indent
    assert_equal(Usamin.pretty_generate({ a: [0, 1] }, single_line_array: true, indent: "\t"), "{\n\t\"a\": [0, 1]\n}")
  end

  def test_invalid_generation
    assert_raises(Usamin::UsaminError){Usamin.pretty_generate({ a: [0, 1] }, indent: 'hoge')}
  end
end
