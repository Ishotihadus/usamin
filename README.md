# Usamin

A fast JSON serializer / deserializer for Ruby with [RapidJSON](http://rapidjson.org/).

The name of "Usamin" is derived from [Nana Abe](https://www.project-imas.com/wiki/Nana_Abe).

I congratulate her on her election as the [7th Cinderella Girl](https://www.project-imas.com/wiki/THE_iDOLM@STER_Cinderella_Girls_General_Election#7th_Cinderella_Girl_General_Election).

## Installation

Install RapidJSON beforehand. Only header files are necessary, and no need to build.

Next, add this line to your application's Gemfile:

```ruby
gem 'usamin'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install usamin

The directory of RapidJSON can be explicitly specified with `--with-rapidjson-dir` option.

    $ gem install usamin -- --with-rapidjson-dir=/usr/local/opt/rapidjson

## Usage

### Loading library

```ruby
require 'usamin'
```

### Parsing

```ruby
json = <<JSON
[
    {
        "name": "miku maekawa",
        "age": 15,
        "height": 152,
        "weight": 45,
        "body_size": [85, 55, 81],
        "birthday": "2/22",
        "cv": "natsumi takamori"
    },
    {
        "name": "nana abe",
        "age": 17,
        "height": 146,
        "weight": 40,
        "body_size": [84, 57, 84],
        "birthday": "5/15",
        "cv": "marie miyake"
    }
]
JSON

Usamin.parse(json)
```

### Lazy loading

```ruby
data = Usamin.load(json)
```

Here, `data` is not a Array, but this can be handled like an Array.

```ruby
data.size
#=> 2

data.map{|e| e['name']}
#=> ["miku maekawa", "nana abe"]
```

Objects also can be handled like Hash objects.

```ruby
data.first['name']
#=> "miku maekawa"

data.first.keys
#=> ["name", "age", "height", "weight", "body_size", "birthday", "cv"]
```

The methods `eval` and `eval_r` convert these data structures into Ruby data structures. `_r` means recursiveness.

```ruby
data.eval
#=> [#<Usamin::Hash>, #<Usamin::Hash>]

data.first.eval_r
#=> {"name"=>"miku maekawa", "age"=>15, "height"=>152, "weight"=>45, "body_size"=>[85, 55, 81], "birthday"=>"2/22", "cv"=>"natsumi takamori"}

# same as Usamin.parse(json)
Usamin.load(json).eval_r
```

#### Notes about lazy loading data

- Frozen. Modification is not allowed.
- A key list of Hash is based on not hash tables but arrays. An index access to hash costs O(n).

### Generating

```ruby
data = [{"name" => "miku maekawa", "age" => 15,
    "height" => 152, "weight" => 45,
    "body_size" => [85, 55, 81], "birthday" => "2/22",
    "cv" => "natsumi takamori"}, {
    "name" => "nana abe", "age" => 17,
    "height" => 146, "weight" => 40,
    "body_size" => [84, 57, 84], "birthday" => "5/15",
    "cv" => "marie miyake"}]

Usamin.generate(data)

# pretty generation is also supported
Usamin.pretty_generate(data)
```

Of course, UsaminValue also can be serialized.

```ruby
data = Usamin.load(json)
Usamin.generate(data[1])
```

### Fast parsing

Usamin uses `kParseFullPrecisionFlag` by default, but this option makes parsing a little slow.

You can use `:fast` option to avoid this.

```ruby
# default
Usamin.parse('77.777700000000795')
#=> 77.77770000000079

# fast but not precise
Usamin.parse('77.777700000000795', fast: true)
#=> 77.7777000000008
```

### Error handling

```ruby
str = '{"this is bad example"'
Usamin.parse(str)
# Usamin::ParserError: Missing a colon after a name of object member. Offset: 22
```

### Overwrite JSON module

You can overwrite JSON module methods by loading `usamin/overwrite`.

```ruby
require 'usamin/overwrite'

# These methods are based on Usamin
JSON.parse(json)
JSON.generate(data)
JSON.pretty_generate(data)
```

The overwritten methods are as follows:

- JSON.parse -> Usamin.parse
- JSON.load / JSON.restore -> Usamin.parse
- JSON.generate -> Usamin.generate / Usamin.pretty_generate
- JSON.pretty_generate -> Usamin.pretty_generate

You can automatically switch packages by following technique.

```ruby
begin
    require 'usamin'
    require 'usamin/overwrite'
rescue LoadError
    require 'json'
end
```

### Documentation

See: http://www.rubydoc.info/gems/usamin/

## Benchmarks

Based on [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark).

### Roundtrips

Usamin passes all roundtrips, and the results are same as official JSON package.

### Reliability of parsed data

Usamin and JSON load the same data from 3 big json data in nativejson-benchmark.

### Performance

The values show the elapsed time for operating 20 times.

#### Parsing

```
nativejson-benchmark/data/canada.json
json                     0.755101   0.004066   0.759167 (  0.762169)
oj                       1.873840   0.040992   1.914832 (  1.919647)
usamin                   0.582432   0.011584   0.594016 (  0.596472)
usamin (fast)            0.271741   0.004775   0.276516 (  0.278315)
usamin (load)            0.458602   0.009857   0.468459 (  0.471155)
usamin (load / fast)     0.183260   0.019372   0.202632 (  0.204489)

nativejson-benchmark/data/citm_catalog.json
json                     0.477735   0.006309   0.484044 (  0.487179)
oj                       0.374920   0.005170   0.380090 (  0.384444)
usamin                   0.363176   0.004812   0.367988 (  0.370558)
usamin (fast)            0.352986   0.004893   0.357879 (  0.360197)
usamin (load)            0.123704   0.006770   0.130474 (  0.133101)
usamin (load / fast)     0.106889   0.008363   0.115252 (  0.117514)

nativejson-benchmark/data/twitter.json
json                     0.227502   0.001665   0.229167 (  0.233347)
oj                       0.148312   0.000936   0.149248 (  0.151006)
usamin                   0.178398   0.003571   0.181969 (  0.183786)
usamin (fast)            0.170842   0.000973   0.171815 (  0.173604)
usamin (load)            0.064007   0.005254   0.069261 (  0.071036)
usamin (load / fast)     0.068870   0.006189   0.075059 (  0.077111)
```

#### Generating

```
nativejson-benchmark/data/canada.json
json                     1.994007   0.026034   2.020041 (  2.025446)
oj                       2.087961   0.029023   2.116984 (  2.123429)
usamin                   0.274208   0.022461   0.296669 (  0.298859)
usamin (load)            0.273885   0.031334   0.305219 (  0.310155)

nativejson-benchmark/data/citm_catalog.json
json                     0.237936   0.009822   0.247758 (  0.252693)
oj                       0.073700   0.008512   0.082212 (  0.084021)
usamin                   0.064633   0.010208   0.074841 (  0.077269)
usamin (load)            0.041944   0.010903   0.052847 (  0.055097)

nativejson-benchmark/data/twitter.json
json                     0.165987   0.005908   0.171895 (  0.176533)
oj                       0.042210   0.005486   0.047696 (  0.049477)
usamin                   0.039056   0.009091   0.048147 (  0.050299)
usamin (load)            0.029979   0.009041   0.039020 (  0.041219)
```

#### Pretty Generating

```
nativejson-benchmark/data/canada.json
json                     2.196968   0.067552   2.264520 (  2.270109)
oj                       1.549510   0.019002   1.568512 (  1.573885)
usamin                   0.373060   0.071227   0.444287 (  0.446266)
usamin (load)            0.363781   0.067480   0.431261 (  0.433839)

nativejson-benchmark/data/citm_catalog.json
json                     0.285428   0.022632   0.308060 (  0.312022)
oj                       0.064475   0.008716   0.073191 (  0.075125)
usamin                   0.088890   0.019419   0.108309 (  0.110423)
usamin (load)            0.058728   0.018471   0.077199 (  0.079330)

nativejson-benchmark/data/twitter.json
json                     0.170966   0.010184   0.181150 (  0.186188)
oj                       0.038465   0.007323   0.045788 (  0.047589)
usamin                   0.046873   0.011960   0.058833 (  0.060903)
usamin (load)            0.038984   0.010469   0.049453 (  0.049652)
```

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/Ishotihadus/usamin.

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT) at the request of RapidJSON.
