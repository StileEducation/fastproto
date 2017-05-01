// Generated code that calls all the entrypoints
#include "rb_fastproto_init.h"

// @@protoc_insertion_point(init_file_header)

namespace rb_fastproto_gen {
    VALUE rb_fastproto_module = Qnil;
    VALUE cls_fastproto_message = Qnil;
    VALUE cls_fastproto_service = Qnil;
    VALUE cls_fastproto_method = Qnil;
    VALUE cls_fastproto_field = Qnil;

    static void define_message_class();
    static void define_service_class();
    static void define_method_class();
    static void define_field_class();
}

extern "C" void Init_fastproto_gen(void) {
    // Define our toplevel module
    rb_fastproto_gen::rb_fastproto_module = rb_define_module("Fastproto");

    rb_fastproto_gen::define_message_class();
    rb_fastproto_gen::define_service_class();
    rb_fastproto_gen::define_method_class();
    rb_fastproto_gen::define_field_class();

    // @@protoc_insertion_point(init_entrypoints)
}

namespace rb_fastproto_gen {
    static VALUE cls_fastproto_message_find_by_fully_qualified_name(VALUE self, VALUE name) {
        Check_Type(name, T_STRING);
        return rb_funcall(rb_cv_get(self, "@@message_classes"), rb_intern("[]"), 1, name);
    }

    static void define_message_class() {
        cls_fastproto_message = rb_define_class_under(rb_fastproto_module, "Message", rb_cObject);
        rb_cv_set(cls_fastproto_message, "@@message_classes", rb_hash_new());
        rb_define_singleton_method(cls_fastproto_message, "find_by_fully_qualified_name", RUBY_METHOD_FUNC(&cls_fastproto_message_find_by_fully_qualified_name), 1);
    }

    static void define_service_class() {
        cls_fastproto_service = rb_define_class_under(rb_fastproto_module, "Service", rb_cObject);
    }

    static void define_method_class() {
        cls_fastproto_method = rb_define_class_under(rb_fastproto_module, "Method", rb_cObject);
    }

    static VALUE cls_fastproto_field_initialize(VALUE self, VALUE tag) {
        Check_Type(tag, T_FIXNUM);
        rb_ivar_set(self, rb_intern("@tag"), tag);
        return self;
    }

    static VALUE cls_fastproto_field_tag(VALUE self) {
        return rb_ivar_get(self, rb_intern("@tag"));
    }

    static void define_field_class() {
        cls_fastproto_field = rb_define_class_under(rb_fastproto_module, "Field", rb_cObject);
        rb_define_method(cls_fastproto_field, "initialize", RUBY_METHOD_FUNC(&cls_fastproto_field_initialize), 1);
        rb_define_method(cls_fastproto_field, "tag", RUBY_METHOD_FUNC(&cls_fastproto_field_tag), 0);
    }
}
