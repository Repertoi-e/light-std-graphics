#include "ast.h"

// @Cleanup
extern "C" double strtod(const char *str, char **endptr);

[[nodiscard("Leak")]] token_stream tokenize(string s) {
    token_stream stream;
    stream.Expression = s;

    s = trim_end(s);

    while (true) {
        if (!s) break;
        s = trim_start(s);
        if (!s) break;

        if (is_digit(s[0]) || (s[0] == '.' && s.Length > 1 && is_digit(s[1]))) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(s);

            if (status == PARSE_INVALID || status == PARSE_EXHAUSTED) {
                error(stream, "Invalid number while parsing", s.Data - stream.Expression.Data - 1);
                return stream;
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                error(stream, "Number was too large", s.Data - stream.Expression.Data - 1);
                return stream;
            } else {
                // PARSE_SUCCESS
                if (rest && rest[0] == '.') {
                    // If there was a dot, roll back and parse a floating point number

                    auto *ch = to_c_string(s);
                    char *end;

                    f64 fltValue = strtod(ch, &end);
                    // u32 read = Numbers_DecToNum_flt64(&fltValue, to_c_string(s));
                    // if (read == 0) {
                    //     error(stream, "Number was too large", ..);
                    //     return stream;
                    // } else if (read == -1) {
                    //     error(stream, "Invalid number", ..);
                    //     return stream;
                    // }

                    auto str = s(0, end - ch);

                    s = s(end - ch, s.Length);
                    append(stream.Tokens, {token::NUMBER, str, fltValue});
                } else {
                    // There was no dot.
                    auto str = s(0, s.Length - string(rest).Length);

                    s = rest;
                    append(stream.Tokens, {token::NUMBER, str, (f64) value});
                }
            }
        } else if (is_op(s[0])) {
            append(stream.Tokens, {token::OPERATOR, s(0, 1)});
            s = s(1, s.Length);
        } else if (is_parenthesis(s[0])) {
            append(stream.Tokens, {token::PARENTHESIS, s(0, 1)});
            s = s(1, s.Length);
        } else if (is_alpha(s[0])) {
            append(stream.Tokens, {token::VARIABLE, s(0, 1)});
            s = s(1, s.Length);
        } else {
            error(stream, tsprint("Unexpected character when parsing", s.Data - stream.Expression.Data - 1));
            return stream;
        }
    }

    stream.It = stream.Tokens;

    return stream;
}

token peek(token_stream &stream) {
    if (stream.It) return stream.It[0];
    return token();
}

token consume(token_stream &stream) {
    auto next = peek(stream);
    if (next.Type != token::NONE) {
        stream.It.Data++, stream.It.Count--;
    }
    return next;
}

void expect(token_stream &stream, token t) {
    auto next = peek(stream);
    if (next == t) {
        consume(stream);
    } else {
        string message = tsprint("Expected \"{}\"", t.Str);
        if (t.Type == token::NONE) message = "Unexpected token";
        error(stream, message);
    }
}

//
// Grammar:
//
//    v      --> Number literal | Variable (letter)
//    Binary --> "+" | "-" | "*" | "/" | "^" | "("
//    Unary  --> "+" | "-"
//
// Note: "("  is a binary operator because it means "implicit multiplication"
//
//    E      --> P {Binary P}
//    P      --> v | "(" E ")" | Unary P
//

void validate_e(token_stream &stream);

bool is_v(token t) { return t.Type == token::VARIABLE || t.Type == token::NUMBER; }

void validate_p(token_stream &stream) {
    auto next = peek(stream);
    if (is_v(next)) {
        consume(stream);
    } else if (next.Str == "(") {
        consume(stream);
        validate_e(stream);
        expect(stream, token(token::PARENTHESIS, ")"));
    } else if (is_unary_op(next.Str[0])) {
        consume(stream);
        validate_p(stream);
    } else if (next.Type == token::NONE) {
        error(stream, "Unexpected end of expression");
    } else {
        error(stream, "Unexpected token");
    }
}

