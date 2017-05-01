// Generated code that calls all the entrypoints
#include "rb_fastproto_init.h"

// @@protoc_insertion_point(init_file_header)

namespace rb_fastproto_gen {
    VALUE rb_fastproto_module = Qnil;
    VALUE cls_fastproto_message = Qnil;
    VALUE cls_fastproto_service = Qnil;
    VALUE cls_fastproto_method = Qnil;
    VALUE cls_fastproto_field = Qnil;
    VALUE cls_fastproto_field_integer = Qnil;
    VALUE cls_fastproto_field_float = Qnil;
    VALUE cls_fastproto_field_bool = Qnil;
    VALUE cls_fastproto_field_bytes = Qnil;
    VALUE cls_fastproto_field_string = Qnil;
    VALUE cls_fastproto_field_enum = Qnil;
    VALUE cls_fastproto_field_message = Qnil;
    VALUE cls_fastproto_field_unknown = Qnil;

    static void define_message_class();
    static void define_service_class();
    static void define_method_class();
    static void define_field_class();
    static void define_field_integer_class();
    static void define_field_float_class();
    static void define_field_bool_class();
    static void define_field_bytes_class();
    static void define_field_string_class();
    static void define_field_enum_class();
    static void define_field_message_class();
    static void define_field_unknown_class();
}

extern "C" void Init_fastproto_gen(void) {
    // Define our toplevel module
    rb_fastproto_gen::rb_fastproto_module = rb_define_module("Fastproto");

    rb_fastproto_gen::define_message_class();
    rb_fastproto_gen::define_service_class();
    rb_fastproto_gen::define_method_class();
    rb_fastproto_gen::define_field_class();
    rb_fastproto_gen::define_field_integer_class();
    rb_fastproto_gen::define_field_float_class();
    rb_fastproto_gen::define_field_bool_class();
    rb_fastproto_gen::define_field_bytes_class();
    rb_fastproto_gen::define_field_string_class();
    rb_fastproto_gen::define_field_enum_class();
    rb_fastproto_gen::define_field_message_class();
    rb_fastproto_gen::define_field_unknown_class();

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

    static VALUE cls_fastproto_field_initialize(VALUE self, VALUE tag, VALUE name) {
        Check_Type(tag, T_FIXNUM);
        Check_Type(name, T_STRING);
        rb_ivar_set(self, rb_intern("@tag"), tag);
        rb_ivar_set(self, rb_intern("@name"), name);
        return self;
    }

    static VALUE cls_fastproto_field_tag(VALUE self) {
        return rb_ivar_get(self, rb_intern("@tag"));
    }

    static VALUE cls_fastproto_field_name(VALUE self) {
        return rb_ivar_get(self, rb_intern("@name"));
    }

    static void define_field_class() {
        cls_fastproto_field = rb_define_class_under(rb_fastproto_module, "Field", rb_cObject);
        rb_define_method(cls_fastproto_field, "initialize", RUBY_METHOD_FUNC(&cls_fastproto_field_initialize), 2);
        rb_define_method(cls_fastproto_field, "tag", RUBY_METHOD_FUNC(&cls_fastproto_field_tag), 0);
        rb_define_method(cls_fastproto_field, "name", RUBY_METHOD_FUNC(&cls_fastproto_field_name), 0);
    }

    static void define_field_integer_class() {
        cls_fastproto_field_integer = rb_define_class_under(rb_fastproto_module, "FieldInteger", cls_fastproto_field);
    }

    static void define_field_float_class() {
        cls_fastproto_field_float = rb_define_class_under(rb_fastproto_module, "FieldFloat", cls_fastproto_field);
    }

    static void define_field_bool_class() {
        cls_fastproto_field_bool = rb_define_class_under(rb_fastproto_module, "FieldBool", cls_fastproto_field);
    }

    static void define_field_bytes_class() {
        cls_fastproto_field_bytes = rb_define_class_under(rb_fastproto_module, "FieldBytes", cls_fastproto_field);
    }

    static void define_field_string_class() {
        cls_fastproto_field_string = rb_define_class_under(rb_fastproto_module, "FieldString", cls_fastproto_field);
    }

    static void define_field_enum_class() {
        cls_fastproto_field_enum = rb_define_class_under(rb_fastproto_module, "FieldEnum", cls_fastproto_field);
    }

    static void define_field_message_class() {
        cls_fastproto_field_message = rb_define_class_under(rb_fastproto_module, "FieldMessage", cls_fastproto_field);
    }

    static void define_field_unknown_class() {
        cls_fastproto_field_unknown = rb_define_class_under(rb_fastproto_module, "FieldUnknown", cls_fastproto_field);
    }
}
