#include <ruby/ruby.h>
#include <ruby/encoding.h>
#include <functional>
#include <string>

#ifndef __RB_FASTPROTO_INIT_H
#define __RB_FASTPROTO_INIT_H

namespace rb_fastproto_gen {
    // The top-level ::Fastproto module
    extern VALUE rb_fastproto_module;

    // The base classes
    extern VALUE cls_fastproto_message;
    extern VALUE cls_fastproto_service;
    extern VALUE cls_fastproto_method;
    extern VALUE cls_fastproto_field;
    extern VALUE cls_fastproto_field_integer;
    extern VALUE cls_fastproto_field_float;
    extern VALUE cls_fastproto_field_bool;
    extern VALUE cls_fastproto_field_bytes;
    extern VALUE cls_fastproto_field_string;
    extern VALUE cls_fastproto_field_enum;
    extern VALUE cls_fastproto_field_aggregate;
    extern VALUE cls_fastproto_field_message;
    extern VALUE cls_fastproto_field_group;
    extern VALUE cls_fastproto_field_unknown;

    static inline unsigned int NUM2UINT_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2UINT(num);
    }

    static inline int NUM2INT_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2INT(num);
    }

    static inline unsigned long NUM2ULONG_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2ULONG(num);
    }

    static inline long NUM2LONG_S(VALUE num) {
        if (RB_TYPE_P(num, T_FLOAT)) {
            rb_raise(rb_eTypeError, "Expected fixnum, got float");
        }
        return NUM2LONG(num);
    }

    static inline bool VAL2BOOL_S(VALUE arg) {
        if (RB_TYPE_P(arg, T_TRUE)) {
            return true;
        } else if (RB_TYPE_P(arg, T_FALSE)) {
            return false;
        } else {
            rb_raise(rb_eTypeError, "Expected boolean");
            return false;
        }
    }

    static inline VALUE BOOL2VAL_S(bool arg) {
        return arg ? Qtrue : Qfalse;
    }

    static inline VALUE RSTR_AS_UTF8(VALUE rstr) {
        rb_enc_associate_index(rstr, rb_enc_find_index("UTF-8"));
        return rstr;
    }

}

#endif
