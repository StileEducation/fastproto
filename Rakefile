require 'fileutils'
require 'rspec/core/rake_task'

task :build_compiler do
    cd 'compiler' do
        sh 'cmake', '.'
        sh 'make', 'clean', 'rb_fastproto_compiler'
    end
end

task :gen_test_protobufs => [:build_compiler] do
    cd 'spec' do
        FileUtils.rm_rf('compiled_protobufs')
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

task :gen_benchmark_protos => [:build_compiler] do
    cd 'benchmark' do
        FileUtils.rm_rf('fastproto_compiled_protobufs')
        FileUtils.mkdir('fastproto_compiled_protobufs')
        FileUtils.rm_rf('rubypb_compiled_protobufs')
        FileUtils.mkdir('rubypb_compiled_protobufs')

        sh *([
            'protoc', '--cpp_out', 'fastproto_compiled_protobufs', '--rb_fastproto_out', 'fastproto_compiled_protobufs',
            '--plugin=protoc-gen-rb_fastproto=../compiler/rb_fastproto_compiler'
        ] + Dir['protobufs/**/*.proto'].map { |f| f.to_s })

        sh *([
            'ruby-protoc', '-o',  'rubypb_compiled_protobufs',
        ] + Dir['protobufs/**/*.proto'].map { |f| f.to_s })

        cd 'fastproto_compiled_protobufs' do
            sh 'ruby', 'extconf.rb'
            sh 'make'
        end
    end
end

task :benchmark => [:gen_benchmark_protos] do
    sh 'ruby', 'benchmark/benchmark_st.rb'
end
