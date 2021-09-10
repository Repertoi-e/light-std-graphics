module;

export module fmt.pretty;

import fmt.context;

LSTD_BEGIN_NAMESPACE

export {
    //
    // The following three classes are used to quickly collect elements and then output them in a pretty way.
    //
    // e.g. usage for a custom quaternion formatter:
    //     ...
    //     fmt_tuple(f, "quat").field(src.s)->field(src.i)->field(src.j)->field(src.k)->finish();
    // Outputs: "quat(1.00, 2.00, 3.00, 4.00)"
    //
    // These are inspired by Rust's API <3

    // Outputs in the following format: *name* { field1: value, field2: value, ... }
    // e.g.     vector3(x: 1.00, y: 4.00, z: 9.00)
    template <typename FC>
    struct format_struct {
        struct field_entry {
            string Name;
            fmt_arg<FC> Arg;
        };

        fmt_context *F;
        string Name;
        array<field_entry> Fields;
        bool NoSpecs;  // Write the result without taking into account specs for individual arguments

        format_struct(fmt_context *f, const string &name, bool noSpecs = false) : F(f), Name(name), NoSpecs(noSpecs) {}

        // I know we are against hidden freeing but having this destructor is fine because it helps with code conciseness.
        ~format_struct() { free(Fields); }

        template <typename T>
        format_struct *field(const string &name, const T &value) {
            array_append(Fields, {name, fmt_make_arg<FC>(value)});
            return this;
        }

        void finish();
    };

    // Outputs in the following format: *name*(element1, element2, ...)
    // e.g.     read_file_result("Hello world!", true)
    template <typename FC>
    struct format_tuple {
        FC *F;
        string Name;
        array<fmt_arg<FC>> Fields;
        bool NoSpecs;  // Write the result without taking into account specs for individual arguments

        format_tuple(FC *f, const string &name, bool noSpecs = false) : F(f), Name(name), NoSpecs(noSpecs) {}

        // I know we are against hidden freeing but having this destructor is fine because it helps with code conciseness.
        ~format_tuple() { free(Fields); }

        template <typename T>
        format_tuple *field(const T &value) {
            array_append(Fields, fmt_make_arg<FC>(value));
            return this;
        }

        void finish();
    };

    // Outputs in the following format: [element1, element2, ...]
    // e.g.     ["This", "is", "an", "array", "of", "strings"]
    template <typename FC>
    struct format_list {
        FC *F;
        array<fmt_arg<FC>> Fields;
        bool NoSpecs;  // Write the result without taking into account specs for individual arguments

        format_list(FC *f, bool noSpecs = false) : F(f), NoSpecs(noSpecs) {}

        // I know we are against hidden freeing but having this destructor is fine because it helps with code conciseness.
        ~format_list() { free(Fields); }

        template <typename T>
        format_list *entries(const array<T> &values) {
            For(values) array_append(Fields, fmt_make_arg<FC>(it));
            return this;
        }

        template <typename T>
        format_list *entries(T *begin, T *end) {
            return entries(array<T>(begin, end - begin));
        }

        template <typename T>
        format_list *entries(T *begin, s64 count) {
            return entries(array<T>(begin, count));
        }

        void finish();
    };
}

template <typename FC>
void format_struct<FC>::finish() {
    auto write_field = [&](field_entry *entry) {
        write_no_specs(F, entry->Name);
        write_no_specs(F, ": ");
        fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), entry->Arg);
    };

    write_no_specs(F, Name);
    write_no_specs(F, " {");

    auto *p = Fields.begin();
    if (p != Fields.end()) {
        write_no_specs(F, " ");
        write_field(p);
        ++p;
        while (p != Fields.end()) {
            write_no_specs(F, ", ");
            write_field(p);
            ++p;
        }
    }
    write_no_specs(F, " }");
}

template <typename FC>
void format_tuple<FC>::finish() {
    write_no_specs(F, Name);
    write_no_specs(F, "(");

    auto *p = Fields.begin();
    if (p != Fields.end()) {
        fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *p);
        ++p;
        while (p != Fields.end()) {
            write_no_specs(F, ", ");
            fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *p);
            ++p;
        }
    }
    write_no_specs(F, ")");
}

template <typename FC>
void format_list<FC>::finish() {
    write_no_specs(F, "[");

    auto *p = Fields.begin();
    if (p != Fields.end()) {
        fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *p);
        ++p;
        while (p != Fields.end()) {
            write_no_specs(F, ", ");
            fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *p);
            ++p;
        }
    }
    write_no_specs(F, "]");
}

LSTD_END_NAMESPACE
