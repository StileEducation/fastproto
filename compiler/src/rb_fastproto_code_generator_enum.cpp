#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {
    void RBFastProtoCodeGenerator::write_header_enum_struct_definition(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::EnumDescriptor* enum_type,
        google::protobuf::io::Printer &printer
    ) const {
        auto class_name = cpp_proto_enum_wrapper_struct_name_no_ns(enum_type);

        printer.Print("// enum $enum_name$\n", "enum_name", enum_type->full_name());
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
            "static VALUE fully_qualified_name(VALUE self);\n\n",
            "class_name", class_name
        );

        printer.Outdent();
        printer.Print("};\n\n");
    }

    std::string RBFastProtoCodeGenerator::write_cpp_enum_struct(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::EnumDescriptor* enum_type,
        google::protobuf::io::Printer &printer
    ) const {
        auto class_name = cpp_proto_enum_wrapper_struct_name(enum_type);

        printer.Print(
            "\n"
            "// ----\n"
            "// begin enum: $enum_name$\n"
            "// ----\n"
            "\n",
            "enum_name", enum_type->full_name()
        );

        write_cpp_enum_struct_constructor(file, enum_type, class_name, printer);
        write_cpp_enum_struct_static_initializer(file, enum_type, class_name, printer);
        write_cpp_enum_struct_allocators(file, enum_type, class_name, printer);
        write_cpp_enum_struct_name(file, enum_type, class_name, printer);

        printer.Print("VALUE $class_name$::rb_cls = Qnil;\n", "class_name", class_name);

        printer.Print(
            "\n"
            "// ----\n"
            "// end enum: $enum_name$\n"
            "// ----\n"
            "\n",
            "enum_name", enum_type->full_name()
        );

        return class_name;
    }

    void RBFastProtoCodeGenerator::write_cpp_enum_struct_constructor(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::EnumDescriptor* enum_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "$class_name$::$constructor_name$(VALUE self) : have_initialized(true) {}\n\n",
            "class_name", class_name,
            "constructor_name", cpp_proto_enum_wrapper_struct_name_no_ns(enum_type)
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_enum_struct_static_initializer(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::EnumDescriptor* enum_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print("void $class_name$::initialize_class() {\n", "class_name", class_name);
        printer.Indent();

        printer.Print(
            "rb_cls = rb_define_class_under(package_rb_module, \"$ruby_class_name$\", cls_fastproto_enum);\n"
            "rb_define_alloc_func(rb_cls, &alloc);\n"
            "rb_define_method(rb_cls, \"initialize\", RUBY_METHOD_FUNC(&initialize), 0);\n"
            "rb_define_singleton_method(rb_cls, \"fully_qualified_name\", RUBY_METHOD_FUNC(&fully_qualified_name), 0);\n\n",
            "class_name", class_name,
            "ruby_class_name", enum_type->name(),
            "enum_full_name", enum_type->full_name()
        );

        for (int i = 0; i < enum_type->value_count(); i++) {
            auto value = enum_type->value(i);

            printer.Print(
                "rb_define_const(rb_cls, \"$value_name$\", LONG2FIX($value_number$));\n",
                "value_name", value->name(),
                "value_number", std::to_string(value->number())
            );
        }

        printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_enum_struct_allocators(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::EnumDescriptor* enum_type,
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
            "rb_class_name", ruby_proto_enum_class_name(enum_type)
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
            "destructor_name", cpp_proto_enum_wrapper_struct_name_no_ns(enum_type)
        );

        printer.Print(
            "void $class_name$::mark(char* memory) {}\n\n",
            "class_name", class_name
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_enum_struct_name(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::EnumDescriptor* enum_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "VALUE $class_name$::fully_qualified_name(VALUE self) {\n"
            "  return rb_str_new2(\"$enum_full_name$\");\n"
            "}\n\n",
            "class_name", class_name,
            "enum_full_name", enum_type->full_name()
        );
    }
}
