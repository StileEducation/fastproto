require 'fileutils'
require 'rspec/core/rake_task'

task :build_compiler do
    cd 'compiler' do
        sh 'cmake', '.'
        sh 'make'
    end
end

task :gen_test_protobufs => [:build_compiler] do
    cd 'spec' do
        FileUtils.rm_r('compiled_protobufs')
        FileUtils.mkdir('compiled_protobufs')

        sh *([
            'protoc', '--cpp_out', 'compiled_protobufs', '--rb_fastproto_out', 'compiled_protobufs',
            '--plugin=protoc-gen-rb_fastproto=../compiler/rb_fastproto_compiler'
        ] + Dir['protobufs/**/*.proto'].map { |f| f.to_s })
    end
end

task :build_gen => [:gen_test_protobufs] do
    cd 'spec/compiled_protobufs' do
        sh 'ruby', 'extconf.rb'
        sh 'make'
    end
end

RSpec::Core::RakeTask.new('spec' => 'build_gen')
