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
        LoadProcCaller.load_call_proc(ret, proc) if proc
        ret
    end

    class LoadProcCaller
        def self.load_call_proc(object, proc)
            if object.is_a?(Array)
                object.each{|e| load_call_proc(e, proc)}
            elsif object.is_a?(Hash)
                object.each do |k, v|
                    proc.call(k)
                    load_call_proc(v, proc)
                end
            end
            proc.call(object)
        end
    end

    class << self
        alias fast_generate generate
        alias restore load
    end
end
