# frozen_string_literal: true

require 'json'
require 'minitest/autorun'
require_relative 'test_helper'

class UsaminArrayTest < Minitest::Test
  def setup
    @data = Usamin.load('[0, 1, true, false, null, "mimimin", "mimimin", "u-samin"]')
    @data_empty = Usamin.load('[]')
  end

  def test_indexer
    assert_equal(@data[0], 0)
    assert_equal(@data[1], 1)
    assert_equal(@data[2], true)
    assert_equal(@data[3], false)
    assert_nil(@data[4])
    assert_equal(@data[5], 'mimimin')
    assert_equal(@data[6], 'mimimin')
    assert_equal(@data[7], 'u-samin')
    assert_equal(@data[-1], 'u-samin')
    assert_equal(@data[1, 0], [])
    assert_equal(@data[1, 1], [1])
    assert_equal(@data[1, 3], [1, true, false])
    assert_equal(@data[1..3], [1, true, false])
  end

  def test_indexer_outofbounds
    assert_equal(@data[7], 'u-samin')
    assert_nil(@data[8])
    assert_nil(@data[9])

    assert_equal(@data[-8], 0)
    assert_nil(@data[-9])
    assert_nil(@data[-10])

    assert_empty(@data[8, 2])
    assert_nil(@data[9, 2])
    assert_nil(@data[-9, 2])

    assert_empty(@data[8..8])
    assert_nil(@data[9..9])

    assert_equal(@data[-8..-8], [0])
    assert_nil(@data[-9..-9])
  end

  def test_indexer_empty
    assert_nil(@data_empty[0])
    assert_nil(@data_empty[-1])
    assert_empty(@data_empty[0, 2])
    assert_nil(@data_empty[1, 2])
    assert_nil(@data_empty[-1, 2])
    assert_empty(@data_empty[0..0])
    assert_nil(@data_empty[1..1])
    assert_nil(@data_empty[-1..-1])
  end

  def test_at
    assert_equal(@data.at(0), 0)
    assert_equal(@data.at(-8), 0)
    assert_equal(@data.at(2), true)
    assert_equal(@data.at(-1), 'u-samin')
    assert_nil(@data.at(8))
    assert_nil(@data.at(-9))
  end

  def test_compact
    assert_equal(@data.compact, @data.eval_r.compact)
  end

  def test_dig
    assert_equal(@data.dig(0), 0)
    assert_nil(@data.dig(8))
    assert_nil(@data.dig(-9))

    d = Usamin.load('[0, [1, [2, [3, [4, [5, 6]]]]]]')
    assert_equal(d.dig(0), 0)
    assert_equal(d.dig(1, 0), 1)
    assert_equal(d.dig(1, 1, 0), 2)
    assert_equal(d.dig(1, 1, 1, 1, 1), [5, 6])
  end

  def test_each
    assert_equal(@data.each.to_a, @data.to_ary)
  end

  def test_each_index
    assert_equal(@data.each_index.to_a, (0...@data.size).to_a)
  end

  def test_is_empty
    refute_predicate(@data, :empty?)
    assert_predicate(@data_empty, :empty?)
  end

  def test_fetch
    assert_equal(@data.fetch(0), 0)
    assert_equal(@data.fetch(-8), 0)
    assert_equal(@data.fetch(-1), 'u-samin')
    assert_equal(@data.fetch(7), 'u-samin')
    assert_raises(IndexError){@data.fetch(8)}
    assert_raises(IndexError){@data.fetch(-9)}
    assert_equal(@data.fetch(8, 'hayate'), 'hayate')
    assert_equal(@data.fetch(8){'nagi'}, 'nagi')
    assert_equal(@data.fetch(8, 'hayate'){'nagi'}, 'nagi')
  end

  def test_index
    assert_equal(@data.index(true), 2)
    assert_nil(@data.index('nya'))
    assert_equal(@data.index{|e| !e}, 3)
    assert_equal(@data.index(1){|e| !e}, 1)
  end

  def test_flatten
    ref = [0, [1, [2, [3, [4, [5, 6]]]]]]
    d = Usamin.load('[0, [1, [2, [3, [4, [5, 6]]]]]]')
    assert_equal(d.flatten(0), ref.flatten(0))
    assert_equal(d.flatten(1), ref.flatten(1))
    assert_equal(d.flatten(2), ref.flatten(2))
    assert_equal(d.flatten, ref.flatten)
    assert_equal(d.flatten(-1), ref.flatten(-1))
    assert_equal(d.flatten(-2), ref.flatten(-2))
  end

  def test_first
    assert_equal(@data.first, 0)
    assert_equal(@data.first(0), [])
    assert_equal(@data.first(1), [0])
    assert_equal(@data.first(2), [0, 1])
    assert_nil(@data_empty.first)
  end

  def test_include?
    assert(@data.include?(0))
    assert(@data.include?(1))
    refute(@data.include?('otokura-chan'))
  end

  def test_inspect
    assert_equal(@data.inspect, @data.eval_r.to_s)
  end

  def test_last
    assert_equal(@data.last, 'u-samin')
    assert_equal(@data.last(0), [])
    assert_equal(@data.last(1), ['u-samin'])
    assert_equal(@data.last(2), ['mimimin', 'u-samin'])
    assert_nil(@data_empty.last)
  end

  def test_length
    assert_equal(@data.length, 8)
    assert_equal(@data_empty.length, 0)
  end

  def test_reverse
    assert_equal(@data.reverse, ['u-samin', 'mimimin', 'mimimin', nil, false, true, 1, 0])
    assert_empty(@data_empty.reverse)
  end

  def test_rindex
    assert_equal(@data.rindex(true), 2)
    assert_nil(@data.rindex('nya'))
    assert_equal(@data.rindex{|e| !e}, 4)
    assert_equal(@data.rindex(1){|e| !e}, 1)
  end

  def test_rotate
    assert_equal(@data.rotate, [1, true, false, nil, 'mimimin', 'mimimin', 'u-samin', 0])
    assert_equal(@data.rotate, @data.rotate(1))
    assert_equal(@data.rotate(2), @data.rotate.rotate)
    assert_equal(@data.rotate(-1), ['u-samin', 0, 1, true, false, nil, 'mimimin', 'mimimin'])
  end
end
