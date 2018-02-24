# Usamin

A fast JSON serializer / deserializer for Ruby with [RapidJSON](http://rapidjson.org/).

The name of "Usamin" is derived from [Nana Abe](https://www.project-imas.com/wiki/Nana_Abe).

## Installation

Install RapidJSON beforehand.

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
json = '{
    "miku maekawa": {
        "age": 15,
        "height": 152,
        "weight": 45,
        "body_size": [85, 55, 81],
        "birthday": [2, 22],
        "cv": "natsumi takamori"
    },
    "nana abe": {
        "age": 17,
        "height": 146,
        "weight": 40,
        "body_size": [84, 57, 84],
        "birthday": [5, 15],
        "cv": "marie miyake"
    }
}'

Usamin.parse(json)
```

### Lazy loading

```ruby
data = Usamin.load(json)
```

Here, `data` is not a Hash, but this can be handled like a Hash.

```ruby
data.keys
#=> ["miku maekawa", "nana abe"]

data.map{|k, v| [k, v['age']]}.to_h
#=> {"miku maekawa"=>15, "nana abe"=>17}
```

Array data also can be handled like an Array object.

```ruby
data['nana abe']['body_size'].size
#=> 3

data['nana abe']['body_size'].inject(:+)
#=> 225
```

The `eval` and `eval_r` converts these data structures into the Ruby data structures. `_r` means recursiveness.

```ruby
data.eval
#=> {"miku maekawa"=>#<Usamin::Hash>, "nana abe"=>#<Usamin::Hash>}

data['miku maekawa'].eval_r
#=> {"age"=>15, "height"=>152, "weight"=>45, "body_size"=>[85, 55, 81], "birthday"=>[2, 22], "cv"=>"natsumi takamori"}

data.eval_r
#=> same as Usamin.parse(json)
```

#### Notes about lazy loading data

- Frozen. Modification is not allowed.
- A key list of Hash is based on not hash tables but arrays. An index access to hash costs O(n).

### Generating

```ruby
data = {
    "miku maekawa" => {
        "age"=>15,
        "height"=>152,
        "weight"=>45,
        "body_size"=>[85, 55, 81],
        "birthday"=>[2, 22],
        "cv"=>"natsumi takamori"},
    "nana abe"=> {
        "age"=>17,
        "height"=>146,
        "weight"=>40,
        "body_size"=>[84, 57, 84],
        "birthday"=>[5, 15],
        "cv"=>"marie miyake"}}

Usamin.generate(data)

# pretty generation is also supported
Usamin.pretty_generate(data)
```

Of course, UsaminValue also can be serialized.

```ruby
data = Usamin.load(json)

Usamin.generate(data['nana abe'])
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
json                     0.785524   0.007927   0.793451 (  0.805269)
oj                       1.833580   0.043648   1.877228 (  1.895704)
usamin                   0.595523   0.012299   0.607822 (  0.612215)
usamin (fast)            0.286458   0.008710   0.295168 (  0.297998)
usamin (load)            0.484056   0.011770   0.495826 (  0.498110)
usamin (load / fast)     0.187712   0.020202   0.207914 (  0.210711)

nativejson-benchmark/data/citm_catalog.json
json                     0.469471   0.005208   0.474679 (  0.478920)
oj                       0.327143   0.002947   0.330090 (  0.331882)
usamin                   0.362764   0.005633   0.368397 (  0.370940)
usamin (fast)            0.359859   0.005884   0.365743 (  0.371016)
usamin (load)            0.117140   0.007003   0.124143 (  0.126548)
usamin (load / fast)     0.115237   0.010061   0.125298 (  0.128470)

nativejson-benchmark/data/twitter.json
json                     0.238582   0.002561   0.241143 (  0.243726)
oj                       0.162212   0.001031   0.163243 (  0.165047)
usamin                   0.194476   0.001523   0.195999 (  0.197920)
usamin (fast)            0.192985   0.001339   0.194324 (  0.197404)
usamin (load)            0.070360   0.005012   0.075372 (  0.078090)
usamin (load / fast)     0.067618   0.006416   0.074034 (  0.076244)
```

#### Generating

```
nativejson-benchmark/data/canada.json
json                     1.988155   0.029728   2.017883 (  2.023475)
oj                       2.092999   0.033136   2.126135 (  2.135765)
usamin                   0.296385   0.031412   0.327797 (  0.330314)

nativejson-benchmark/data/citm_catalog.json
json                     0.243795   0.007463   0.251258 (  0.256621)
oj                       0.076474   0.009617   0.086091 (  0.087966)
usamin                   0.059434   0.009616   0.069050 (  0.071158)

nativejson-benchmark/data/twitter.json
json                     0.159030   0.006730   0.165760 (  0.170796)
oj                       0.052856   0.009164   0.062020 (  0.064344)
usamin                   0.047707   0.010265   0.057972 (  0.061729)
```

#### Pretty Generating

```
nativejson-benchmark/data/canada.json
json                     2.188493   0.066003   2.254496 (  2.260332)
oj                       1.583613   0.029798   1.613411 (  1.619652)
usamin                   0.399066   0.081314   0.480380 (  0.482841)

nativejson-benchmark/data/citm_catalog.json
json                     0.300601   0.029596   0.330197 (  0.335351)
oj                       0.065151   0.009553   0.074704 (  0.077010)
usamin                   0.089872   0.022203   0.112075 (  0.116063)

nativejson-benchmark/data/twitter.json
json                     0.189439   0.010086   0.199525 (  0.204491)
oj                       0.040982   0.008203   0.049185 (  0.049352)
usamin                   0.044296   0.010095   0.054391 (  0.056245)
```

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/Ishotihadus/usamin.

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT) at the request of RapidJSON.
