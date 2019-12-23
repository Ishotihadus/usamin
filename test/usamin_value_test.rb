# frozen_string_literal: true

require 'json'
require 'minitest/autorun'
require_relative 'test_helper'

class UsaminValueTest < Minitest::Test
  def setup
    @data_object = Usamin.load('{"a": true}')
    @data_array = Usamin.load('[true]')
  end

  def test_is_array
    refute_predicate(@data_object, :array?)
    assert_predicate(@data_array, :array?)
  end

  def test_is_hash
    assert_predicate(@data_object, :hash?)
    assert_predicate(@data_object, :object?)
    refute_predicate(@data_array, :hash?)
    refute_predicate(@data_array, :object?)
  end

  def test_equal
    assert_equal(@data_object, Usamin.parse('{"a": true}'))
    assert_equal(@data_object, Usamin.load('{"a": true}'))
    assert_equal(@data_array, Usamin.parse('[true]'))
    assert_equal(@data_array, Usamin.load('[true]'))
  end

  def test_not_equal
    refute_equal(@data_object, Usamin.parse('{"a": false}'))
    refute_equal(@data_object, Usamin.load('{"a": false}'))
    refute_equal(@data_array, Usamin.parse('[false]'))
    refute_equal(@data_array, Usamin.load('[false]'))
  end

  def test_eql
    assert(@data_object.eql?(Usamin.load('{"a": true}')))
    assert(@data_array.eql?(Usamin.load('[true]')))
  end

  def test_not_eql
    refute(@data_object.eql?(Usamin.load('{"a": false}')))
    refute(@data_array.eql?(Usamin.load('[false]')))
  end

  def test_not_eql_different_type
    refute(@data_object.eql?(Usamin.parse('{"a": true}')))
    refute(@data_array.eql?(Usamin.parse('[true]')))
  end

  def test_always_frozen
    assert(@data_array.frozen?)
  end

  def test_root
    text = '{"a": [1, 2, 3]}'
    obj = Usamin.load(text)['a']
    assert_equal(obj.root, Usamin.load(text))
  end

  def test_marshal
    assert_equal(@data_object, Marshal.load(Marshal.dump(@data_object)))
    assert_equal(@data_array, Marshal.load(Marshal.dump(@data_array)))
  end
end
