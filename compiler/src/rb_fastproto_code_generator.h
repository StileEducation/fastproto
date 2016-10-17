
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
        void write_entrypoint(
            const google::protobuf::FileDescriptor* file,
            google::protobuf::compiler::OutputDirectory *output_directory
        ) const;

        void write_cpp_message_struct(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;

        void write_cpp_message_struct_constructor(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_static_initializer(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_fields(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_accessors(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_allocators(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_struct_validator(
            const google::protobuf::FileDescriptor* file,
            const google::protobuf::Descriptor* message_type,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp_message_module_init(
            const google::protobuf::FileDescriptor* file,
            google::protobuf::io::Printer &printer
        ) const;

    };

    std::string header_name_as_identifier(const google::protobuf::FileDescriptor* proto_file);
    std::string header_path_for_proto(const google::protobuf::FileDescriptor* proto_file);
    std::string cpp_path_for_proto(const google::protobuf::FileDescriptor* proto_file);
    void add_entrypoint_files(google::protobuf::compiler::CodeGeneratorResponse &response);
}

#endif
