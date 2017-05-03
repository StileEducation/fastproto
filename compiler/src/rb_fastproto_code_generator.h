
#include <string>
#include <vector>
#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/compiler/plugin.pb.h>

#ifndef __RB_FASTPROTO_CODE_GENERATOR
#define __RB_FASTPROTO_CODE_GENERATOR

namespace rb_fastproto {
    class RBFastProtoCodeGenerator : public ::google::protobuf::compiler::CodeGenerator {
    public:
        explicit RBFastProtoCodeGenerator();
        virtual ~RBFastProtoCodeGenerator() = default;

        virtual bool Generate(
            const google::protobuf::FileDescriptor *file,
            const std::string &parameter,
            google::protobuf::compiler::OutputDirectory *output_directory,
            std::string *error
        ) const;

    private:
        void write_header(
            const google::protobuf::FileDescriptor *file,
            google::protobuf::io::Printer &printer
        ) const;

        void write_cpp(
            const google::protobuf::FileDescriptor *file,
            google::protobuf::io::Printer &printer
        ) const;

        void write_cpp_module_init(
            const google::protobuf::FileDescriptor* file,
            const std::vector<std::string> &class_names,
            google::protobuf::io::Printer &printer
        ) const;

        // message code

        void write_header_message_struct_definition(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_header_message_struct_fields(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_header_message_struct_accessors(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;

        std::string write_cpp_message_struct(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;

        void write_cpp_message_struct_constructor(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_static_initializer(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_default_factories(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_accessors(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_dynamic_accessors(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_allocators(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_to_proto_obj(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_from_proto_obj(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_validator(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_serializer(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_parser(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_equality(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_inspect(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_singleton_parse(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_singleton_field_for_name(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_singleton_fields(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_singleton_fully_qualified_name(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;

        // service code

        void write_header_service_struct_definition(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            google::protobuf::io::Printer &printer
        ) const;

        std::string write_cpp_service_struct(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            google::protobuf::io::Printer &printer
        ) const;

        void write_cpp_service_struct_constructor(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_service_struct_static_initializer(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_service_struct_allocators(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_service_struct_name(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_service_struct_rpcs(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::ServiceDescriptor* service,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;

        // method code

        void write_header_method_struct_definition(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            google::protobuf::io::Printer &printer
        ) const;

        std::string write_cpp_method_struct(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            google::protobuf::io::Printer &printer
        ) const;

        void write_cpp_method_struct_constructor(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_method_struct_static_initializer(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_method_struct_allocators(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_method_struct_name(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_method_struct_classes(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::MethodDescriptor* method,
            const std::string &class_name,
            google::protobuf::io::Printer &printer
        ) const;
    };

    std::string header_name_as_identifier(const google::protobuf::FileDescriptor* proto_file);
    std::string header_path_for_proto(const google::protobuf::FileDescriptor* proto_file);
    std::string cpp_proto_header_path_for_proto(const google::protobuf::FileDescriptor* proto_file);
    std::string cpp_path_for_proto(const google::protobuf::FileDescriptor* proto_file);
    std::string cpp_proto_class_name(const google::protobuf::Descriptor* message_type);
    std::string cpp_proto_descriptor_name(const google::protobuf::Descriptor* message_type);
    std::string ruby_proto_message_class_name(const google::protobuf::Descriptor* message_type);
    std::string ruby_proto_service_class_name(const google::protobuf::ServiceDescriptor* service);
    std::string ruby_proto_method_class_name(const google::protobuf::MethodDescriptor* method);
    std::vector<std::string> rubyised_namespace_els(const google::protobuf::FileDescriptor* file);
    std::string cpp_proto_message_wrapper_struct_name(const google::protobuf::Descriptor* message_type);
    std::string cpp_proto_message_wrapper_struct_name_no_ns(const google::protobuf::Descriptor* message_type);
    std::string cpp_proto_service_wrapper_struct_name(const google::protobuf::ServiceDescriptor* service);
    std::string cpp_proto_service_wrapper_struct_name_no_ns(const google::protobuf::ServiceDescriptor* service);
    std::string cpp_proto_method_wrapper_struct_name(const google::protobuf::MethodDescriptor* method);
    std::string cpp_proto_method_wrapper_struct_name_no_ns(const google::protobuf::MethodDescriptor* method);
    std::string cpp_field_name(const google::protobuf::FieldDescriptor* field);
}

#endif
