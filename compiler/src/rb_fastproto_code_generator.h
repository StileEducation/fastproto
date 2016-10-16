
#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>

#ifndef __RB_FASTPROTO_CODE_GENERATOR
#define __RB_FASTPROTO_CODE_GENERATOR

namespace rb_fastproto {
    class RBFastProtoCodeGenerator : public ::google::protobuf::compiler::CodeGenerator {
    public:
        explicit RBFastProtoCodeGenerator() = default;
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
            const std::string &full_header_file_name,
            google::protobuf::io::Printer &printer
        ) const;
        void write_cpp(
            const google::protobuf::FileDescriptor *file,
            const std::string &full_cpp_file_name,
            const std::string &full_header_file_name,
            google::protobuf::io::Printer &printer
        ) const;

        std::string header_name_as_identifier(const std::string &header_file_name) const;
    };
}

#endif
