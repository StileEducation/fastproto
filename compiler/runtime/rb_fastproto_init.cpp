// Generated code that calls all the entrypoints
#include "rb_fastproto_init.h"
#include "rb_fastproto_init_thunks.h"

namespace rb_fastproto_gen {
    VALUE rb_fastproto_module = Qnil;
    VALUE cls_fastproto_enum = Qnil;
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
    VALUE cls_fastproto_field_aggregate = Qnil;
    VALUE cls_fastproto_field_message = Qnil;
    VALUE cls_fastproto_field_group = Qnil;
    VALUE cls_fastproto_field_unknown = Qnil;

    static void define_enum_class();
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
    static void define_field_aggregate_class();
    static void define_field_message_class();
    static void define_field_group_class();
    static void define_field_unknown_class();
}

extern "C" void Init_fastproto_gen(void) {
    // Define our toplevel module
    rb_fastproto_gen::rb_fastproto_module = rb_define_module("Fastproto");

    rb_fastproto_gen::define_enum_class();
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
    rb_fastproto_gen::define_field_aggregate_class();
    rb_fastproto_gen::define_field_message_class();
    rb_fastproto_gen::define_field_group_class();
    rb_fastproto_gen::define_field_unknown_class();

    // Now call all the initialisation thunks for each protobuf class in rb_fastproto_init_thunks.h
    for (int i = 0; i < rb_fastproto_init_thunks_len; i++) {
        rb_fastproto_init_thunks[i]();
    }
}

namespace rb_fastproto_gen {
    static void define_enum_class() {
        cls_fastproto_enum = rb_define_class_under(rb_fastproto_module, "Enum", rb_cObject);
    }

    static VALUE cls_fastproto_message_find_by_fully_qualified_name(VALUE self, VALUE name) {
        Check_Type(name, T_STRING);
        return rb_funcall(rb_cv_get(self, "@@message_classes"), rb_intern("[]"), 1, name);
    }

    static VALUE cls_fastproto_message_to_hash(VALUE self, VALUE msg) {
        if (msg == Qnil) {
          return Qnil;
        }

        if (RB_TYPE_P(msg, T_STRING)) {
          return rb_funcall(msg, rb_intern("dup"), 0);
        }

        if (RB_TYPE_P(msg, T_ARRAY)) {
          auto ary = rb_ary_new();

          for (int i = 0; i < RARRAY_LEN(msg); i++) {
            rb_ary_push(ary, rb_funcall(self, rb_intern("to_hash"), 1, rb_ary_entry(msg, i)));
          }

          return ary;
        }

        if (rb_respond_to(msg, rb_intern("to_hash"))) {
          return rb_funcall(msg, rb_intern("to_hash"), 0);
        }

        return msg;
    }

    static void define_message_class() {
        cls_fastproto_message = rb_define_class_under(rb_fastproto_module, "Message", rb_cObject);
        rb_cv_set(cls_fastproto_message, "@@message_classes", rb_hash_new());
        rb_define_singleton_method(cls_fastproto_message, "find_by_fully_qualified_name", RUBY_METHOD_FUNC(&cls_fastproto_message_find_by_fully_qualified_name), 1);
        rb_define_singleton_method(cls_fastproto_message, "to_hash", RUBY_METHOD_FUNC(&cls_fastproto_message_to_hash), 1);
    }

    static void define_service_class() {
        cls_fastproto_service = rb_define_class_under(rb_fastproto_module, "Service", rb_cObject);
    }

    static void define_method_class() {
        cls_fastproto_method = rb_define_class_under(rb_fastproto_module, "Method", rb_cObject);
    }

    static VALUE cls_fastproto_field_initialize(VALUE self, VALUE tag, VALUE name, VALUE repeated) {
        Check_Type(tag, T_FIXNUM);
        Check_Type(name, T_STRING);
        rb_ivar_set(self, rb_intern("@tag"), tag);
        rb_ivar_set(self, rb_intern("@name"), name);
        rb_ivar_set(self, rb_intern("@repeated"), repeated);
        return self;
    }

    static VALUE cls_fastproto_field_repeated(VALUE self) {
        return rb_ivar_get(self, rb_intern("@repeated"));
    }

    static void define_field_class() {
        cls_fastproto_field = rb_define_class_under(rb_fastproto_module, "Field", rb_cObject);
        rb_define_method(cls_fastproto_field, "initialize", RUBY_METHOD_FUNC(&cls_fastproto_field_initialize), 3);
        rb_define_attr(cls_fastproto_field, "tag", 1, 0);
        rb_define_attr(cls_fastproto_field, "name", 1, 0);
        rb_define_method(cls_fastproto_field, "repeated?", RUBY_METHOD_FUNC(&cls_fastproto_field_repeated), 0);
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

    static VALUE cls_fastproto_field_enum_initialize(VALUE self, VALUE tag, VALUE name, VALUE repeated, VALUE value_to_name, VALUE name_to_value) {
        Check_Type(tag, T_FIXNUM);
        Check_Type(name, T_STRING);
        Check_Type(value_to_name, T_HASH);
        Check_Type(name_to_value, T_HASH);
        VALUE args[] = { tag, name, repeated };
        rb_call_super(3, args);
        rb_ivar_set(self, rb_intern("@value_to_name"), value_to_name);
        rb_ivar_set(self, rb_intern("@name_to_value"), name_to_value);
        return self;
    }

    static void define_field_enum_class() {
        cls_fastproto_field_enum = rb_define_class_under(rb_fastproto_module, "FieldEnum", cls_fastproto_field);
        rb_define_method(cls_fastproto_field_enum, "initialize", RUBY_METHOD_FUNC(&cls_fastproto_field_enum_initialize), 5);
        rb_define_attr(cls_fastproto_field_enum, "value_to_name", 1, 0);
        rb_define_attr(cls_fastproto_field_enum, "name_to_value", 1, 0);
    }

    static VALUE cls_fastproto_field_aggregate_initialize(VALUE self, VALUE tag, VALUE name, VALUE repeated, VALUE proxy_class) {
        Check_Type(tag, T_FIXNUM);
        Check_Type(name, T_STRING);
        Check_Type(proxy_class, T_CLASS);
        VALUE args[] = { tag, name, repeated };
        rb_call_super(3, args);
        rb_ivar_set(self, rb_intern("@proxy_class"), proxy_class);
        return self;
    }

    static void define_field_aggregate_class() {
        cls_fastproto_field_aggregate = rb_define_class_under(rb_fastproto_module, "FieldAggregate", cls_fastproto_field);
        rb_define_method(cls_fastproto_field_aggregate, "initialize", RUBY_METHOD_FUNC(&cls_fastproto_field_aggregate_initialize), 4);
        rb_define_attr(cls_fastproto_field_aggregate, "proxy_class", 1, 0);
    }

    static void define_field_message_class() {
        cls_fastproto_field_message = rb_define_class_under(rb_fastproto_module, "FieldMessage", cls_fastproto_field_aggregate);
    }

    static void define_field_group_class() {
        cls_fastproto_field_group = rb_define_class_under(rb_fastproto_module, "FieldGroup", cls_fastproto_field_aggregate);
    }

    static void define_field_unknown_class() {
        cls_fastproto_field_unknown = rb_define_class_under(rb_fastproto_module, "FieldUnknown", cls_fastproto_field);
    }
}
