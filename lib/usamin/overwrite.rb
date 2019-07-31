require 'usamin'

module JSON
    def self.generate(object, **options)
        options[:indent] ? Usamin.pretty_generate(object, indent: options[:indent]) : Usamin.generate(object)
    end

    def self.pretty_generate(object, **options)
        options[:indent] ? Usamin.pretty_generate(object, indent: options[:indent]) : Usamin.pretty_generate(object)
    end

    def self.parse(source, **options)
        Usamin.parse(source)
    end

    def self.load(source, proc = nil, **options)
        ret = Usamin.parse(source)
        proc.call(ret) if proc
        ret
    end

    class << self
        alias fast_generate generate
        alias restore load
    end
end
