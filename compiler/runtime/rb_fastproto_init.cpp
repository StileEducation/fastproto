// Generated code that calls all the entrypoints
#include "rb_fastproto_init.h"

// @@protoc_insertion_point(init_file_header)

namespace rb_fastproto_gen {
    VALUE rb_fastproto_module = Qnil;
    VALUE cls_fastproto_message = Qnil;
    VALUE cls_fastproto_service = Qnil;
    VALUE cls_fastproto_method = Qnil;

    static void define_message_class();
    static void define_service_class();
    static void define_method_class();
}

extern "C" void Init_fastproto_gen(void) {
    // Define our toplevel module
    rb_fastproto_gen::rb_fastproto_module = rb_define_module("Fastproto");

    rb_fastproto_gen::define_message_class();
    rb_fastproto_gen::define_service_class();
    rb_fastproto_gen::define_method_class();

    // @@protoc_insertion_point(init_entrypoints)
}

namespace rb_fastproto_gen {
    static void define_message_class() {
        cls_fastproto_message = rb_define_class_under(rb_fastproto_module, "Message", rb_cObject);
    }

    static void define_service_class() {
        cls_fastproto_service = rb_define_class_under(rb_fastproto_module, "Service", rb_cObject);
    }

    static void define_method_class() {
        cls_fastproto_method = rb_define_class_under(rb_fastproto_module, "Method", rb_cObject);
    }
}
