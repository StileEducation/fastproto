require 'fileutils'

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
