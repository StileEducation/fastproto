#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include "rb_fastproto_code_generator.h"

// Some utils for working with filenames et al
namespace rb_fastproto {

    std::string header_name_as_identifier(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path header_file_path(header_path_for_proto(proto_file));
        auto header_path_no_ext = header_file_path.parent_path() / header_file_path.stem();

        auto header_file_name_with_underscores = boost::replace_all_copy(
            header_path_no_ext.string(),
            std::string(1, boost::filesystem::path::preferred_separator),
            "_"
        );
        boost::replace_all(header_file_name_with_underscores, ".", "_");
        boost::to_upper(header_file_name_with_underscores);
        return header_file_name_with_underscores;
    }

    std::string header_path_for_proto(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path proto_file_path(proto_file->name());
        auto header_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".h");
        return header_file_path.string();
    }

    std::string cpp_proto_header_path_for_proto(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path proto_file_path(proto_file->name());
        auto header_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".pb.h");
        return header_file_path.string();
    }

    std::string cpp_path_for_proto(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path proto_file_path(proto_file->name());
        auto header_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".cpp");
        return header_file_path.string();
    }

    std::string cpp_proto_class_name(const google::protobuf::Descriptor* message_type) {
        auto cpp_proto_ns = boost::replace_all_copy(message_type->file()->package(), ".", "::");
        auto cpp_proto_cls = message_type->name();
        return boost::str(boost::format("%s::%s") % cpp_proto_ns % cpp_proto_cls);
    }

    std::string ruby_proto_class_name(const google::protobuf::Descriptor* message_type) {
        std::string cls_name("::");
        auto namespace_bits = rubyised_namespace_els(message_type->file());
        for (auto el : rubyised_namespace_els(message_type->file())) {
            cls_name += (el + "::");
        }
        cls_name += message_type->name();
        return cls_name;
    }

    std::vector<std::string> rubyised_namespace_els(const google::protobuf::FileDescriptor* file) {
        std::vector<std::string> namespace_els;
        // TODO: Case conversion of packages
        boost::split(namespace_els, file->package(), boost::is_any_of("."));
        for (auto&& el : namespace_els) {
            el = boost::regex_replace(el, boost::regex("(?:^|_)(.)"), [](const boost::smatch &match){
                return boost::to_upper_copy(match.str(1));
            });
        }
        return namespace_els;
    }


    std::string cpp_proto_wrapper_struct_name(const google::protobuf::Descriptor* message_type) {
        std::string name("RB");
        name += message_type->name();
        return name;
    }
}
