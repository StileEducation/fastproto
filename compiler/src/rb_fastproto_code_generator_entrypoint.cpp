#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <boost/scoped_ptr.hpp>
#include <iostream>
#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {
    void RBFastProtoCodeGenerator::write_entrypoint(
        const google::protobuf::FileDescriptor* file,
        google::protobuf::compiler::OutputDirectory *output_directory
    ) const {

        // Open the insertion point to include headers
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_insertion(
            output_directory->OpenForInsert("rb_fastproto_init.cpp", "init_file_header")
        );
        google::protobuf::io::Printer header_insertion_printer(header_insertion.get(), '$');

        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> entrypoint_insertion(
            output_directory->OpenForInsert("rb_fastproto_init.cpp", "init_entrypoints")
        );
        google::protobuf::io::Printer entrypoint_insertion_printer(entrypoint_insertion.get(), '$');

        header_insertion_printer.Print(
            "#include \"$header$\"\n",
            "header", header_path_for_proto(file)
        );
        entrypoint_insertion_printer.Print(
            "rb_fastproto_gen::_Init_$init_method$();\n",
            "init_method", header_name_as_identifier(file)
        );
    }

    // Static method that writes the extconf.rb and rb_fastproto_init files
    void add_entrypoint_files(google::protobuf::compiler::CodeGeneratorResponse &response) {

        auto fastproto_header_file = response.add_file();
        fastproto_header_file->set_name("rb_fastproto_init.h");
        fastproto_header_file->set_content(R"EOS(
#include <ruby/ruby.h>
#include <functional>

#ifndef __RB_FASTPROTO_INIT_H
#define __RB_FASTPROTO_INIT_H

namespace rb_fastproto_gen {
    // The topleve ::Fastproto module
    extern VALUE rb_fastproto_module;

    // The base message class
    extern VALUE cls_fastproto_message;

    static inline unsigned int NUM2UINT_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2UINT(num);
    }

    static inline int NUM2INT_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2INT(num);
    }

    static inline unsigned long NUM2ULONG_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2ULONG(num);
    }

    static inline long NUM2LONG_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2LONG(num);
    }

    static inline bool VAL2BOOL_S(VALUE arg) {
        if (RB_TYPE_P(arg, T_TRUE)) {
            return true;
        } else if (RB_TYPE_P(arg, T_FALSE)) {
            return false;
        } else {
            rb_raise(rb_eTypeError, "Expected boolean");
            return false;
        }
    }

    static inline VALUE BOOL2VAL_S(bool arg) {
        return arg ? Qtrue : Qfalse;
    }
}

#endif
        )EOS");

        auto fastproto_init_file = response.add_file();
        fastproto_init_file->set_name("rb_fastproto_init.cpp");
        fastproto_init_file->set_content(R"EOS(
// Generated code that calls all the entrypoints
#include <cstdarg>
#include "rb_fastproto_init.h"

// @@protoc_insertion_point(init_file_header)

namespace rb_fastproto_gen {
    static void define_message_class();
    VALUE rb_fastproto_module = Qnil;
    VALUE cls_fastproto_message = Qnil;
}

extern "C" void Init_fastproto_gen(void) {

    // Define our toplevel module
    rb_fastproto_gen::rb_fastproto_module = rb_define_module("Fastproto");

    rb_fastproto_gen::define_message_class();

    // @@protoc_insertion_point(init_entrypoints)
}

namespace rb_fastproto_gen {
    static void define_message_class() {
        cls_fastproto_message = rb_define_class_under(rb_fastproto_module, "Message", rb_cObject);
    }
}
        )EOS");

        // Add an extconf.rb so it can be compiled
        auto extconf_file = response.add_file();
        extconf_file->set_name("extconf.rb");
        extconf_file->set_content(R"RUBY(
require 'mkmf'
dir_config('protobuf')
dir_config('boost')
$CXXFLAGS << ' --std=c++14 -Wno-shorten-64-to-32'
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
        )RUBY");
    }
}