require 'fileutils'
require 'rspec/core/rake_task'

RSpec::Core::RakeTask.new(:spec => [:build_gen])

FASTPROTO_COMPILER = 'compiler/rb_fastproto_compiler'

# We are invoking make, so it takes care of appropriately caching outputs
file FASTPROTO_COMPILER do
    cd 'compiler' do
        sh 'make', '-j', 'rb_fastproto_compiler'
    end
end

# Tasks to generate compiled protobufs in spec/compiled_protobufs
directory 'spec/compiled_protobufs'

# Find whether the libprotoc headers are in /usr/include or /usr/local/include
LIBPROTOBUF_HEADER_DIR_BASE = if File.exist?('/usr/include/google/protobuf')
    '/usr/include'
elsif File.exist?('/usr/local/include/google/protobuf')
    '/usr/local/include'
else
    raise "Could not find libprotobuf headers anywhere?"
end

PROTO_SOURCES = Dir.glob('spec/protobufs/**/*.proto')
# We also need to generate ruby for system proto files that are auto-magically in libprotoc
SYSTEM_PROTO_FILES = %w(google/protobuf/descriptor.proto).map { |f| File.join(LIBPROTOBUF_HEADER_DIR_BASE, f) }

file_targets = []

PROTO_SOURCES.each do |proto_f|
    # Work out the .pb.cc filename and the .fastproto.cpp filename of this proto_f,
    # which looks like spec/protobufs/foo/bar/file.proto
    # want to turn into spec/compiled_protobufs/foo/bar/file.{fastproto.cpp,pb.cc}
    pb_cc_file = proto_f.gsub(/^spec\/protobufs\//, 'spec/compiled_protobufs/').gsub(/\.proto$/, '.pb.cc')
    file pb_cc_file => ['spec/compiled_protobufs', proto_f, FASTPROTO_COMPILER] do
        sh 'protoc', '--cpp_out', 'spec/compiled_protobufs',
            '-I', 'spec/protobufs', '-I', LIBPROTOBUF_HEADER_DIR_BASE,
            proto_f
    end
    file_targets << pb_cc_file
end

(PROTO_SOURCES + SYSTEM_PROTO_FILES).each do |proto_f|
    fastproto_cpp_file = proto_f.gsub(/^spec\/protobufs\//, 'spec/compiled_protobufs/')
                                .gsub(/^#{LIBPROTOBUF_HEADER_DIR_BASE}/, 'spec/compiled_protobufs/')
                                .gsub(/\.proto$/, '.fastproto.cpp')
    file fastproto_cpp_file => ['spec/compiled_protobufs', proto_f, FASTPROTO_COMPILER] do

        sh 'protoc', '--rb-fastproto_out', 'spec/compiled_protobufs',
            "--plugin=protoc-gen-rb-fastproto=#{FASTPROTO_COMPILER}",
            '-I', 'spec/protobufs', '-I', LIBPROTOBUF_HEADER_DIR_BASE,
            proto_f
    end
    file_targets << fastproto_cpp_file
end

# Task to copy the static bits over
RUNTIME_FILES = Dir.glob('compiler/runtime/*')
RUNTIME_FILES.each do |fname|
    fname_in_build_dir = File.join('spec/compiled_protobufs', File.basename(fname))
    file fname_in_build_dir => [fname, 'spec/compiled_protobufs'] do
        cp fname, fname_in_build_dir
    end
    file_targets << fname_in_build_dir
end

# Task to actually build the .so file. The rakefile we copied over from compiler/runtime actually
# takes care of doing that properly
task :build_gen => [FASTPROTO_COMPILER, file_targets].flatten do
    cd 'spec/compiled_protobufs' do
        sh 'rake'
    end
end


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
