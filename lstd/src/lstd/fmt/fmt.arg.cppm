module;

#include "../common.h"

export module lstd.fmt.arg;

export import lstd.fmt.fmt_type;
import lstd.fmt.fmt_type_constant;

LSTD_BEGIN_NAMESPACE

export {
    struct fmt_context;

    void write_no_specs(fmt_context * f, string str);

    //
    // Specialize this for custom types.
    //
    // e.g.
    //
    // template <>
    // struct formatter<my_vector> {
    //     void format(my_vector *value, fmt_context *f) {
    //         fmt_to_writer(f, "v1: {}, v2: {}, ...", my_vector->A, my_vector->B);
    // 		   ...
    // 	   }
    // };
    template <typename T, typename Enable = void>
    struct formatter {
        void format(T *value, fmt_context *f) {
            f.on_error();
        }
    };

    // Can T be formatted with a custom formatter
    template <typename T>
    concept formattable = requires(T) { {formatter<T>{}}; };
}

template <formattable T>
void call_formatter_on_custom_arg(void *arg, fmt_context *f) {
    formatter<types::remove_cvref_t<T>> formatter;
    formatter.format((T *) (arg), f);
}

export {
    // Contains a value of any type
    struct fmt_value {
        struct custom {
            void *Data;
            void (*FormatFunc)(void *arg, fmt_context *f);
        };

        union {
            s64 S64;
            u64 U64;
            f32 F32;
            f64 F64;

            void *Pointer;
            string String;

            custom CUSTOM;
        };

        fmt_value(s64 v = 0) : S64(v) {}
        fmt_value(bool v) : S64(v) {}  // We store bools in S64
        fmt_value(u64 v) : U64(v) {}
        fmt_value(f32 v) : F32(v) {}
        fmt_value(f64 v) : F64(v) {}
        fmt_value(void *v) : Pointer(v) {}
        fmt_value(string v) : String(v) {}

        template <typename T>
        fmt_value(T *v) {
            CUSTOM.Data       = (void *) v;
            CUSTOM.FormatFunc = call_formatter_on_custom_arg<T>;
        }
    };

    // Holds a type and a value. If the value is not arithmetic (custom or string type)
    // then the life time of the parameter isn't extended (we just hold a pointer)!
    // That means that the parameters need to outlive the parse and format function itself.
    struct fmt_arg {
        fmt_type Type = fmt_type::NONE;
        fmt_value Value;
    };

    // Maps formatting arguments to types that can be used to construct a fmt_value.
    //
    // The order in which we look:
    //   * does the type have a formatter? maps to &v (value then setups a function call to formatter<T>::format())
    //   * is string constructible from T? then we map to string(T)
    //   * is the type a pointer? if it's non-void we throw an error, otherwise we map to (void *) v
    //   * is the type a bool? maps to bool
    //   * is the type an (un)integral? maps to u64 or s64
    //   * is the type a floating point? maps to f64
    //   * is the type a code_point_ref? maps to u64 (we want the value in that case)
    //   * is the type an enum? calls map_arg again with the underlying type
    // Otherwise we static_assert that the argument can't be formatted
    template <typename U>
    auto fmt_map_arg(const U &v) {
        using T = typename types::remove_cvref_t<U>;

        if constexpr (formattable<T>) {
            return &v;
        } else if constexpr (types::is_same<string, T> || types::is_constructible<string, T>) {
            return string(v);
        } else if constexpr (types::is_pointer<T>) {
            static_assert(types::is_same<T, void *>, "Formatting of non-void pointers is disallowed");
            return (void *) v;
        } else if constexpr (types::is_same<bool, T>) {
            return (bool) v;
        } else if constexpr (types::is_signed_integral<T>) {
            return (s64) v;
        } else if constexpr (types::is_unsigned_integral<T>) {
            return (u64) v;
        } else if constexpr (types::is_floating_point<T>) {
            return v;
        } else if constexpr (types::is_same<T, string::code_point_ref>) {
            return (u64) v;
        } else if constexpr (types::is_enum<T>) {
            return fmt_map_arg((types::underlying_type_t<T>) v);
        } else {
            static_assert(false, "Argument doesn't have a formatter");
        }
    }

    // !!!
    // If you get a compiler error here it's probably because you passed in an argument that can't be formatted
    // To format custom types, implement a formatter specialization.
    // !!!
    template <typename T>
    constexpr auto fmt_mapped_type_constant_v = type_constant_v<decltype(fmt_map_arg(types::declval<T>()))>;

    template <typename T>
    fmt_arg fmt_make_arg(const T &v) { return {fmt_mapped_type_constant_v<T>, fmt_value(fmt_map_arg(v))}; }

    template <bool IsPacked, typename T>
    auto fmt_make_arg(const T &v) {
        if constexpr (IsPacked) {
            return fmt_value(fmt_map_arg(v));  // We either pack values (we know their types in the fmg_args array)
        } else {
            return fmt_make_arg(v);  // .. or we don't have enough bits in the integer for everybody's type
                                         // and we store an array of fmt_args instead
        }
    }

    // Visits an argument dispatching with the right value based on the argument type
    template <typename Visitor>
    auto fmt_visit_fmt_arg(Visitor && visitor, fmt_arg ar)->decltype(visitor(0)) {
        switch (ar.Type) {
            case fmt_type::NONE:
                break;
            case fmt_type::S64:
                return visitor(ar.Value.S64);
            case fmt_type::U64:
                return visitor(ar.Value.U64);
            case fmt_type::BOOL:
                return visitor(ar.Value.S64 != 0);  // We store bools in S64
            case fmt_type::F32:
                return visitor(ar.Value.F32);
            case fmt_type::F64:
                return visitor(ar.Value.F64);
            case fmt_type::STRING:
                return visitor(ar.Value.String);
            case fmt_type::POINTER:
                return visitor(ar.Value.Pointer);
            case fmt_type::CUSTOM:
                return visitor(ar.Value.CUSTOM);
        }
        return visitor(types::unused{});
    }
}

// Hacky template because C++
template <typename Dummy>
u64 get_packed_fmt_types() {
    return 0ull;
}

// Hacky template because C++
template <typename Dummy, typename Arg, typename... Args>
u64 get_packed_fmt_types() {
    return (u64) fmt_mapped_type_constant_v<Arg> | (get_packed_fmt_types<Dummy, Args...>() << 4);
}

export {
    // We can't really combine this with _fmt_args_, ugh!
    // Stores either an array of values or arguments on the stack (just values if number is less than MAX_PACKED_ARGS)
    template <typename... Args>
    struct fmt_args_on_the_stack {
        static constexpr s64 NUM_ARGS   = sizeof...(Args);
        static constexpr bool IS_PACKED = NUM_ARGS < MAX_PACKED_ARGS;

        using T = types::select_t<IS_PACKED, fmt_value, fmt_arg>;
        stack_array<T, NUM_ARGS> Data;

        u64 Types;

        fmt_args_on_the_stack(Args &&...args) : Types(IS_PACKED ? get_packed_fmt_types<types::unused, Args...>() : IS_UNPACKED_BIT | NUM_ARGS) {
            Data = {fmt_make_arg<IS_PACKED>(args)...};
        }
    };

    struct fmt_args {
        void *Data;  // (fmt_value *) or (fmt_arg *) if not packed
        s64 Count = 0;
        u64 Types = 0;

        fmt_args() {}

        template <typename... Args>
        fmt_args(fmt_args_on_the_stack<Args...> *store) : Data((void *) store->Data.Data), Types(store->Types), Count(sizeof...(Args)) {}
    };
}

LSTD_END_NAMESPACE
