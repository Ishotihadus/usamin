# frozen_string_literal: true

require 'json'
require 'minitest/autorun'
require_relative 'test_helper'

class UsaminHashTest < Minitest::Test
  def setup
    @data = Usamin.load('{"a": 0, "b": 1, "c": 2, "d": null}')
  end

  def test_indexer
    assert_equal(@data['a'], 0)
    assert_equal(@data['b'], 1)
    assert_equal(@data['c'], 2)
    assert_nil(@data['d'])
    assert_nil(@data['e'])
  end

  def test_assoc
    assert_equal(@data.assoc('a'), ['a', 0])
    assert_equal(@data.assoc('b'), ['b', 1])
    assert_equal(@data.assoc('c'), ['c', 2])
    assert_equal(@data.assoc('d'), ['d', nil])
    assert_nil(@data.assoc('e'))
  end

  def test_compact
    assert_equal(@data.compact, {'a' => 0, 'b' => 1, 'c' => 2})
  end

  def test_dig
    data = Usamin.load('{"a": {"b": "c"}}')
    assert_equal(data.dig('a'), {'b' => 'c'})
    assert_equal(data.dig('a', 'b'), 'c')
    assert_nil(data.dig('a', 'd'))
    assert_nil(data.dig('b'))
  end

  def test_each
    assert_equal(@data.each.to_a, [['a', 0], ['b', 1], ['c', 2], ['d', nil]])
  end

  def test_each_key
    assert_equal(@data.each_key.to_a, %w[a b c d])
  end

  def test_each_value
    assert_equal(@data.each_value.to_a, [0, 1, 2, nil])
  end

  def test_is_empty
    refute_predicate(@data, :empty?)
    assert_predicate(Usamin.load('{}'), :empty?)
  end

  def test_fetch
    assert_equal(@data.fetch('a'), 0)
    assert_equal(@data.fetch('a', 10), 0)
    assert_equal(@data.fetch('a'){7}, 0)
    assert_equal(@data.fetch('a', 10){7}, 0)
    assert_nil(@data.fetch('e'))
    assert_equal(@data.fetch('e', 10), 10)
    assert_equal(@data.fetch('e'){7}, 7)
    assert_equal(@data.fetch('e', 10){7}, 7)
  end

  def test_fetch_values
    assert_equal(@data.fetch_values('a'), [0])
    assert_equal(@data.fetch_values('a'){7}, [0])
    assert_raises(KeyError){@data.fetch_values('e')}
    assert_equal(@data.fetch_values('e'){7}, [7])
    assert_equal(@data.fetch_values('a', 'e'){'q'}, [0, 'q'])
  end

  def test_flatten
    text = '{"kitami": "yuzu", "hisakawa": [["nagi", "naa"], "hayate"]}'
    data = Usamin.load(text)
    ref = JSON.parse(text)
    assert_equal(data.flatten, ref.flatten)
    assert_equal(data.flatten(0), ref.flatten(0))
    assert_equal(data.flatten(2), ref.flatten(2))
    assert_equal(data.flatten(-1), ref.flatten(-1))
  end

  def test_has_key
    assert(@data.has_key?('d'))
    refute(@data.has_key?('e'))
    assert(@data.include?('d'))
    refute(@data.include?('e'))
    assert(@data.key?('d'))
    refute(@data.key?('e'))
    assert(@data.member?('d'))
    refute(@data.member?('e'))
  end

  def test_has_value
    assert(@data.has_value?(0))
    refute(@data.has_value?(3))
    assert(@data.value?(0))
    refute(@data.value?(3))
  end

  def test_key
    assert_equal(@data.key(0), 'a')
    assert_nil(@data.key(5))
    assert_equal(@data.index(0), 'a')
    assert_nil(@data.index(5))
  end

  def test_inspect
    assert_equal(@data.inspect, @data.eval_r.inspect)
  end

  def test_invert
    assert_equal(@data.invert, { 0 => 'a', 1 => 'b', 2 => 'c', nil => 'd' })
  end

  def test_keys
    assert_equal(@data.keys, ['a', 'b', 'c', 'd'])
  end

  def test_length
    assert_equal(@data.length, 4)
    assert_equal(@data.size, 4)
  end

  def test_merge
    assert_equal(@data.merge({'c' => 9, 'd' => 3, 'e' => 10}), {'a' => 0, 'b' => 1, 'c' => 9, 'd' => 3, 'e' => 10})
    assert_equal(@data.merge({'c' => 9, 'd' => 3, 'e' => 10}){|k, s, v| k}, {'a' => 0, 'b' => 1, 'c' => 'c', 'd' => 'd', 'e' => 10})
  end

  def test_rassoc
    assert_equal(@data.rassoc(2), ['c', 2])
    assert_nil(@data.rassoc(6))
  end

  def test_reject
    assert_equal(@data.reject{|_k, v| v}, { 'd' => nil })
  end

  def test_select
    assert_equal(@data.select{|_k, v| !v}, { 'd' => nil })
  end

  def test_slice
    assert_equal(@data.slice(), {})
    assert_equal(@data.slice('a'), { 'a' => 0 })
    assert_equal(@data.slice('a', 'd'), { 'a' => 0, 'd' => nil })
    assert_equal(@data.slice('a', 'x'), { 'a' => 0 })
  end

  def test_to_hash
    assert_equal(@data.to_hash, @data.eval)
  end

  def test_to_proc
    pr = @data.to_proc
    assert_instance_of(Proc, pr)
    assert_equal(['b', 'c', 'e'].map(&pr), [1, 2, nil])
  end

  def test_transform_keys
    assert_equal(@data.transform_keys{|k| k * 2}, { 'aa' => 0, 'bb' => 1, 'cc' => 2, 'dd' => nil })
  end

  def test_transform_values
    assert_equal(@data.transform_values{|k| k && k * 2}, { 'a' => 0, 'b' => 2, 'c' => 4, 'd' => nil })
  end

  def test_values
    assert_equal(@data.values, [0, 1, 2, nil])
  end

  def test_values_at
    assert_equal(@data.values_at, [])
    assert_equal(@data.values_at('d'), [nil])
    assert_equal(@data.values_at('a', 'b'), [0, 1])
    assert_equal(@data.values_at(:a, :e), [0, nil])
  end
end
