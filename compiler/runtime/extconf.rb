require 'mkmf'
dir_config('protobuf')
dir_config('boost')
$CXXFLAGS << ' --std=c++14 -Wno-shorten-64-to-32 -O0'
$LOCAL_LIBS << ' -lprotobuf'
# All of our objects are in subdirectories (that match the original proto subdirectories)
# but mkmf only compiles stuff in the toplevel. Add targets to $objs representing *every*
# cpp/cc file in the tree.
$objs =  Dir['**/*.{cpp,cc,c}'].map { |f|
    File.join(File.dirname(f), File.basename(f, '.*')) + '.' + CONFIG['OBJEXT']
}
create_makefile('fastproto_gen')
# We now need to hack at the generated makefile to add a cleanup job (using a bash extglob)
# to clean up the extra targets we added.
makefile_text = File.read('Makefile')
makefile_text.gsub!(/^\\s*SHELL\\s*=.*$/, 'SHELL=/bin/bash -O extglob -c')
makefile_text += <<-EOS
    CLEANOBJS:= $(CLEANOBJS) **/*.#{CONFIG['OBJEXT']}
EOS
File.open('Makefile', 'w') { |f| f.write makefile_text }
