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
            "#include <vector>\n"
            "#include <utility>\n"
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
        auto class_name = cpp_proto_wrapper_struct_name_no_ns(message_type);
        printer.Print("struct $class_name$ {\n", "class_name", class_name);
        printer.Indent(); printer.Indent();

        // Fields that every message has
        printer.Print(
            "// Keep this so we know whether to delete a char array of memory, or delete the object\n"
            "// (thereby invoking its constructor)\n"
            "bool have_initialized;\n"
            "static VALUE rb_cls;\n"
            "bool is_default_value;\n"
        );
        // Write fields for the message field
        write_header_message_struct_fields(file, message_type, class_name, printer);

        // Constructor declarations.
        printer.Print(
            "\n\n"
            "// The default constructor will make a default message.\n"
            "$class_name$(VALUE rb_self);\n"
            "~$class_name$() = default;\n"
            "\n",
            "class_name", class_name
        );

        // Static methods...
        printer.Print(
            "// methods whose implementations will control ruby lifecycle stuff\n"
            "static void initialize_class();\n"
            "static VALUE alloc(VALUE self);\n"
            "static VALUE alloc();\n"
            "static VALUE initialize(int argc, VALUE* argv, VALUE self);\n"
            "static void free(char* memory);\n"
            "static void mark(char* memory);\n"
            "\n"
            "static VALUE validate(VALUE self);\n"
            "static VALUE serialize_to_string(VALUE self);\n"
            "static VALUE serialize_to_string_with_gvl(VALUE self);\n"
            "static VALUE parse(VALUE self, VALUE buffer);\n"
            "static VALUE value_for_tag(VALUE self, VALUE tag);\n"
            "static VALUE set_value_for_tag(VALUE self, VALUE tag, VALUE val);\n"
            "static VALUE has_value_for_tag(VALUE self, VALUE tag);\n"
            "static VALUE get_nested(int argc, VALUE* argv, VALUE self);\n"
            "static VALUE get_nested_bang(int argc, VALUE* argv, VALUE self);\n"
            "static VALUE notify_default_changed(VALUE self, VALUE sender, VALUE notify_tag);\n"
            "\n"
            "VALUE to_proto_obj($cpp_proto_class$* cpp_proto);\n"
            "VALUE from_proto_obj(const $cpp_proto_class$& cpp_proto);\n",
            "cpp_proto_class", cpp_proto_class_name(message_type)
        );

        // various methods defined for each protobuf field, like getters & setters
        write_header_message_struct_accessors(file, message_type, class_name, printer);

        // Define any submessages
        for (int i = 0; i < message_type->nested_type_count(); i++ ) {
            write_header_message_struct_definition(file, message_type->nested_type(i), printer);
        }

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

            printer.Print("VALUE field_$field_name$;\n", "field_name", cpp_field_name(field));
            if (field->is_optional()) {
                printer.Print("bool has_field_$field_name$;\n", "field_name", cpp_field_name(field));
            }
        }

        // Add storage for unknown fields
        // Note that this is not exposed to ruby, except if you deserialize unknown fields,
        // they will be serialized again.
        printer.Print("google::protobuf::UnknownFieldSet unknown_fields;\n");
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
                "static VALUE has_$field_name$(VALUE self);\n"
                "static VALUE default_factory_$field_name$(VALUE self, bool constructor);\n"
                "\n",
                "field_name", cpp_field_name(field)
            );

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
        // Dynamic value_for_tag methods
        write_cpp_message_struct_dynamic_accessors(file, message_type, class_name, printer);
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

        // Write the implementation for all submessages to
        for (int i = 0; i < message_type->nested_type_count(); i++ ) {
            write_cpp_message_struct(file, message_type->nested_type(i), printer);
        }

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
            "$class_name$::$constructor_name$(VALUE rb_self) : have_initialized(true), is_default_value(true) { \n",
            "class_name", class_name,
            "constructor_name", cpp_proto_wrapper_struct_name_no_ns(message_type)
        );

        // Initialize each field in the constructor.
        printer.Indent(); printer.Indent();
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            // Set the default value appropriately
            printer.Print(
                "field_$field_name$ = default_factory_$field_name$(rb_self, true);\n",
                "field_name", cpp_field_name(field)
            );

            // If the field is optional, we need to set the has or not status of it
            if (field->is_optional()) {
                printer.Print("has_field_$field_name$ = false;\n", "field_name", cpp_field_name(field));
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
            "rb_cls = rb_define_class_under($ruby_namespace$, \"$ruby_class_name$\", cls_fastproto_message);\n"
            "rb_define_alloc_func(rb_cls, &alloc);\n"
            "rb_define_method(rb_cls, \"initialize\", RUBY_METHOD_FUNC(&initialize), -1);\n"
            "rb_define_method(rb_cls, \"validate!\", RUBY_METHOD_FUNC(&validate), 0);\n"
            "rb_define_method(rb_cls, \"serialize_to_string\", RUBY_METHOD_FUNC(&serialize_to_string), 0);\n"
            "rb_define_method(rb_cls, \"serialize_to_string_with_gvl\", RUBY_METHOD_FUNC(&serialize_to_string_with_gvl), 0);\n"
            "rb_define_method(rb_cls, \"parse\", RUBY_METHOD_FUNC(&parse), 1);\n"
            "rb_define_method(rb_cls, \"value_for_tag\", RUBY_METHOD_FUNC(&value_for_tag), 1);\n"
            "rb_define_method(rb_cls, \"set_value_for_tag\", RUBY_METHOD_FUNC(&set_value_for_tag), 2);\n"
            "rb_define_method(rb_cls, \"value_for_tag?\", RUBY_METHOD_FUNC(&has_value_for_tag), 1);\n"
            "rb_define_method(rb_cls, \"get\", RUBY_METHOD_FUNC(&get_nested), -1);\n"
            "rb_define_method(rb_cls, \"get!\", RUBY_METHOD_FUNC(&get_nested_bang), -1);\n"
            "rb_define_method(rb_cls, \"notify_default_changed\", RUBY_METHOD_FUNC(&notify_default_changed), 2);\n"
            "\n",
            "ruby_namespace", message_type->containing_type() == nullptr ?
                "package_rb_module" :
                cpp_proto_wrapper_struct_name(message_type->containing_type()) + "::rb_cls",
            "ruby_class_name", message_type->name(),
            "class_name", class_name
        );

        // The ruby class needs accessors defined
        for(int i =  0; i < message_type->field_count(); i++) {
            auto field = message_type->field(i);

            printer.Print(
                "rb_define_method(rb_cls, \"$field_name$\", RUBY_METHOD_FUNC(&get_$field_name$), 0);\n"
                "rb_define_method(rb_cls, \"$field_name$=\", RUBY_METHOD_FUNC(&set_$field_name$), 1);\n"
                "rb_define_method(rb_cls, \"has_$field_name$?\", RUBY_METHOD_FUNC(&has_$field_name$), 0);\n",
                "field_name", cpp_field_name(field)
            );
        }

        // Initialize any submessages too
        for (int i = 0; i < message_type->nested_type_count(); i++ ) {
            printer.Print("$subtype$::initialize_class();\n", "subtype", cpp_proto_wrapper_struct_name(message_type->nested_type(i)));
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
            "VALUE $class_name$::alloc() {\n"
            "    return alloc(rb_path2class(\"$rb_class_name$\"));\n"
            "}\n"
            "VALUE $class_name$::initialize(int argc, VALUE* argv, VALUE self) {\n"
            "    // Use placement new to create the object\n"
            "    void* memory;\n"
            "    Data_Get_Struct(self, void*, memory);\n"
            "    new(memory) $class_name$(self);\n"
            "    // If we got passed a hash, set attributes with it.\n"
            "    VALUE attrs = Qnil;\n"
            "    rb_scan_args(argc, argv, \"01\", &attrs);\n"
            "    if (attrs != Qnil) {\n"
            "        // Could longjmp(); don't think there's anything to protect in here.\n"
            "        // Ruby now knows about our VALUE and will GC it if needed, calling ::free()\n"
            "        Check_Type(attrs, T_HASH);\n"
            "        // If you skip the static_cast, the RUBY_METHOD_FUNC will do a reinterpret_cast on the std::function to a function pointer. Not valid.\n"
            "        // However, here, the compiler knows how to static_cast to a function pointer, which can then be reinterpret_cast to the right pointer type\n"
            "        auto key_iterator = static_cast<VALUE(*)(VALUE, VALUE, int, VALUE*)>(\n"
            "            [](VALUE block_arg, VALUE _self, int argc, VALUE* argv) -> VALUE {\n"
            "\n"
            "            if (argc != 1) rb_raise(rb_eRuntimeError, \"wtf? - rb_fastproto_code_generator.cpp:393(~ish)\");\n"
            "            Check_Type(argv[0], T_ARRAY);"
            "            VALUE key = rb_ary_entry(argv[0], 0);\n"
            "            VALUE value = rb_ary_entry(argv[0], 1);\n"
            "            // stringify key if need be\n"
            "            if (TYPE(key) == T_SYMBOL) {\n"
            "                key = rb_funcall(key, rb_intern(\"to_s\"), 0);\n"
            "            }\n"
            "            Check_Type(key, T_STRING);\n"
            "            ID _assign_key; { _assign_key = rb_intern(\n"
            "                (std::string(RSTRING_PTR(key), RSTRING_LEN(key)) + \"=\").c_str()\n"
            "            ); }\n"
            "            return rb_funcall(_self, _assign_key, 1, value);\n"
            "        });\n"
            "        rb_block_call(attrs, rb_intern(\"each\"), 0, nullptr, RUBY_METHOD_FUNC(key_iterator), self);\n"
            "    }\n"
            "    return self;\n"
            "}\n"
            "\n"
            "void $class_name$::free(char* memory) {\n"
            "    auto obj = reinterpret_cast<$class_name$*>(memory);\n"
            "    if (obj->have_initialized) {\n"
            "        obj->~$destructor_name$();\n"
            "    }\n"
            "    ruby_xfree(memory);\n"
            "}\n"
            "\n"
            "void $class_name$::mark(char* memory) {\n"
            "    auto cpp_this = reinterpret_cast<$class_name$*>(memory);\n"
            "\n",
            "class_name", class_name,
            "destructor_name", cpp_proto_wrapper_struct_name_no_ns(message_type),
            "rb_class_name", ruby_proto_class_name(message_type)
        );

        // Mark each field
        printer.Indent(); printer.Indent();

        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            printer.Print("rb_gc_mark(cpp_this->field_$field_name$);\n", "field_name", cpp_field_name(field));
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
                "VALUE $class_name$::default_factory_$field_name$(VALUE self, bool constructor) {\n",
                "field_name", cpp_field_name(field),
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
                            "    // obj = rb_obj_freeze(obj);\n"
                            "    // Set the parent_for_notify to be us - we will find out when one of its subfields\n"
                            "    // changes, so we can update is_set for optional fields.\n"
                            "    rb_ivar_set(obj, rb_intern(\"@parent_for_notify\"), self);\n"
                            "    rb_ivar_set(obj, rb_intern(\"@tag_for_notify\"), INT2NUM($field_number$));"
                            "    return obj;\n"
                            "} else {\n"
                            "    return Qnil;\n"
                            "}\n",
                            "is_required", field->is_required() ? "true" : "false",
                            "rb_message_class_name", ruby_proto_class_name(field->message_type()),
                            "field_name", cpp_field_name(field),
                            "field_number", std::to_string(field->number())
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
                "field_name", cpp_field_name(field),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            printer.Print(
                "if (RB_OBJ_FROZEN(self)) {\n"
                "    rb_raise(rb_eRuntimeError, \"Message is frozen\");\n"
                "    return Qnil;\n"
                "}\n"
            );

            // If nil was provided, interpret that as "unset"
            printer.Print("if (val == Qnil) {\n");
            printer.Indent(); printer.Indent();

            printer.Print("cpp_self->field_$field_name$ = default_factory_$field_name$(self, false);\n", "field_name", cpp_field_name(field));
            if (field->is_optional()) {
                printer.Print("cpp_self->has_field_$field_name$ = false;\n", "field_name", cpp_field_name(field));
            }

            printer.Outdent(); printer.Outdent();

            // Otherwise, we need to set our internal VALUE to what was provided.
            // No type checking is done at this stage.
            printer.Print("} else {\n");
            printer.Indent(); printer.Indent();

            printer.Print("cpp_self->field_$field_name$ = val;\n", "field_name", cpp_field_name(field));

            if (field->is_optional()) {
                printer.Print("cpp_self->has_field_$field_name$ = true;\n", "field_name", cpp_field_name(field));
            }

            printer.Print(
                "if (cpp_self->is_default_value) {\n"
                "    cpp_self->is_default_value = false;\n"
                "    if (rb_ivar_get(self, rb_intern(\"@parent_for_notify\"))) {\n"
                "        VALUE parent_for_notify = rb_ivar_get(self, rb_intern(\"@parent_for_notify\"));\n"
                "        if (parent_for_notify != Qnil) {\n"
                "            VALUE notify_tag = rb_ivar_get(self, rb_intern(\"@tag_for_notify\"));\n"
                "            rb_funcall(parent_for_notify, rb_intern(\"notify_default_changed\"), 2, self, notify_tag);\n"
                "            rb_ivar_set(self, rb_intern(\"@parent_for_notify\"), Qnil);\n"
                "        }\n"
                "    }\n"
                "}\n"
            );

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
                "field_name", cpp_field_name(field),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            // If the field is a message, and currently nil, we need to fault it in
            if (field->message_type()) {
                printer.Print(
                    "if (cpp_self->field_$field_name$ == Qnil) {\n"
                    "    cpp_self->field_$field_name$ = default_factory_$field_name$(self, false);\n"
                    "}\n",
                    "field_name", cpp_field_name(field)
                );
            }
            printer.Print("return cpp_self->field_$field_name$;\n", "field_name", cpp_field_name(field));

            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");

            // Maybe a has_? as well
            printer.Print(
                "VALUE $class_name$::has_$field_name$(VALUE self) {\n",
                "field_name", cpp_field_name(field),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            if (field->is_optional()) {
                printer.Print(
                    "$class_name$* cpp_self;\n"
                    "Data_Get_Struct(self, $class_name$, cpp_self);\n"
                    "return cpp_self->has_field_$field_name$ ? Qtrue : Qfalse; \n",
                    "field_name", cpp_field_name(field),
                    "class_name", class_name
                );
            } else {
                printer.Print("return Qtrue;\n");
            }

            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");
        }
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_dynamic_accessors(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "VALUE $class_name$::value_for_tag(VALUE self, VALUE tag) {\n"
            "    Check_Type(tag, T_FIXNUM);\n"
            "    auto field_descriptor = $cpp_proto_type$::descriptor()->FindFieldByNumber(NUM2INT(tag));\n"
            "    if (field_descriptor == nullptr) {\n"
            "        rb_raise(rb_eKeyError, \"Tag not found\");\n"
            "        return Qnil;\n"
            "    }\n"
            "    auto method = rb_intern(field_descriptor->name().c_str());\n"
            "    return rb_funcall(self, method, 0);\n"
            "}\n"
            "VALUE $class_name$::set_value_for_tag(VALUE self, VALUE tag, VALUE val) {\n"
            "    Check_Type(tag, T_FIXNUM);\n"
            "    auto field_descriptor = $cpp_proto_type$::descriptor()->FindFieldByNumber(NUM2INT(tag));\n"
            "    if (field_descriptor == nullptr) {\n"
            "        rb_raise(rb_eKeyError, \"Tag not found\");\n"
            "        return Qnil;\n"
            "    }\n"
            "    auto method = rb_intern(std::string(field_descriptor->name() + \"=\").c_str());\n"
            "    return rb_funcall(self, method, 1, val);\n"
            "}\n"
            "VALUE $class_name$::has_value_for_tag(VALUE self, VALUE tag) {\n"
            "    Check_Type(tag, T_FIXNUM);\n"
            "    auto field_descriptor = $cpp_proto_type$::descriptor()->FindFieldByNumber(NUM2INT(tag));\n"
            "    if (field_descriptor == nullptr) {\n"
            "        rb_raise(rb_eKeyError, \"Tag not found\");\n"
            "        return Qnil;\n"
            "    }\n"
            "    ID method; { method = rb_intern((std::string(\"has_\") + field_descriptor->name() + \"?\").c_str()); }\n"
            "    return rb_funcall(self, method, 0);\n"
            "}\n"
            "VALUE $class_name$::get_nested(int argc, VALUE* argv, VALUE self) {\n"
            "    VALUE field_sym = Qnil;\n"
            "    VALUE rest = Qnil;\n"
            "    rb_scan_args(argc, argv, \"1*\", &field_sym, &rest);\n"
            "    ID field_sym_id;\n"
            "    ID has_field_sym_id;\n"
            "    if (TYPE(field_sym) == T_STRING) {\n"
            "        field_sym_id = rb_intern_str(field_sym);\n"
            "        std::string has_f(\"has_\");\n"
            "        has_f += std::string(RSTRING_PTR(field_sym), RSTRING_LEN(field_sym));\n"
            "        has_f += \"?\";\n"
            "        has_field_sym_id = rb_intern(has_f.c_str());\n"
            "    } else if (TYPE(field_sym) == T_SYMBOL) {\n"
            "        field_sym_id = SYM2ID(field_sym);\n"
            "        VALUE field_sym_str = rb_funcall(field_sym, rb_intern(\"to_s\"), 0);\n"
            "        std::string has_f(\"has_\");\n"
            "        has_f += std::string(RSTRING_PTR(field_sym_str), RSTRING_LEN(field_sym_str));\n"
            "        has_f += \"?\";\n"
            "        has_field_sym_id = rb_intern(has_f.c_str());\n"
            "    } else {\n"
            "        rb_raise(rb_eTypeError, \"Not a symbol or string\");\n"
            "        return Qnil;\n"
            "    }\n"
            "    VALUE first_obj = Qnil;\n"
            "    if (rb_funcall(self, has_field_sym_id, 0) == Qtrue) {\n"
            "        first_obj = rb_funcall(self, field_sym_id, 0);\n"
            "    }\n"
            "    if (first_obj == Qnil) {\n"
            "        return Qnil;\n"
            "    } else if (argc >= 2) {\n"
            "        return rb_funcall2(first_obj, rb_intern(\"get\"), argc - 1, argv + 1);\n"
            "    } else {\n"
            "        return first_obj;\n"
            "    }"
            "}\n"
            "VALUE $class_name$::get_nested_bang(int argc, VALUE* argv, VALUE self) {\n"
            "    VALUE obj = get_nested(argc, argv, self);\n"
            "    if (obj == Qnil) {\n"
            "        rb_raise(rb_eArgError, \"Field is not set\");\n"
            "        return Qnil;\n"
            "    } else {\n"
            "        return obj;\n"
            "    }\n"
            "}\n"
            "VALUE $class_name$::notify_default_changed(VALUE self, VALUE sender, VALUE notify_tag) {\n"
            "    int field_num = NUM2INT(notify_tag);\n"
            "    // Meaningless for required fields;\n"
            "    if (!$cpp_proto_type$::descriptor()->FindFieldByNumber(field_num)->is_optional()) {\n"
            "        return Qnil;\n"
            "    }\n"
            "    // Just do the slow get_value_for_tag thing :(\n"
            "    VALUE current_field_value = value_for_tag(self, notify_tag);\n"
            "    if (current_field_value != sender) {\n"
            "        return Qnil;\n"
            "    }\n"
            "    set_value_for_tag(self, notify_tag, current_field_value);\n"
            "    return Qnil;\n"
            "}\n",
            "class_name", class_name,
            "cpp_proto_type", cpp_proto_class_name(message_type)
        );
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
            "    auto data = reinterpret_cast<dangerous_func_data*>(rb_data_object_get(data_as_value));\n"
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
                // Loop the array into the protobuf.
                printer.Print(
                    "for (\n"
                    "    const VALUE* array_el = rb_array_const_ptr(_self->field_$field_name$);\n"
                    "    array_el < rb_array_const_ptr(_self->field_$field_name$) + rb_array_len(_self->field_$field_name$);\n"
                    "    array_el++\n"
                    ") {\n",
                    "field_name", cpp_field_name(field)
                );
            } else if (field->is_optional()) {
                // Do nothing if the field is not set
                printer.Print("if (_self->has_field_$field_name$) {\n", "field_name", cpp_field_name(field));
            } else {
                printer.Print("if (true) {\n");
            }
            printer.Indent(); printer.Indent();

            // Depending on the ruby class, we do a different conversion
            // The numeric _S macros are defined in generator_entrypoint.cpp
            // and wrap the ruby macros to disable float-to-int coersion
            std::string single_op;
            std::string repeated_op;
            switch (field->type()) {
                case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
                    single_op = "_cpp_proto->set_$field_name$(NUM2INT_S(_self->field_$field_name$));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(NUM2INT_S(*array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                    single_op = "_cpp_proto->set_$field_name$(NUM2UINT_S(_self->field_$field_name$));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(NUM2UINT_S(*array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
                    single_op = "_cpp_proto->set_$field_name$(NUM2LONG_S(_self->field_$field_name$));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(NUM2LONG_S(*array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                    single_op = "_cpp_proto->set_$field_name$(NUM2ULONG_S(_self->field_$field_name$));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(NUM2ULONG_S(*array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                    single_op = "_cpp_proto->set_$field_name$(static_cast<float>(NUM2DBL(_self->field_$field_name$)));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(static_cast<float>(NUM2DBL(*array_el)));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                    single_op = "_cpp_proto->set_$field_name$(NUM2DBL(_self->field_$field_name$));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(NUM2DBL(*array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                    single_op = "_cpp_proto->set_$field_name$(VAL2BOOL_S(_self->field_$field_name$));\n";
                    repeated_op = "_cpp_proto->add_$field_name$(VAL2BOOL_S(*array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                    // Pass two arguments to set_stringfield()
                    single_op = (
                        "_cpp_proto->set_$field_name$(\n"
                        "    RSTRING_PTR(_self->field_$field_name$),\n"
                        "    RSTRING_LEN(_self->field_$field_name$)\n"
                        ");\n"
                    );
                    repeated_op = (
                        "_cpp_proto->add_$field_name$(\n"
                        "    RSTRING_PTR(*array_el),\n"
                        "    RSTRING_LEN(*array_el)\n"
                        ");\n"
                    );
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
                    // Recurse to serialize the message
                    single_op = (
                        "Check_Type(_self->field_$field_name$, T_DATA);\n"
                        "if (CLASS_OF(_self->field_$field_name$) != rb_path2class(\"$rb_message_class_name$\")) {\n"
                        "    rb_raise(rb_eTypeError, \"$field_name$ not a $rb_message_class_name$\");\n"
                        "} else {\n"
                        "    $nested_message_type$* cpp_nested;\n"
                        "    Data_Get_Struct(_self->field_$field_name$, $nested_message_type$, cpp_nested);\n"
                        "    cpp_nested->to_proto_obj(_cpp_proto->mutable_$field_name$());\n"
                        "}\n"
                    );
                    repeated_op = (
                        "Check_Type(*array_el, T_DATA);\n"
                        "if (CLASS_OF(*array_el) != rb_path2class(\"$rb_message_class_name$\")) {\n"
                        "    rb_raise(rb_eTypeError, \"$field_name$ not a $rb_message_class_name$\");\n"
                        "} else {\n"
                        "    $nested_message_type$* cpp_nested;\n"
                        "    Data_Get_Struct(*array_el, $nested_message_type$, cpp_nested);\n"
                        "    auto pb_el = _cpp_proto->add_$field_name$();\n"
                        "    cpp_nested->to_proto_obj(pb_el);\n"
                        "}\n"
                    );
                    break;
                default:
                    break;
            }

            printer.Print(
                (field->is_repeated() ? repeated_op : single_op).c_str(),
                "field_name", cpp_field_name(field),
                "rb_message_class_name", field->message_type() != nullptr ? ruby_proto_class_name(field->message_type()) : "",
                "nested_message_type", field->message_type() != nullptr ? cpp_proto_wrapper_struct_name(field->message_type()) : ""
            );

            printer.Outdent(); printer.Outdent();
            if (field->is_optional()) {
                printer.Print("} else {\n"); // close of if (required | set)
                printer.Print("    _cpp_proto->clear_$field_name$();\n", "field_name", cpp_field_name(field));
            }
            printer.Print("}\n");
        }

        // Now set any unknown fields.
        printer.Print("_cpp_proto->GetReflection()->MutableUnknownFields(_cpp_proto)->MergeFrom(_self->unknown_fields);\n");

        printer.Print("return Qnil;\n");
        printer.Outdent(); printer.Outdent();

        printer.Print(
            "};\n"
            "int exc_status;\n"
            "dangerous_func_data data_struct;\n"
            "data_struct.cpp_proto = cpp_proto;\n"
            "data_struct.self = this;\n"
            "VALUE data_struct_as_value = rb_data_object_wrap(rb_cObject, &data_struct, nullptr, [](void* _){});\n"
            "rb_protect(dangerous_func, data_struct_as_value, &exc_status);\n"
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
                printer.Print(
                    "// Initialize the array to the right size.\n"
                    "field_$field_name$ = rb_ary_new_capa(cpp_proto.$field_name$_size());\n"
                    "for (auto&& array_el : cpp_proto.$field_name$()) {\n",
                    "field_name", cpp_field_name(field)
                );
            } else if (field->is_optional()) {
                // Do nothing if the field is not set
                printer.Print("if (cpp_proto.has_$field_name$()) {\n", "field_name", cpp_field_name(field));
            } else {
                printer.Print("if (true) {\n");
            }
            printer.Indent(); printer.Indent();

            std::string single_op;
            std::string repeated_op;
            switch (field->type()) {
                case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
                    single_op = "field_$field_name$ = INT2NUM(cpp_proto.$field_name$());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, INT2NUM(array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                    single_op = "field_$field_name$ = UINT2NUM(cpp_proto.$field_name$());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, UINT2NUM(array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
                    single_op = "field_$field_name$ = LONG2NUM(cpp_proto.$field_name$());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, LONG2NUM(array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                    single_op = "field_$field_name$ = ULONG2NUM(cpp_proto.$field_name$());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, ULONG2NUM(array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                    single_op = "field_$field_name$ = DBL2NUM(cpp_proto.$field_name$());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, DBL2NUM(array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                    single_op = "field_$field_name$ = BOOL2VAL_S(cpp_proto.$field_name$());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, BOOL2VAL_S(array_el));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                    // Important to use the length, because .data() also has a terminating
                    // null byte.
                    single_op = "field_$field_name$ = rb_str_new(cpp_proto.$field_name$().data(), cpp_proto.$field_name$().length());\n";
                    repeated_op = "rb_ary_push(field_$field_name$, rb_str_new(array_el.data(), array_el.length()));\n";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
                    // Recurse to serialize the message
                    single_op = (
                        "{\n"
                        "    this->field_$field_name$ = $nested_message_type$::alloc();\n"
                        "    rb_obj_call_init(this->field_$field_name$, 0, nullptr);\n"
                        "    $nested_message_type$* cpp_nested;\n"
                        "    Data_Get_Struct(this->field_$field_name$, $nested_message_type$, cpp_nested);\n"
                        "    cpp_nested->from_proto_obj(cpp_proto.$field_name$());\n"
                        "}\n"
                    );
                    repeated_op = (
                        "{\n"
                        "    VALUE new_obj = $nested_message_type$::alloc();\n"
                        "    rb_obj_call_init(this->field_$field_name$, 0, nullptr);\n"
                        "    rb_ary_push(field_$field_name$, new_obj);\n"
                        "    $nested_message_type$* cpp_nested;\n"
                        "    Data_Get_Struct(new_obj, $nested_message_type$, cpp_nested);\n"
                        "    cpp_nested->from_proto_obj(array_el);\n"
                        "}\n"
                    );
                    break;
                default:
                    break;
            }

            printer.Print(
                (field->is_repeated() ? repeated_op : single_op).c_str(),
                "field_name", cpp_field_name(field),
                "rb_message_class_name", field->message_type() != nullptr ? ruby_proto_class_name(field->message_type()) : "",
                "nested_message_type", field->message_type() != nullptr ? cpp_proto_wrapper_struct_name(field->message_type()) : ""
            );

            if (field->is_optional()) {
                printer.Print("has_field_$field_name$ = true;\n", "field_name", cpp_field_name(field));
            }


            printer.Outdent(); printer.Outdent();
            if (field->is_optional()) {
                printer.Print("} else {\n"); // close of if (required | set)
                printer.Print("   has_field_$field_name$ = false;\n", "field_name", cpp_field_name(field));
            }
            printer.Print("}\n");
        }

        // Now set any unknown fields.
        printer.Print("this->unknown_fields.MergeFrom(cpp_proto.GetReflection()->GetUnknownFields(cpp_proto));\n");

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
            "}\n\n"

            "VALUE $class_name$::serialize_to_string_with_gvl(VALUE self) {\n"
            "    VALUE ex;\n"
            "\n"
            "    {\n"
            "        $class_name$* cpp_self;\n"
            "        Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "\n"
            "        $cpp_proto_class$ cpp_proto;\n"
            "        ex = cpp_self->to_proto_obj(&cpp_proto);\n"
            "        if (ex != Qnil) {\n"
            "            goto raise;\n"
            "        }\n"
            "        VALUE rb_str = rb_str_new(\"\", 0);\n"
            "        auto pb_size = cpp_proto.ByteSize();\n"
            "        rb_str_resize(rb_str, pb_size);\n"
            "        cpp_proto.SerializeToArray(RSTRING_PTR(rb_str), static_cast<int>(pb_size));\n"
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
