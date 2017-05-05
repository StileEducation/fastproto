#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <stack>
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
        auto header_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".fastproto.h");
        return header_file_path.string();
    }

    std::string rb_path_for_proto(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path proto_file_path(proto_file->name());
        auto rb_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".pb.rb");
        return rb_file_path.string();
    }

    std::string cpp_proto_header_path_for_proto(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path proto_file_path(proto_file->name());
        auto header_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".pb.h");
        return header_file_path.string();
    }

    std::string cpp_path_for_proto(const google::protobuf::FileDescriptor* proto_file) {
        boost::filesystem::path proto_file_path(proto_file->name());
        auto header_file_path = proto_file_path.parent_path() / (proto_file_path.stem().string() + ".fastproto.cpp");
        return header_file_path.string();
    }

    std::string cpp_proto_class_name(const google::protobuf::Descriptor* message_type) {
        auto cpp_proto_ns = boost::replace_all_copy(message_type->file()->package(), ".", "::");
        // Use a std::stack to push the parents up the chain, then append them to the namespace, most senior first
        std::stack<std::string> message_type_chain;
        auto cur_type = message_type;
        do {
            message_type_chain.push(cur_type->name());
        } while ( (cur_type = cur_type->containing_type()) != nullptr );
        cpp_proto_ns += "::";
        while (!message_type_chain.empty()) {
            cpp_proto_ns += message_type_chain.top();
            message_type_chain.pop();
            if (!message_type_chain.empty()) {
                cpp_proto_ns += "_";
            }
        }
        return cpp_proto_ns;
    }

    std::string cpp_proto_descriptor_name(const google::protobuf::Descriptor* message_type) {
        return boost::str(boost::format("%s_descriptor_") % cpp_proto_class_name(message_type));
    }

    std::string ruby_proto_enum_class_name(const google::protobuf::EnumDescriptor* enum_type) {
        auto cpp_proto_ns = boost::join(rubyised_namespace_els(enum_type->file()), "::");

        std::stack<std::string> enum_type_chain;

        enum_type_chain.push(enum_type->name());

        for (auto cur_type = enum_type->containing_type(); cur_type != nullptr; cur_type = cur_type->containing_type()) {
            enum_type_chain.push(cur_type->name());
        }

        while (!enum_type_chain.empty()) {
            cpp_proto_ns += (std::string("::") + enum_type_chain.top());
            enum_type_chain.pop();
        }

        return cpp_proto_ns;
    }

    std::string cpp_proto_enum_wrapper_struct_name(const google::protobuf::EnumDescriptor* enum_type) {
        std::string cpp_proto_ns("rb_fastproto_gen::");

        cpp_proto_ns += boost::join(rubyised_namespace_els(enum_type->file()), "::");

        std::stack<std::string> enum_type_chain;

        for (auto cur_type = enum_type->containing_type(); cur_type != nullptr; cur_type = cur_type->containing_type()) {
            enum_type_chain.push(cur_type->name());
        }

        while (!enum_type_chain.empty()) {
            cpp_proto_ns += (std::string("::RB") + enum_type_chain.top());
            enum_type_chain.pop();
        }

        if (enum_type->containing_type() != nullptr) {
            return cpp_proto_ns + "_" + enum_type->name();
        } else {
            return cpp_proto_ns + "::RB" + enum_type->name();
        }
    }

    std::string cpp_proto_enum_wrapper_struct_name_no_ns(const google::protobuf::EnumDescriptor* enum_type) {
        if (enum_type->containing_type() != nullptr) {
            return cpp_proto_message_wrapper_struct_name_no_ns(enum_type->containing_type()) + "_" + enum_type->name();
        } else {
            return std::string("RB") + enum_type->name();
        }
    }

    std::string ruby_proto_message_class_name(const google::protobuf::Descriptor* message_type) {
        auto cpp_proto_ns = boost::join(rubyised_namespace_els(message_type->file()), "::");
        // Use a std::stack to push the parents up the chain, then append them to the namespace, most senior first
        std::stack<std::string> message_type_chain;
        auto cur_type = message_type;
        do {
            message_type_chain.push(cur_type->name());
        } while ( (cur_type = cur_type->containing_type()) != nullptr );
        while (!message_type_chain.empty()) {
            cpp_proto_ns += (std::string("::") + message_type_chain.top());
            message_type_chain.pop();
        }
        return cpp_proto_ns;
    }

    std::string cpp_proto_message_wrapper_struct_name(const google::protobuf::Descriptor* message_type) {
        std::string cpp_proto_ns("rb_fastproto_gen::");
        cpp_proto_ns += boost::join(rubyised_namespace_els(message_type->file()), "::");
        // Use a std::stack to push the parents up the chain, then append them to the namespace, most senior first
        std::stack<std::string> message_type_chain;
        auto cur_type = message_type;
        do {
            message_type_chain.push(cur_type->name());
        } while ( (cur_type = cur_type->containing_type()) != nullptr );
        while (!message_type_chain.empty()) {
            cpp_proto_ns += (std::string("::RB") + message_type_chain.top());
            message_type_chain.pop();
        }
        return cpp_proto_ns;
    }

    std::string cpp_proto_message_wrapper_struct_name_no_ns(const google::protobuf::Descriptor* message_type) {
        return std::string("RB") + message_type->name();
    }

    std::string ruby_proto_service_class_name(const google::protobuf::ServiceDescriptor* service) {
        return boost::join(rubyised_namespace_els(service->file()), "::") + "::" + service->name();
    }

    std::string cpp_proto_service_wrapper_struct_name(const google::protobuf::ServiceDescriptor* service) {
        std::string cpp_proto_ns("rb_fastproto_gen::");
        cpp_proto_ns += boost::join(rubyised_namespace_els(service->file()), "::");
        cpp_proto_ns += "::RB";
        cpp_proto_ns += service->name();
        return cpp_proto_ns;
    }

    std::string cpp_proto_service_wrapper_struct_name_no_ns(const google::protobuf::ServiceDescriptor* service) {
        std::string cpp_proto_ns("RB");
        cpp_proto_ns += service->name();
        return cpp_proto_ns;
    }

    std::string ruby_proto_method_class_name(const google::protobuf::MethodDescriptor* method) {
        return boost::join(rubyised_namespace_els(method->service()->file()), "::") + "::" + method->service()->name() + method->name();
    }

    std::string cpp_proto_method_wrapper_struct_name(const google::protobuf::MethodDescriptor* method) {
        std::string cpp_proto_ns("rb_fastproto_gen::");
        cpp_proto_ns += boost::join(rubyised_namespace_els(method->service()->file()), "::");
        cpp_proto_ns += "::RB";
        cpp_proto_ns += method->service()->name() + method->name();
        return cpp_proto_ns;
    }

    std::string cpp_proto_method_wrapper_struct_name_no_ns(const google::protobuf::MethodDescriptor* method) {
        std::string cpp_proto_ns("RB");
        cpp_proto_ns += method->service()->name() + method->name();
        return cpp_proto_ns;
    }

    std::vector<std::string> rubyised_namespace_els(const google::protobuf::FileDescriptor* file) {
        std::vector<std::string> namespace_els;
        boost::split(namespace_els, file->package(), boost::is_any_of("."));
        for (auto&& el : namespace_els) {
            el = boost::regex_replace(el, boost::regex("(?:^|_)(.)"), [](const boost::smatch &match){
                return boost::to_upper_copy(match.str(1));
            });
        }
        return namespace_els;
    }

    // Rip this from cpp_helpers.cc to transform PB names to C++ field names
    std::set<std::string> cpp_keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "class", "compl", "const",
        "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do",
        "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
        "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
        "mutable", "namespace", "new", "noexcept", "not", "not_eq", "NULL",
        "operator", "or", "or_eq", "private", "protected", "public", "register",
        "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while", "xor", "xor_eq"
    };

    std::string cpp_field_name(const google::protobuf::FieldDescriptor* field) {
        std::string result = field->name();
        boost::to_lower(result);
        if (cpp_keywords.count(result) > 0) {
            result += "_";
        }
        return result;
    }
}
