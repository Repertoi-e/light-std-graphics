#pragma once

#include <driver.h>

inline s32 op_precedence(string op, bool unary = false) {
    if (strings_match(op, "sentinel")) return 0;

    if (strings_match(op, "+") && !unary) return 1;
    if (strings_match(op, "-") && !unary) return 1;

    if (strings_match(op, "+")) return 2;
    if (strings_match(op, "-")) return 2;

    if (strings_match(op, "*")) return 2;
    if (strings_match(op, "/")) return 2;

    if (strings_match(op, "^")) return 3;

    assert(false);
    return -1;
}

constexpr s32 LEFT_ASSOCIATIVE  = 0;
constexpr s32 RIGHT_ASSOCIATIVE = 1;

inline s32 op_associativity(string op) {
    if (strings_match(op, "^")) return RIGHT_ASSOCIATIVE;
    return LEFT_ASSOCIATIVE;
}

struct token {
    enum type { NONE = 0,
                NUMBER,
                OPERATOR,
                PARENTHESIS,
                VARIABLE };

    type Type;
    string Str;  // Always gets set to the initial representation of the token, used for error reporting

    f64 F64Value;  // Only valid if type is NUMBER
    bool Unary;    // Only valid if type is OPERATOR. Note: We don't know this until we are at the parsing stage! This is used when building the AST.

    token(type t = NONE, string s = " ") : Type(t), Str(s), F64Value(0.0), Unary(false) {}
    token(type t, string s, f64 f) : Type(t), Str(s), F64Value(f), Unary(false) {}
};

// Only makes sense for Token_Type.OPERATOR.
// Decides if one operator should take precedence over the other
// and also takes into account operator associativity(left vs right).
inline bool operator>(const token &a, const token &b) {
    s32 p1 = op_precedence(a.Str, a.Unary);
    s32 p2 = op_precedence(b.Str, b.Unary);

    s32 a1 = op_associativity(a.Str);
    s32 a2 = op_associativity(b.Str);

    if (p1 > p2) return true;
    if (p1 == p2 && a1 == LEFT_ASSOCIATIVE) return true;
    return false;
}

inline bool operator==(const token &a, const token &b) { return a.Type == b.Type && a.Str == b.Str; }
inline bool operator!=(const token &a, const token &b) { return !(a == b); }

inline auto OP_SENTINEL = token(token::OPERATOR, "sentinel");

struct token_stream {
    string Expression;  // The original tokenized expression, used for error reporting.

    array<token> Tokens;
    array<token> It;  // We eat tokens from here, initially it equals Tokens, we must save Tokens in order to free the array.
                      // Note: We free the array, the tokens themselves are substrings and point in _Expression_ so they shouldn't be freed

    string Error = "";  // If there was an error, it gets stored here
};

inline void free(token_stream *stream) {
    free(stream->Tokens.Data);
}

inline bool is_op(code_point ch) { return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^'; }
inline bool is_unary_op(code_point ch) { return ch == '+' || ch == '-'; }
inline bool is_parenthesis(code_point ch) { return ch == '(' || ch == ')'; }

// errorPos = -1 means we calculate from _It_,
// if we are tokenizing, then the caller must pass the correct position (since we don't have a full token array yet!)
inline void error(token_stream *stream, string message, s64 errorPos = -1) {
    if (stream->Error) return;
    string_append(&stream->Error, message);
    string_append(&stream->Error, "\n  ");
    string_append(&stream->Error, stream->Expression);

    if (errorPos == -1) {
        if (stream->It) {
            errorPos = stream->It[0].Str.Data - stream->Expression.Data;
        } else {
            errorPos = string_length(stream->Expression);
        }
    }
    string_append(&stream->Error, mprint("\n  {: >{}}", "^", errorPos + 1));
}

struct ast {
    enum type {
        NONE,
        TERM,
        OP
    };

    type Type;
    ast *Left;
    ast *Right;

    // See note in state.h
    u32 ImGuiID;

    ast(type t = NONE, ast *left = null, ast *right = null);
};

// A term contains a bunch of letters (the variables which it depends on)
// It may also contain 0 letters (in that case it's simply a literal)
struct ast_term : ast {
    f64 Coeff;
    hash_table<code_point, s32> Letters;  // Key - letter, Value - power

    ast_term() : ast(TERM, null, null) {}

    bool is_literal() { return Letters.Count == 0; }
};

// If unary operator, only "left" node is used. "right" must always be None.
struct ast_op : ast {
    char Op;

    ast_op(char op = 0, ast *left = null, ast *right = null) : ast(OP, left, right), Op(op) {}
};

inline void free_ast(ast *node) {
    if (!node) return;

    if (node->Type == ast::OP) {
        free_ast(node->Left);
        free_ast(node->Right);
    } else if (node->Type == ast::TERM) {
        free_table(&((ast_term *) node)->Letters);
    }

    free(node);
}

[[nodiscard("Leak")]] token_stream tokenize(string s);
void validate_expression(token_stream *stream);
[[nodiscard("Leak")]] ast *parse_expression(token_stream *stream);