void validate_e(token_stream &stream) {
    validate_p(stream);

    // "(" without operator beforehand means implicit *
    while (!stream.Error && (peek(stream).Type == token::OPERATOR || is_v(peek(stream)) || peek(stream).Str == "(")) {
        // We consume the operator but not the "(" since that is part of "p"
        if (peek(stream).Type == token::OPERATOR) {
            consume(stream);
        }
        validate_p(stream);
    }
}

void validate_expression(token_stream &stream) {
    validate_e(stream);
    expect(stream, token());
}

void pop_op(array<token> &ops, array<ast *> &operands) {
    if (ops[-1].Unary) {
        auto *t0 = operands[-1];
        remove_at_index(operands, -1);  // pop

        auto *unop = allocate<ast_op>();
        unop->Left = t0;
        unop->Op = ops[-1].Str[0];

        remove_at_index(ops, -1);  // pop

        append(operands, (ast *) unop);
    } else {
        auto *t1 = operands[-1];
        remove_at_index(operands, -1);  // pop

        auto *t0 = operands[-1];
        remove_at_index(operands, -1);  // pop

        auto *binop = allocate<ast_op>();
        binop->Left = t0;
        binop->Right = t1;

        binop->Op = ops[-1].Str[0];
        remove_at_index(ops, -1);  // pop

        append(operands, (ast *) binop);
    }
}

void push_op(token op, array<token> &ops, array<ast *> &operands) {
    while (ops[-1] > op) {
        pop_op(ops, operands);
    }
    append(ops, op);
}

void parse_e(token_stream &stream, array<token> &ops, array<ast *> &operands);

void parse_p(token_stream &stream, array<token> &ops, array<ast *> &operands) {
    auto next = peek(stream);
    if (is_v(next)) {
        auto *v = allocate<ast_variable>();
        if (next.Type == token::VARIABLE) {
            v->Coeff = 1;
            add(v->Letters, next.Str[0], 1);
        } else {
            v->Coeff = next.F64Value;
        }
        append(operands, (ast *) v);
        consume(stream);
    } else if (next.Str == "(") {
        consume(stream);

        append(ops, OP_SENTINEL);  // push sentinel

        parse_e(stream, ops, operands);
        expect(stream, token(token::PARENTHESIS, ")"));

        remove_at_index(ops, -1);  // pop sentinel
    } else if (is_unary_op(next.Str[0])) {
        auto op = token(token::OPERATOR, next.Str);
        op.Unary = true;
        push_op(op, ops, operands);

        consume(stream);

        parse_p(stream, ops, operands);
    } else if (next.Type == token::NONE) {
        error(stream, "Unexpected end of expression");
    } else {
        error(stream, "Unexpected token");
    }
}

void parse_e(token_stream &stream, array<token> &ops, array<ast *> &operands) {
    parse_p(stream, ops, operands);

    // "(" without operator beforehand means implicit *
    while (!stream.Error && (peek(stream).Type == token::OPERATOR || is_v(peek(stream)) || peek(stream).Str == "(")) {
        token op;

        // We consume the operator but not the "(" since that is part of "p"
        if (peek(stream).Type == token::OPERATOR) {
            op = token(token::OPERATOR, peek(stream).Str);
            consume(stream);
        } else {
            op = token(token::OPERATOR, "*");  // Passing a string literal here is fine since we don't do error checking when parsing
        }

        push_op(op, ops, operands);

        parse_p(stream, ops, operands);
    }

    while (ops[-1] != OP_SENTINEL) {
        pop_op(ops, operands);
    }
}

[[nodiscard("Leak")]] ast *parse_expression(token_stream &stream) {
    array<token> ops;  // @Speed @Cleanup Make these stacks
    array<ast *> operands;

    append(ops, OP_SENTINEL);

    parse_e(stream, ops, operands);
    expect(stream, token());

    auto *result = operands[-1];

    free(ops);
    free(operands);

    return result;
}
