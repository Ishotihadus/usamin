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
#=> => [{...}, {...}]
```

Here, `data` is not an Array, but this can be handled like an Array.

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
- Hash objects are based on not hash tables but arrays. An index access to an object costs O(n).

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

Ruby 2.7.0-rc2. json 2.3.0, oj 3.10.0, usamin 7.7.10 (rapidjson 1.1.0).

#### Parsing

```
nativejson-benchmark/data/canada.json
json                     0.734855   0.005684   0.740539 (  0.743125)
oj                       1.906612   0.022766   1.929378 (  1.938912)
usamin                   0.546606   0.016939   0.563545 (  0.565339)
usamin (fast)            0.221778   0.013511   0.235289 (  0.235782)
usamin (load)            0.467457   0.018688   0.486145 (  0.487849)
usamin (load / fast)     0.166556   0.025738   0.192294 (  0.192736)

nativejson-benchmark/data/citm_catalog.json
json                     0.339319   0.004765   0.344084 (  0.345174)
oj                       0.224548   0.000997   0.225545 (  0.225837)
usamin                   0.278662   0.003313   0.281975 (  0.285040)
usamin (fast)            0.232262   0.001691   0.233953 (  0.234662)
usamin (load)            0.111687   0.006829   0.118516 (  0.118821)
usamin (load / fast)     0.072404   0.007138   0.079542 (  0.079620)

nativejson-benchmark/data/twitter.json
json                     0.208798   0.004463   0.213261 (  0.213952)
oj                       0.134336   0.000970   0.135306 (  0.135999)
usamin                   0.174997   0.000755   0.175752 (  0.176467)
usamin (fast)            0.176687   0.001193   0.177880 (  0.179466)
usamin (load)            0.062983   0.004450   0.067433 (  0.067547)
usamin (load / fast)     0.063495   0.006539   0.070034 (  0.071615)
```

#### Generating

```
nativejson-benchmark/data/canada.json
json                     2.039965   0.015920   2.055885 (  2.065514)
oj                       2.008353   0.004610   2.012963 (  2.016850)
usamin                   0.276563   0.015915   0.292478 (  0.294615)
usamin (load)            0.256360   0.010180   0.266540 (  0.268350)

nativejson-benchmark/data/citm_catalog.json
json                     0.068053   0.004018   0.072071 (  0.072138)
oj                       0.060933   0.003070   0.064003 (  0.064161)
usamin                   0.056743   0.008311   0.065054 (  0.065449)
usamin (load)            0.037438   0.003680   0.041118 (  0.041461)

nativejson-benchmark/data/twitter.json
json                     0.040689   0.003881   0.044570 (  0.044641)
oj                       0.038957   0.003410   0.042367 (  0.042525)
usamin                   0.037130   0.005539   0.042669 (  0.042951)
usamin (load)            0.031568   0.003316   0.034884 (  0.035690)
```

#### Pretty Generating

```
nativejson-benchmark/data/canada.json
json                     2.247403   0.056727   2.304130 (  2.312832)
oj                       1.560007   0.005153   1.565160 (  1.569151)
usamin                   0.353357   0.063384   0.416741 (  0.418236)
usamin (load)            0.341948   0.055289   0.397237 (  0.399525)

nativejson-benchmark/data/citm_catalog.json
json                     0.128840   0.008824   0.137664 (  0.139104)
oj                       0.061869   0.004010   0.065879 (  0.067000)
usamin                   0.071300   0.005988   0.077288 (  0.077439)
usamin (load)            0.048758   0.004353   0.053111 (  0.053186)

nativejson-benchmark/data/twitter.json
json                     0.060095   0.004639   0.064734 (  0.065314)
oj                       0.037025   0.004194   0.041219 (  0.041495)
usamin                   0.053145   0.011938   0.065083 (  0.065184)
usamin (load)            0.034704   0.002547   0.037251 (  0.037505)
```

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/Ishotihadus/usamin.

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT) at the request of RapidJSON.
