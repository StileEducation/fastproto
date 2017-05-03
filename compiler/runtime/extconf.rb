require 'mkmf'

$INCFLAGS << ' -I/usr/local/include'
$LIBPATH.push('/usr/local/lib')
$CXXFLAGS << ' --std=c++11 -Wno-shorten-64-to-32 -Wno-sign-compare -Wno-deprecated-declarations -O3'

$warnflags.gsub!('-Wdeclaration-after-statement', '')
$warnflags.gsub!('-Wimplicit-function-declaration', '')

dir_config('protobuf')

# All of our objects are in subdirectories (that match the original proto subdirectories)
# but mkmf only compiles stuff in the toplevel. Fix by editing $srcs and $VPATH
$srcs = Dir.glob('**/*.{cpp,cc,c}').map do |name|
  name.gsub(/\//, '__REPLACE_WITH_A_SLASH__')
end

unless have_library('protobuf')
  abort "protobuf is missing. please install protobuf"
end

create_makefile('fastproto_gen')

makefile_text = File.read('Makefile')
makefile_text_with_slashes = makefile_text.gsub(/__REPLACE_WITH_A_SLASH__/, '/')
File.open('Makefile', 'w') { |f| f.write makefile_text_with_slashes }

# Hack at the generated makefile - it includes this rule:
# $(OBJS): $(HDRS) $(ruby_headers)
# which has the effect of causing a full rebuild whenever a header file is touched. In our case, this is
# unnescessary, because the .h files are only actually required by the corresponding .pb.cc and .fastproto.cpp
# files, which will be updated in lockstep (because protoc does this). By removing this rule, we are able to
# correctly do an incremental rebuild.
# TODO: Enable this when the headers aren't actually included and we are just using forward declarations
# WARNING: I have not actually run this code yet.

# makefile_text = File.read('Makefile')
# makefile_text_without_rule = makefile_text.lines.reject { |line|
#     line =~ /\$\(OBJS\):.*\$\(HDRS\)/
# }.join("\n")
# File.open('Makefile', 'w') { |f| f.write makefile_text_without_rule }
