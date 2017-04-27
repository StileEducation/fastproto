#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {
    void RBFastProtoCodeGenerator::write_header_method_struct_definition(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::io::Printer &printer
    ) const {
        auto class_name = cpp_proto_method_wrapper_struct_name_no_ns(method);

        printer.Print("// method $method_name$\n", "method_name", method->full_name());
        printer.Print("struct $class_name$ {\n", "class_name", class_name);
        printer.Indent();

        printer.Print(
            "static VALUE rb_cls;\n"
            "\n"
            "bool have_initialized;\n"
            "\n"
            "$class_name$(VALUE rb_self);\n"
            "~$class_name$() = default;\n"
            "\n"
            "static void initialize_class();\n"
            "static VALUE alloc(VALUE self);\n"
            "static VALUE alloc();\n"
            "static VALUE initialize(VALUE self);\n"
            "static void free(char* memory);\n"
            "static void mark(char* memory);\n"
            "\n"
            "static VALUE name(VALUE self);\n"
            "static VALUE proto_name(VALUE self);\n"
            "static VALUE request_class(VALUE self);\n"
            "static VALUE response_class(VALUE self);\n"
            "static VALUE service_class(VALUE self);\n",
            "class_name", class_name
        );

        printer.Outdent();
        printer.Print("};\n\n");
    }

    std::string RBFastProtoCodeGenerator::write_cpp_method_struct(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::io::Printer &printer
    ) const {
        auto class_name = cpp_proto_method_wrapper_struct_name(method);

        printer.Print(
            "\n"
            "// ----\n"
            "// begin method: $method_name$\n"
            "// ----\n"
            "\n",
            "method_name", method->full_name()
        );

        write_cpp_method_struct_constructor(file, method, class_name, printer);
        write_cpp_method_struct_static_initializer(file, method, class_name, printer);
        write_cpp_method_struct_allocators(file, method, class_name, printer);
        write_cpp_method_struct_name(file, method, class_name, printer);
        write_cpp_method_struct_classes(file, method, class_name, printer);

        printer.Print("VALUE $class_name$::rb_cls = Qnil;\n", "class_name", class_name);

        printer.Print(
            "\n"
            "// ----\n"
            "// end method: $method_name$\n"
            "// ----\n"
            "\n",
            "method_name", method->full_name()
        );

        return class_name;
    }

    void RBFastProtoCodeGenerator::write_cpp_method_struct_constructor(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "$class_name$::$constructor_name$(VALUE self) : have_initialized(true) {}\n\n",
            "class_name", class_name,
            "constructor_name", cpp_proto_method_wrapper_struct_name_no_ns(method)
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_method_struct_static_initializer(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print("void $class_name$::initialize_class() {\n", "class_name", class_name);
        printer.Indent();

        printer.Print(
            "rb_cls = rb_define_class_under(package_rb_module, \"$ruby_class_name$\", cls_fastproto_method);\n"
            "rb_define_alloc_func(rb_cls, &alloc);\n"
            "rb_define_method(rb_cls, \"initialize\", RUBY_METHOD_FUNC(&initialize), 0);\n"
            "rb_define_singleton_method(rb_cls, \"name\", RUBY_METHOD_FUNC(&name), 0);\n"
            "rb_define_singleton_method(rb_cls, \"proto_name\", RUBY_METHOD_FUNC(&proto_name), 0);\n"
            "rb_define_singleton_method(rb_cls, \"request_class\", RUBY_METHOD_FUNC(&request_class), 0);\n"
            "rb_define_singleton_method(rb_cls, \"response_class\", RUBY_METHOD_FUNC(&response_class), 0);\n"
            "rb_define_singleton_method(rb_cls, \"service_class\", RUBY_METHOD_FUNC(&service_class), 0);\n",
            "class_name", class_name,
            "ruby_class_name", method->name(),
            "method_full_name", method->full_name()
        );

        printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_method_struct_allocators(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "VALUE $class_name$::alloc(VALUE self) {\n"
            "  auto memory = ruby_xmalloc(sizeof($class_name$));\n"
            "  std::memset(memory, 0, sizeof($class_name$));\n"
            "  return Data_Wrap_Struct(self, &mark, &free, memory);\n"
            "}\n\n",
            "class_name", class_name
        );

        printer.Print(
            "VALUE $class_name$::alloc() {\n"
            "  return alloc(rb_path2class(\"$rb_class_name$\"));\n"
            "}\n\n",
            "class_name", class_name,
            "rb_class_name", ruby_proto_method_class_name(method)
        );

        printer.Print(
            "VALUE $class_name$::initialize(VALUE self) {\n"
            "  void* memory;\n"
            "  Data_Get_Struct(self, void*, memory);\n"
            "  new(memory) $class_name$(self);\n"
            "  return self;\n"
            "}\n\n",
            "class_name", class_name
        );

        printer.Print(
            "void $class_name$::free(char* memory) {\n"
            "  auto obj = reinterpret_cast<$class_name$*>(memory);\n"
            "  if (obj->have_initialized) {\n"
            "    obj->~$destructor_name$();\n"
            "  }\n"
            "  ruby_xfree(memory);\n"
            "}\n\n",
            "class_name", class_name,
            "destructor_name", cpp_proto_method_wrapper_struct_name_no_ns(method)
        );

        printer.Print(
            "void $class_name$::mark(char* memory) {}\n\n",
            "class_name", class_name
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_method_struct_name(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        boost::regex re("([a-z])([A-Z])");

        auto symbol_name = boost::regex_replace(method->name(), re, "$1_$2");
        boost::algorithm::to_lower(symbol_name);

        printer.Print(
            "VALUE $class_name$::name(VALUE self) {\n"
            "  return ID2SYM(rb_intern(\"$symbol_name$\"));\n"
            "}\n\n",
            "class_name", class_name,
            "symbol_name", symbol_name
        );

        printer.Print(
            "VALUE $class_name$::proto_name(VALUE self) {\n"
            "  return rb_obj_freeze(rb_str_new2(\"$method_name$\"));\n"
            "}\n\n",
            "class_name", class_name,
            "method_name", method->name()
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_method_struct_classes(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::MethodDescriptor* method,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        auto input_type = method->input_type();
        if (input_type == nullptr) {
            printer.Print(
                "VALUE $class_name$::request_class(VALUE self) {\n"
                "  return Qnil;\n"
                "}\n\n",
                "class_name", class_name
            );
        } else {
            printer.Print(
                "VALUE $class_name$::request_class(VALUE self) {\n"
                "  return $message_class$::rb_cls;\n"
                "}\n\n",
                "class_name", class_name,
                "message_class", cpp_proto_message_wrapper_struct_name(input_type)
            );
        }

        auto output_type = method->output_type();
        if (output_type == nullptr) {
            printer.Print(
                "VALUE $class_name$::response_class(VALUE self) {\n"
                "  return Qnil;\n"
                "}\n\n",
                "class_name", class_name
            );
        } else {
            printer.Print(
                "VALUE $class_name$::response_class(VALUE self) {\n"
                "  return $message_class$::rb_cls;\n"
                "}\n\n",
                "class_name", class_name,
                "message_class", cpp_proto_message_wrapper_struct_name(output_type)
            );
        }

        printer.Print(
            "VALUE $class_name$::service_class(VALUE self) {\n"
            "  return $message_class$::rb_cls;\n"
            "}\n\n",
            "class_name", class_name,
            "message_class", cpp_proto_service_wrapper_struct_name(method->service())
        );
    }
}
