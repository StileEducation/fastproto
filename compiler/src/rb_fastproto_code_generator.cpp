#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {

    RBFastProtoCodeGenerator::RBFastProtoCodeGenerator() { }

    bool RBFastProtoCodeGenerator::Generate(
        const google::protobuf::FileDescriptor *file,
        const std::string &parameter,
        google::protobuf::compiler::OutputDirectory *output_directory,
        std::string *error
    ) const {
        auto output_cpp_file_name = cpp_path_for_proto(file);
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> cpp_output(output_directory->Open(output_cpp_file_name));
        google::protobuf::io::Printer cpp_printer(cpp_output.get(), '$');

        auto output_header_file_name = header_path_for_proto(file);
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(output_directory->Open(output_header_file_name));
        google::protobuf::io::Printer header_printer(header_output.get(), '$');

        write_header(file, header_printer);
        write_cpp(file, cpp_printer);

        write_entrypoint(file, output_directory);

        return true;
    }

    void RBFastProtoCodeGenerator::write_header(
        const google::protobuf::FileDescriptor *file,
        google::protobuf::io::Printer &printer
    ) const {

        printer.Print(
            "#include <ruby/ruby.h>\n"
            "#include \"$pb_header_name$\"\n"
            "\n",
            "pb_header_name", cpp_proto_header_path_for_proto(file)
        );
        // Write an include guard in the header
        printer.Print(
            "#ifndef __$header_file_name$_H\n"
            "#define __$header_file_name$_H\n"
            "\n"
            "namespace rb_fastproto_gen {"
            "\n",
            "header_file_name", header_name_as_identifier(file)
        );

        // Add namespaces for the generated proto package
        auto namespace_parts = rubyised_namespace_els(file);
        for (auto&& ns : namespace_parts) {
            printer.Print("namespace $ns$ {\n", "ns", ns);
        }
        printer.Print("\n");

        printer.Indent(); printer.Indent();

        // We need to export our classes in the header file, so that other messages can
        // serialize this one as a part of their operation.
        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);
            write_header_message_struct_definition(file, message_type, printer);
        }

        // Define a method that the overall-init will call to define the ruby classes & modules contained
        // in this file.
        printer.Print(
            "void _Init();\n"
        );


        printer.Outdent(); printer.Outdent();
        for (auto&& ns : namespace_parts) {
            printer.Print("}\n");
        }
        printer.Print(
            "}\n"
            "#endif\n"
        );
    }


    void RBFastProtoCodeGenerator::write_header_message_struct_definition(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        google::protobuf::io::Printer &printer
    ) const {
        auto class_name = cpp_proto_wrapper_struct_name(message_type);
        printer.Print("struct $class_name$ {\n", "class_name", class_name);
        printer.Indent(); printer.Indent();

        // Fields that every message has
        printer.Print(
            "// Keep this so we know whether to delete a char array of memory, or delete the object\n"
            "// (thereby invoking its constructor)\n"
            "bool have_initialized;\n"
            "static VALUE rb_cls;\n"
        );
        // Write fields for the message field
        write_header_message_struct_fields(file, message_type, class_name, printer);

        // Constructor declarations.
        printer.Print(
            "\n\n"
            "// The default constructor will make a default message.\n"
            "$class_name$();\n"
            "~$class_name$() = default;\n"
            "\n",
            "class_name", class_name
        );

        // Static methods...
        printer.Print(
            "// methods whose implementations will control ruby lifecycle stuff\n"
            "static void initialize_class();\n"
            "static VALUE alloc(VALUE self);\n"
            "static VALUE initialize(VALUE self);\n"
            "static void free(char* memory);\n"
            "static void mark(char* memory);\n"
            "\n"
            "static VALUE validate(VALUE self);\n"
            "static VALUE serialize_to_string(VALUE self);\n"
            "static VALUE parse(VALUE self, VALUE buffer);\n"
            "\n"
            "VALUE to_proto_obj($cpp_proto_class$* cpp_proto);\n"
            "VALUE from_proto_obj(const $cpp_proto_class$& cpp_proto);\n",
            "cpp_proto_class", cpp_proto_class_name(message_type)
        );

        // various methods defined for each protobuf field, like getters & setters
        write_header_message_struct_accessors(file, message_type, class_name, printer);

        printer.Outdent(); printer.Outdent();
        printer.Print("};\n\n");
    }

    void RBFastProtoCodeGenerator::write_header_message_struct_fields(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Write a VALUE for each field to store the ruby-version of it.
        // Also a bool to store whether or not it is set.
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            printer.Print("VALUE field_$field_name$;\n", "field_name", field->name());
            if (field->is_optional()) {
                printer.Print("bool has_field_$field_name$;\n", "field_name", field->name());
            }
        }
    }

    void RBFastProtoCodeGenerator::write_header_message_struct_accessors(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            // Every method needs a getter, setter, and a factory for constructing a default
            printer.Print(
                "static VALUE get_$field_name$(VALUE self);\n"
                "static VALUE set_$field_name$(VALUE self, VALUE val);\n"
                "static VALUE default_factory_$field_name$(bool constructor);\n"
                "\n",
                "field_name", field->name()
            );

            // Optional fields need has_? too
            if (field->is_optional()) {
                printer.Print("static VALUE has_$field_name$(VALUE self);\n", "field_name", field->name());
            }
            printer.Print("\n");
        }
    }

    void RBFastProtoCodeGenerator::write_cpp(
        const google::protobuf::FileDescriptor *file,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "#include <ruby/ruby.h>\n"
            "#include <ruby/encoding.h>\n"
            "#include <ruby/thread.h>\n"
            "#include <limits>\n"
            "#include <type_traits>\n"
            "#include <new>\n"
            "#include <cstring>\n"
            "#include <functional>\n"
            "#include <tuple>\n"
            "#include <typeinfo>\n"
            "#include \"rb_fastproto_init.h\"\n"
            "#include \"$header_name$\"\n"
            "#include \"$pb_header_name$\"\n"
            "\n"
            "namespace rb_fastproto_gen {"
            "\n",
            "header_name", header_path_for_proto(file),
            "pb_header_name", cpp_proto_header_path_for_proto(file)
        );

        // Add namespaces for the generated proto package
        auto namespace_parts = rubyised_namespace_els(file);
        for (auto&& ns : namespace_parts) {
            printer.Print("namespace $ns$ {\n", "ns", ns);
        }
        printer.Print("\n");

        printer.Indent(); printer.Indent();

        printer.Print(
            "static VALUE package_rb_module = Qnil;\n"
        );
        // Define a struct for each message we have
        // Collect the class names that got written.
        std::vector<std::string> class_names;
        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);
            class_names.push_back(write_cpp_message_struct(file, message_type, printer));
        }

        // Write an Init function that gets called from the ruby gem init.
        write_cpp_message_module_init(file, class_names, printer);

        for (auto&& ns : namespace_parts) {
            printer.Print("}\n");
        }

        // Close off the rb_fastproto_gen namespace
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

    std::string RBFastProtoCodeGenerator::write_cpp_message_struct(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        google::protobuf::io::Printer &printer
    ) const {
        // This bit is responsible for writing out the message struct.

        auto class_name = cpp_proto_wrapper_struct_name(message_type);

        // The instance constructor for a protobuf message
        write_cpp_message_struct_constructor(file, message_type, class_name, printer);
        // a static method that defines this class in rubyland
        write_cpp_message_struct_static_initializer(file, message_type, class_name, printer);
        // Methods that return default values for each field
        write_cpp_message_struct_default_factories(file, message_type, class_name, printer);
        // Static accessors for each field, so ruby can call them
        write_cpp_message_struct_accessors(file, message_type, class_name, printer);
        // to and from proto object conversion
        write_cpp_message_struct_to_proto_obj(file, message_type, class_name, printer);
        write_cpp_message_struct_from_proto_obj(file, message_type, class_name, printer);
        // The message needs an alloc function, and a free function, and a mark function, for ruby.
        // It also needs a static initialize method to use as a factory.
        write_cpp_message_struct_allocators(file, message_type, class_name, printer);
        // Define validation methods
        write_cpp_message_struct_validator(file, message_type, class_name, printer);
        // Define serialization methods
        write_cpp_message_struct_serializer(file, message_type, class_name, printer);
        // Desserialization methods
        write_cpp_message_struct_parser(file, message_type, class_name, printer);

        // For some silly reason we need to initialized its static members at translation-unit scope?
        printer.Print("VALUE $class_name$::rb_cls = Qnil;\n", "class_name", class_name);

        return class_name;
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_constructor(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Object constructor; called in ruby initialize method.
        printer.Print(
            "$class_name$::$class_name$() : have_initialized(true) { \n",
            "class_name", class_name
        );

        // Initialize each field in the constructor.
        printer.Indent(); printer.Indent();
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            // Set the default value appropriately
            printer.Print("field_$field_name$ = default_factory_$field_name$(true);\n", "field_name", field->name());

            // If the field is optional, we need to set the has or not status of it
            if (field->is_optional()) {
                printer.Print("has_field_$field_name$ = false;\n", "field_name", field->name());
            }
        }
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_static_initializer(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Defines the static initialize_class method for this struct

        printer.Print(
            "void $class_name$::initialize_class() {\n",
            "class_name", class_name
        );
        printer.Indent(); printer.Indent();

        printer.Print(
            "rb_cls = rb_define_class_under(package_rb_module, \"$ruby_class_name$\", cls_fastproto_message);\n"
            "rb_define_alloc_func(rb_cls, &alloc);\n"
            "rb_define_method(rb_cls, \"initialize\", reinterpret_cast<VALUE(*)(...)>(&initialize), 0);\n"
            "rb_define_method(rb_cls, \"validate!\", reinterpret_cast<VALUE(*)(...)>(&validate), 0);\n"
            "rb_define_method(rb_cls, \"serialize_to_string\", reinterpret_cast<VALUE(*)(...)>(&serialize_to_string), 0);\n"
            "rb_define_method(rb_cls, \"parse\", reinterpret_cast<VALUE(*)(...)>(&parse), 1);\n"
            "\n",
            "ruby_class_name", message_type->name(),
            "class_name", class_name
        );

        // The ruby class needs accessors defined
        for(int i =  0; i < message_type->field_count(); i++) {
            auto field = message_type->field(i);

            printer.Print(
                "rb_define_method(rb_cls, \"$field_name$\", reinterpret_cast<VALUE(*)(...)>(&get_$field_name$), 0);\n"
                "rb_define_method(rb_cls, \"$field_name$=\", reinterpret_cast<VALUE(*)(...)>(&set_$field_name$), 1);\n",
                "field_name", field->name()
            );
            if (field->is_optional()) {
                printer.Print(
                    "rb_define_method(rb_cls, \"has_$field_name$?\", reinterpret_cast<VALUE(*)(...)>(&has_$field_name$), 0);\n",
                    "field_name", field->name()
                );
            }
        }

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_allocators(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // The message needs an alloc function, and a free function, and a mark function, for ruby.
        // It also needs a static initialize method to use as a factory.

        printer.Print(
            "VALUE $class_name$::alloc(VALUE self) {\n"
            "    auto memory = ruby_xmalloc(sizeof($class_name$));\n"
            "    // Important: It guarantees that reading have_initialized returns false so we know\n"
            "    // not to run the destructor\n"
            "    std::memset(memory, 0, sizeof($class_name$));\n"
            "    return Data_Wrap_Struct(self, &mark, &free, memory);\n"
            "}\n"
            "\n"
            "VALUE $class_name$::initialize(VALUE self) {\n"
            "    // Use placement new to create the object\n"
            "    void* memory;\n"
            "    Data_Get_Struct(self, void*, memory);\n"
            "    new(memory) $class_name$();\n"
            "    return self;\n"
            "}\n"
            "\n"
            "void $class_name$::free(char* memory) {\n"
            "    auto obj = reinterpret_cast<$class_name$*>(memory);\n"
            "    if (obj->have_initialized) {\n"
            "        obj->~$class_name$();\n"
            "    }\n"
            "    ruby_xfree(memory);\n"
            "}\n"
            "\n"
            "void $class_name$::mark(char* memory) {\n"
            "    auto cpp_this = reinterpret_cast<$class_name$*>(memory);\n"
            "\n",
            "class_name", class_name
        );

        // Mark each field
        printer.Indent(); printer.Indent();

        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            printer.Print("rb_gc_mark(cpp_this->field_$field_name$);\n", "field_name", field->name());
        }

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");

    }


    void RBFastProtoCodeGenerator::write_cpp_message_struct_default_factories(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Default factories for each field.
        // Method that just returns an appropriate default ruby VALUE.
        // Takes a parameter which is whether or not we are in a constructor (and hence should be lazy)


        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            printer.Print(
                "VALUE $class_name$::default_factory_$field_name$(bool constructor) {\n",
                "field_name", field->name(),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            if (field->is_repeated()) {
                // Repeated fields just default to an empty array
                printer.Print("return rb_ary_new();\n");
            } else {
                switch (field->type()) {
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                        printer.Print("return LONG2NUM(0);\n");
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                    case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                        printer.Print("return DBL2NUM(0.0);\n");
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                        printer.Print("return Qfalse;\n");
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                        printer.Print("return rb_str_new2(\"\");\n");
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                        printer.Print(
                            "auto r_str = rb_str_new2(\"\");\n"
                            "rb_enc_associate_index(r_str, rb_enc_find_index(\"UTF-8\"));\n"
                            "return r_str;\n"
                        );
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
                        // TODO: What happens here?
                        printer.Print("return Qnil;\n");
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                    case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
                        // If it is optional, just write nil when in the constructor and we will fault it in on read.
                        // This prevents stack overflows.
                        printer.Print(
                            "if ($is_required$ || !constructor) {\n"
                            "    auto obj = rb_funcall(\n"
                            "        rb_path2class(\"$rb_message_class_name$\"),\n"
                            "        rb_intern(\"new\"), 0\n"
                            "    );\n"
                            "    return obj;\n"
                            "} else {\n"
                            "    return Qnil;\n"
                            "}\n",
                            "is_required", field->is_required() ? "true" : "false",
                            "rb_message_class_name", ruby_proto_class_name(field->message_type())

                        );
                    default:
                        break;
                }
            }

            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");
        }
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_accessors(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // The accessors are really simple, just setting the VALUE and has status of fields.
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            printer.Print(
                "VALUE $class_name$::set_$field_name$(VALUE self, VALUE val) {\n"
                "    $class_name$* cpp_self;\n"
                "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                "\n",
                "field_name", field->name(),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            // If nil was provided, interpret that as "unset"
            printer.Print("if (val == Qnil) {\n");
            printer.Indent(); printer.Indent();

            printer.Print("cpp_self->field_$field_name$ = default_factory_$field_name$(false);\n", "field_name", field->name());
            if (field->is_optional()) {
                printer.Print("cpp_self->has_field_$field_name$ = false;\n", "field_name", field->name());
            }

            printer.Outdent(); printer.Outdent();

            // Otherwise, we need to set our internal VALUE to what was provided.
            // No type checking is done at this stage.
            printer.Print("} else {\n");
            printer.Indent(); printer.Indent();

            printer.Print("cpp_self->field_$field_name$ = val;\n", "field_name", field->name());

            if (field->is_optional()) {
                printer.Print("cpp_self->has_field_$field_name$ = true;\n", "field_name", field->name());
            }


            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");

            printer.Print("return Qnil;\n");
            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");


            // We also need a getter.
            printer.Print(
                "VALUE $class_name$::get_$field_name$(VALUE self) {\n"
                "    $class_name$* cpp_self;\n"
                "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                "\n",
                "field_name", field->name(),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            // If the field is a message, and currently nil, we need to fault it in
            if (field->message_type()) {
                printer.Print(
                    "if (cpp_self->field_$field_name$ == Qnil) {\n"
                    "    cpp_self->field_$field_name$ = default_factory_$field_name$(false);\n"
                    "}\n",
                    "field_name", field->name()
                );
            }
            printer.Print("return cpp_self->field_$field_name$;\n", "field_name", field->name());

            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");

            // Maybe a has_? as well
            if (field->is_optional()) {
                printer.Print(
                    "VALUE $class_name$::has_$field_name$(VALUE self) {\n"
                    "    $class_name$* cpp_self;\n"
                    "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                    "    return cpp_self->has_field_$field_name$ ? Qtrue : Qfalse; \n"
                    "}\n\n",
                    "field_name", field->name(),
                    "class_name", class_name
                );
            }
        }
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_to_proto_obj(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Implements to_proto_obj which fills an appropriately classed google C++
        // protobuf object with the VALUES from this object.
        // Returns any exception that happened, or Qnil.

        printer.Print(
            "VALUE $class_name$::to_proto_obj($cpp_proto_type$* cpp_proto) {\n",
            "cpp_proto_type", cpp_proto_class_name(message_type),
            "class_name", class_name
        );
        printer.Indent(); printer.Indent();

        // The gymnastics with the the dangerous_func lambda is so we can catch any exception
        // that gets raised in the conversion macro return it to our caller, so we can run destructors
        // before re-raising the exception to ruby.
        printer.Print(
            "struct dangerous_func_data {\n"
            "    decltype(cpp_proto) cpp_proto;\n"
            "    $class_name$* self;\n"
            "};\n"
            "auto dangerous_func = [](VALUE data_as_value) -> VALUE {\n"
            "    // Unwrap our 'VALUE' into an actual pointer\n"
            "    auto data = reinterpret_cast<dangerous_func_data*>(NUM2ULONG(data_as_value));\n"
            "    auto _cpp_proto = data->cpp_proto;\n"
            "    auto _self = data->self;\n",
            "class_name", class_name
        );

        printer.Indent(); printer.Indent();

        // OK. Now, carefully, without allocating memory on the heap, set each of our fields onto the proto
        // object. If there is a type mismatch, we get longjmp()'d out into the ruby exception handler.
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            if (field->is_repeated()) {
                // TODO...
            } else {
                // Do nothing if the field is not set
                if (field->is_required()) {
                    printer.Print("if (true) {\n");
                } else {
                    printer.Print("if (_self->has_field_$field_name$) {\n", "field_name", field->name());
                }
                printer.Indent(); printer.Indent();

                // Depending on the ruby class, we do a different conversion
                // The numeric _S macros are defined in generator_entrypoint.cpp
                // and wrap the ruby macros to disable float-to-int coersion
                switch (field->type()) {
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
                        printer.Print("_cpp_proto->set_$field_name$(NUM2INT_S(_self->field_$field_name$));\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                    printer.Print("_cpp_proto->set_$field_name$(NUM2UINT_S(_self->field_$field_name$));\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
                    printer.Print("_cpp_proto->set_$field_name$(NUM2LONG_S(_self->field_$field_name$));\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                    printer.Print("_cpp_proto->set_$field_name$(NUM2ULONG_S(_self->field_$field_name$));\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                        printer.Print(
                            "_cpp_proto->set_$field_name$(static_cast<float>(NUM2DBL(_self->field_$field_name$)));\n",
                            "field_name", field->name()
                        );
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                        printer.Print(
                            "_cpp_proto->set_$field_name$(NUM2DBL(_self->field_$field_name$));\n",
                            "field_name", field->name()
                        );
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                        printer.Print("_cpp_proto->set_$field_name$(VAL2BOOL_S(_self->field_$field_name$));\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                    case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                        // Pass two arguments to set_stringfield()
                        printer.Print(
                            "_cpp_proto->set_$field_name$(\n"
                            "    RSTRING_PTR(_self->field_$field_name$),\n"
                            "    RSTRING_LEN(_self->field_$field_name$)\n"
                            ");\n",
                            "field_name", field->name()
                        );
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                    case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
                        // Recurse to serialize the message
                        // TODO: Needs to access the type of the dependant message.
                        break;
                    default:
                        break;
                }

                printer.Outdent(); printer.Outdent();
                printer.Print("} else {\n"); // close of if (required | set)
                printer.Print("    _cpp_proto->clear_$field_name$();\n", "field_name", field->name());
                printer.Print("}\n");
            }
        }

        printer.Print("return Qnil;\n");
        printer.Outdent(); printer.Outdent();

        printer.Print(
            "};\n"
            "int exc_status;\n"
            "dangerous_func_data data_struct;\n"
            "data_struct.cpp_proto = cpp_proto;\n"
            "data_struct.self = this;\n"
            "rb_protect(dangerous_func, ULONG2NUM(reinterpret_cast<uint64_t>(&data_struct)), &exc_status);\n"
            "if (exc_status) {\n"
            "    // Exception!\n"
            "    auto err = rb_errinfo();\n"
            "    rb_set_errinfo(Qnil);\n"
            "    return err;\n"
            "} else {\n"
            "    // cpp_proto has its field set\n"
            "    return Qnil;\n"
            "}\n"
        );

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_from_proto_obj(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Defines a from_proto_obj that takes a protobuf object and sets our VALUES
        // from it.
        printer.Print(
            "VALUE $class_name$::from_proto_obj(const $cpp_proto_type$& cpp_proto) {\n",
            "cpp_proto_type", cpp_proto_class_name(message_type),
            "class_name", class_name
        );
        printer.Indent(); printer.Indent();

        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            if (field->is_repeated()) {
                // TODO
            } else {
                // Do nothing if the field is not set
                if (field->is_required()) {
                    printer.Print("if (true) {\n");
                } else {
                    printer.Print("if (cpp_proto.has_$field_name$()) {\n", "field_name", field->name());
                }
                printer.Indent(); printer.Indent();

                switch (field->type()) {
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
                        printer.Print("field_$field_name$ = INT2NUM(cpp_proto.$field_name$());\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                        printer.Print("field_$field_name$ = UINT2NUM(cpp_proto.$field_name$());\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
                        printer.Print("field_$field_name$ = LONG2NUM(cpp_proto.$field_name$());\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                        printer.Print("field_$field_name$ = ULONG2NUM(cpp_proto.$field_name$());\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                    case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                        printer.Print("field_$field_name$ = DBL2NUM(cpp_proto.$field_name$());\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                        printer.Print("field_$field_name$ = BOOL2VAL_S(cpp_proto.$field_name$());\n", "field_name", field->name());
                        break;
                    case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                    case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                        // Important to use the length, because .data() also has a terminating
                        // null byte.
                        printer.Print(
                            "field_$field_name$ = rb_str_new(cpp_proto.$field_name$().data(), cpp_proto.$field_name$().length());\n",
                            "field_name", field->name()
                        );
                        break;
                    default:
                        break;
                }

                if (field->is_optional()) {
                    printer.Print("has_field_$field_name$ = true;\n", "field_name", field->name());
                }


                printer.Outdent(); printer.Outdent();
                printer.Print("} else {\n"); // close of if (required | set)
                if (field->is_optional()) {
                    printer.Print("   has_field_$field_name$ = false;\n", "field_name", field->name());
                }
                printer.Print("}\n");
            }
        }

        printer.Print("return Qnil;\n");
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_validator(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Implements validate!
        printer.Print(
            "VALUE $class_name$::validate(VALUE self) {\n"
            "    VALUE ex;\n"
            "    {\n"
            "        $class_name$* cpp_self;\n"
            "        Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "        $cpp_proto_class$ cpp_proto;\n"
            "        ex = cpp_self->to_proto_obj(&cpp_proto);\n"
            "        if (ex != Qnil) {\n"
            "            goto raise;\n"
            "        }\n"
            "        return Qnil;\n"
            "    }\n"
            "    raise:\n"
            "        rb_exc_raise(ex);\n"
            "        return Qnil;\n"
            "}\n\n",
            "class_name", class_name,
            "cpp_proto_class", cpp_proto_class_name(message_type)
        );

    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_serializer(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {

        printer.Print(
            "VALUE $class_name$::serialize_to_string(VALUE self) {\n"
            "    VALUE ex;\n"
            "    // More function pointer hax to avoid GVL...\n"
            "    struct serialize_args {\n"
            "        $cpp_proto_class$* cpp_proto;\n"
            "        size_t pb_size;\n"
            "        char* rb_buffer_ptr;\n"
            "    };\n"
            "\n"
            "    {\n"
            "        $class_name$* cpp_self;\n"
            "        Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "\n"
            "        serialize_args args;\n"
            "        $cpp_proto_class$ cpp_proto;\n"
            "        ex = cpp_self->to_proto_obj(&cpp_proto);\n"
            "        if (ex != Qnil) {\n"
            "            goto raise;\n"
            "        }\n"
            "        args.cpp_proto = &cpp_proto;\n"
            "        args.pb_size = cpp_proto.ByteSize();\n"
            "        VALUE rb_str = rb_str_new(\"\", 0);\n"
            "        rb_str_resize(rb_str, args.pb_size);\n"
            "        args.rb_buffer_ptr = RSTRING_PTR(rb_str);\n"
            "        // We want to call proto.SerializeToArray outside of the ruby GVL.\n"
            "        // We can't convert a lambda that captures to a function pointer, so we\n"
            "        // do stupid struct hacks.\n"
            "        rb_thread_call_without_gvl(\n"
            "            [](void* _args_void) -> void* {\n"
            "                auto _args = reinterpret_cast<serialize_args*>(_args_void);\n"
            "                _args->cpp_proto->SerializeToArray(_args->rb_buffer_ptr, static_cast<int>(_args->pb_size));\n"
            "                return nullptr;\n"
            "            },\n"
            "            &args, RUBY_UBF_IO, nullptr\n"
            "        );\n"
            "        return rb_str;\n"
            "    }\n"
            "    raise:\n"
            "        rb_exc_raise(ex);\n"
            "        return Qnil;\n"
            "}\n\n",
            "class_name", class_name,
            "cpp_proto_class", cpp_proto_class_name(message_type)
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_parser(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {

        printer.Print(
            "VALUE $class_name$::parse(VALUE self, VALUE buffer) {\n"
            "    // VALUE ex;\n"
            "    // More function pointer hax to avoid GVL...\n"
            "    struct parse_args {\n"
            "        $cpp_proto_class$* cpp_proto;\n"
            "        size_t pb_size;\n"
            "        char* rb_buffer_ptr;\n"
            "    };\n"
            "\n"
            "    {\n"
            "        $class_name$* cpp_self;\n"
            "        Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "        parse_args args;"
            "        args.pb_size = RSTRING_LEN(buffer);\n"
            "        args.rb_buffer_ptr = RSTRING_PTR(buffer);\n"
            "        $cpp_proto_class$ cpp_proto;\n"
            "        args.cpp_proto = &cpp_proto;\n"
            "\n"
            "        // We want to call proto.ParseFromArray outside of the ruby GVL.\n"
            "        // We can't convert a lambda that captures to a function pointer, so we\n"
            "        // do stupid struct hacks.\n"
            "        rb_thread_call_without_gvl(\n"
            "            [](void* _args_void) -> void* {\n"
            "                auto _args = reinterpret_cast<parse_args*>(_args_void);\n"
            "                _args->cpp_proto->ParseFromArray(_args->rb_buffer_ptr, static_cast<int>(_args->pb_size));\n"
            "                return nullptr;\n"
            "            },\n"
            "            &args, RUBY_UBF_IO, nullptr\n"
            "        );\n"
            "        cpp_self->from_proto_obj(cpp_proto);\n"
            "        return Qnil;\n"
            "    }\n"
            "    // raise:\n"
            "    //    rb_exc_raise(ex);\n"
            "    //    return Qnil;\n"
            "}\n\n",
            "class_name", class_name,
            "cpp_proto_class", cpp_proto_class_name(message_type)
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_message_module_init(
        const google::protobuf::FileDescriptor* file,
        const std::vector<std::string> &class_names,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "void _Init() {\n"
        );
        printer.Indent(); printer.Indent();

        // We need to define a ruby module for the package to store message classes in.
        auto package_elements = rubyised_namespace_els(file);
        // The first scope is special because we need to use rb_define_module, not rb_define_module_under
        printer.Print(
            "package_rb_module = rb_define_module(\"$package_el$\");\n",
            "package_el", package_elements[0]
        );
        for (int i = 1; i < package_elements.size(); i++) {
            printer.Print(
                "package_rb_module = rb_define_module_under(package_rb_module, \"$package_el$\");\n",
                "package_el", package_elements[i]
            );
        }

        for (auto&& class_name : class_names) {

            printer.Print(
                "$class_name$::initialize_class();\n",
                "class_name", class_name
            );
        }

        // Close off the _Init_header_name function
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

}
