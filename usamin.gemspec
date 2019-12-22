# frozen_string_literal: true

lib = File.expand_path('lib', __dir__)
$:.unshift(lib) unless $:.include?(lib)
require 'usamin/version'

Gem::Specification.new do |spec|
  spec.name          = 'usamin'
  spec.version       = Usamin::VERSION
  spec.authors       = ['Ishotihadus']
  spec.email         = ['hanachan.pao@gmail.com']

  spec.summary       = 'A RapidJSON Wrapper for Ruby'
  spec.description   = 'A fast JSON serializer / deserializer for Ruby with RapidJSON.'
  spec.homepage      = 'https://github.com/Ishotihadus/usamin'
  spec.license       = 'MIT'

  spec.files         = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^(test|spec|features)/})
  end
  spec.bindir        = 'exe'
  spec.executables   = spec.files.grep(%r{^exe/}) {|f| File.basename(f)}
  spec.require_paths = ['lib']
  spec.extensions    = ['ext/usamin/extconf.rb']

  spec.add_development_dependency 'bundler'
  spec.add_development_dependency 'pry'
  spec.add_development_dependency 'rake'
  spec.add_development_dependency 'rake-compiler'
end
