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

The values show the elapsed time for operating 20 times. SSE4.2 was enabled in these tests.

#### Parsing

```
nativejson-benchmark/data/canada.json
json                     0.743695   0.005426   0.749121 (  0.751004)
oj                       1.779671   0.039642   1.819313 (  1.824576)
usamin                   0.510856   0.013166   0.524022 (  0.525174)
usamin (fast)            0.258916   0.007298   0.266214 (  0.267910)
usamin (load)            0.403337   0.010124   0.413461 (  0.414023)
usamin (load / fast)     0.164099   0.020126   0.184225 (  0.184817)

nativejson-benchmark/data/citm_catalog.json
json                     0.481490   0.006217   0.487707 (  0.490407)
oj                       0.337618   0.002676   0.340294 (  0.340697)
usamin                   0.363082   0.005774   0.368856 (  0.370124)
usamin (fast)            0.348798   0.005620   0.354418 (  0.355544)
usamin (load)            0.114139   0.004866   0.119005 (  0.119659)
usamin (load / fast)     0.101378   0.004685   0.106063 (  0.106161)

nativejson-benchmark/data/twitter.json
json                     0.242091   0.004512   0.246603 (  0.246985)
oj                       0.165441   0.000833   0.166274 (  0.166517)
usamin                   0.179744   0.000353   0.180097 (  0.180320)
usamin (fast)            0.181126   0.000577   0.181703 (  0.182080)
usamin (load)            0.061376   0.000944   0.062320 (  0.062647)
usamin (load / fast)     0.064891   0.001240   0.066131 (  0.067542)
```

#### Generating

```
nativejson-benchmark/data/canada.json
json                     1.964397   0.024287   1.988684 (  1.993397)
oj                       2.070033   0.028986   2.099019 (  2.107441)
usamin                   0.247659   0.018489   0.266148 (  0.266663)
usamin (load)            0.246872   0.027028   0.273900 (  0.274989)

nativejson-benchmark/data/citm_catalog.json
json                     0.244533   0.009066   0.253599 (  0.255088)
oj                       0.069532   0.008088   0.077620 (  0.077661)
usamin                   0.058047   0.009538   0.067585 (  0.067694)
usamin (load)            0.038105   0.009722   0.047827 (  0.047993)

nativejson-benchmark/data/twitter.json
json                     0.160310   0.006469   0.166779 (  0.167137)
oj                       0.042114   0.004044   0.046158 (  0.046254)
usamin                   0.038971   0.004943   0.043914 (  0.044313)
usamin (load)            0.029375   0.004385   0.033760 (  0.033882)
```

#### Pretty Generating

```
nativejson-benchmark/data/canada.json
json                     2.224924   0.089327   2.314251 (  2.320405)
oj                       1.624790   0.022204   1.646994 (  1.650566)
usamin                   0.356511   0.084944   0.441455 (  0.442372)
usamin (load)            0.334884   0.076830   0.411714 (  0.412275)

nativejson-benchmark/data/citm_catalog.json
json                     0.302005   0.017108   0.319113 (  0.322104)
oj                       0.071386   0.008597   0.079983 (  0.080194)
usamin                   0.099413   0.021275   0.120688 (  0.121417)
usamin (load)            0.060486   0.021065   0.081551 (  0.081808)

nativejson-benchmark/data/twitter.json
json                     0.170121   0.011923   0.182044 (  0.182585)
oj                       0.035700   0.006364   0.042064 (  0.042099)
usamin                   0.040141   0.006321   0.046462 (  0.046556)
usamin (load)            0.036990   0.005729   0.042719 (  0.042816)
```

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/Ishotihadus/usamin.

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT) at the request of RapidJSON.
