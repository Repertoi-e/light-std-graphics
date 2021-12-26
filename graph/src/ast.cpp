#include "ast.h"

#include <driver.h>

// @Cleanup
extern "C" double strtod(const char *str, char **endptr);

[[nodiscard("Leak")]] token_stream tokenize(string s) {
    token_stream stream;
    make_dynamic(&stream.Tokens, 20);
    make_dynamic(&stream.Error, 20);

    stream.Expression = s;  // We save the original string in order to assist with error reporting

    while (true) {
        if (!s) break;

        // We make direct modifications (just substringing, no allocations) to the _s_ variable
        s = trim_start(s);
        if (!s) break;

        if (is_digit(s[0])) {
            auto *ch = string_to_c_string(s);
            char *end;

            f64 fltValue = strtod(ch, &end);  // @DependencyCleanup

            auto str = substring(s, 0, end - ch);
            s        = substring(s, end - ch, string_length(s));

            add(&stream.Tokens, token{token::NUMBER, str, fltValue});
        } else if (is_op(s[0])) {
            add(&stream.Tokens, token{token::OPERATOR, substring(s, 0, 1)});
            s = substring(s, 1, string_length(s));
        } else if (is_parenthesis(s[0])) {
            add(&stream.Tokens, token{token::PARENTHESIS, substring(s, 0, 1)});
            s = substring(s, 1, string_length(s));
        } else if (is_alpha(s[0])) {
            add(&stream.Tokens, token{token::VARIABLE, substring(s, 0, 1)});
            s = substring(s, 1, string_length(s));
        } else {
            error(&stream, "Unexpected character when parsing", s.Data - stream.Expression.Data);
            return stream;
        }
    }

    stream.It = stream.Tokens;

    return stream;
}

token peek(token_stream *stream) {
    if (stream->It) return stream->It[0];
    return token();
}

token consume(token_stream *stream) {
    auto next = peek(stream);
    if (next.Type != token::NONE) {
        stream->It.Data++, stream->It.Count--;
    }
    return next;
}

void expect(token_stream *stream, token t) {
    auto next = peek(stream);
    if (next == t) {
        consume(stream);
    } else {
        string message = mprint("Expected \"{}\"", t.Str);
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

void validate_e(token_stream *stream);

bool is_v(token t) { return t.Type == token::VARIABLE || t.Type == token::NUMBER; }

void validate_p(token_stream *stream) {
    auto next = peek(stream);
    if (is_v(next)) {
        consume(stream);
    } else if (strings_match(next.Str, "(")) {
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

void validate_e(token_stream *stream) {
    validate_p(stream);

    // :ImplicitTimes:
    // "(" or a term without operator beforehand means implicit *
    //
    // This means that we treat the following expressions as multiplications:
    //     xy      = x * y
    //     2x      = 2 * x
    //     2(x)    = 2 * x
    //     2(...)  = 2 * (...)
    //     2 2     = 2 * 2               <- side effect of allowing x2 = 2 * x, we don't check if the previous token was a number, should we?
    while (!stream->Error && (peek(stream).Type == token::OPERATOR || is_v(peek(stream)) || strings_match(peek(stream).Str, "("))) {
        // We consume the operator but not the "(" or the following term (that is the job of validate_p())
        if (peek(stream).Type == token::OPERATOR) {
            consume(stream);
            if (peek(stream).Type == token::OPERATOR) {
                error(stream, "Operator after another operator");
            }
        }
        validate_p(stream);
    }
}

void validate_expression(token_stream *stream) {
    validate_e(stream);
    expect(stream, token());
}

/*
def optimize_plus(l, r):
    if l.letters == r.letters:
        l.coeff += r.coeff
        return l
    return None

def optimize_times(l, r):
    l.coeff *= r.coeff
    for key, value in r.letters.items():
        new_value = l.letters.get(key, 0) + value
        l.letters[key] = new_value
    return l
       
def collapse_unary(op, t0):
    if t0.type == Ast_Type.TERM:
        if op == "-unary":
            t0.coeff *= -1
            return t0
    return None
    
def collapse_binary(op, t0, t1):
    if t0.type == Ast_Type.TERM and t1.type == Ast_Type.TERM:
        if op == "+":
            optimized = optimize_plus(t0, t1)
            if optimized: return optimized
        elif op == "*":
            return optimize_times(t0, t1)
    return None
*/

void pop_op(array<token> ref_volatile ops, array<ast *> ref_volatile operands) {
    if (ops[-1].Unary) {
        auto *t0 = operands[-1];
        pop(&operands);

        auto top = ops[-1];
        auto op = top.Str[0];
        pop(&ops);

        // Now we try to "collapse" - avoid a new ast_op if the term is a variable and we can directly negate the coefficient
        ast *toPush = null;
        if (op == '+') {
            // Do nothing
            toPush = t0;
        } else if (op == '-') {
            if (t0->Type == ast::TERM) {
                auto *var = (ast_term *) t0;
                var->Coeff *= -1;

                toPush = t0;
            }
        } else {
            assert(false && "What?? Should be unary operator..");
        }

        // We couldn't collapse... create a new ast node.
        if (!toPush) {
            auto *unop = malloc<ast_op>();
            unop->Left = t0;
            unop->Op   = (char) op;

            toPush = unop;
        }

        add(&operands, toPush);
    } else {
        auto *t1 = operands[-1];
        pop(&operands);

        auto *t0 = operands[-1];
        pop(&operands);

        auto op = ops[-1].Str[0];
        pop(&ops);

        ast *toPush = null;

        // Now we try to "collapse" - avoid a new ast_op if both of the terms are variables and we can do the operation directly
        if (t0->Type == ast::TERM && t1->Type == ast::TERM) {
            auto *l = (ast_term *) t0;
            auto *r = (ast_term *) t1;

            if (op == '+') {
                // We can add the coefficients only if all letters match
                if (l->Letters == r->Letters) {
                    l->Coeff += r->Coeff;
                    toPush = l;
                }
            } else if (op == '-') {
                // We can subtract the coefficients only if all letters match
                if (l->Letters == r->Letters) {
                    l->Coeff -= r->Coeff;
                    toPush = l;
                }
            } else if (op == '*') {
                // We multiply the coefficients and add the powers of every letter

                l->Coeff *= r->Coeff;
                for (auto [k, v] : r->Letters) {
                    if (has(&l->Letters, *k)) {
                        *(l->Letters[*k]) += (*v);
                    } else {
                        *(l->Letters[*k]) = (*v);
                    }
                }
                toPush = l;
            } else if (op == '/') {
                // We divide the coefficients and subtract the powers of every letter

                l->Coeff /= r->Coeff;
                for (auto [k, v] : r->Letters) {
                    if (has(&l->Letters, *k)) {
                        *(l->Letters[*k]) -= (*v);
                    } else {
                        *(l->Letters[*k]) = -(*v);
                    }
                }
                toPush = l;
            }
        }

        // We couldn't collapse... create a new ast node.
        if (!toPush) {
            auto *binop  = malloc<ast_op>();
            binop->Left  = t0;
            binop->Right = t1;
            binop->Op    = (char) op;

            toPush = binop;
        }

        add(&operands, toPush);
    }
}

void push_op(token op, array<token> ref_volatile ops, array<ast *> ref_volatile operands) {
    while (ops[-1] > op) {
        pop_op(ops, operands);
    }
    add(&ops, op);
}

void parse_e(token_stream *stream, array<token> ref_volatile ops, array<ast *> ref_volatile operands);

void parse_p(token_stream *stream, array<token> ref_volatile ops, array<ast *> ref_volatile operands) {
    auto next = peek(stream);
    if (is_v(next)) {
        auto *v = malloc<ast_term>();
        if (next.Type == token::VARIABLE) {
            v->Coeff = 1;
            add(&v->Letters, next.Str[0], 1);
        } else {
            v->Coeff = next.F64Value;
        }
        add(&operands, (ast *) v);
        consume(stream);
    } else if (strings_match(next.Str, "(")) {
        consume(stream);

        add(&ops, OP_SENTINEL);  // push sentinel

        parse_e(stream, ops, operands);
        expect(stream, token(token::PARENTHESIS, ")"));

        pop(&ops);
    } else if (is_unary_op(next.Str[0])) {
        auto op  = token(token::OPERATOR, next.Str);
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

void parse_e(token_stream *stream, array<token> ref_volatile ops, array<ast *> ref_volatile operands) {
    parse_p(stream, ops, operands);

    // :ImplicitTimes: See comment in validate_e.
    while (!stream->Error && (peek(stream).Type == token::OPERATOR || is_v(peek(stream)) || strings_match(peek(stream).Str, "("))) {
        token op;

        // We consume the operator but not the "(" since that is part of "p"
        if (peek(stream).Type == token::OPERATOR) {
            op = token(token::OPERATOR, peek(stream).Str);
            consume(stream);
        } else {
            op = token(token::OPERATOR, "*");  // Passing a string literal here is fine since we shouldn't report errors when parsing
        }

        push_op(op, ops, operands);

        parse_p(stream, ops, operands);
    }

    while (ops[-1] != OP_SENTINEL) {
        pop_op(ops, operands);
    }
}

[[nodiscard("Leak")]] ast *parse_expression(token_stream *stream) {
    array<token> ops;  // @Speed @Cleanup Make these stacks
    array<ast *> operands;
    make_dynamic(&ops, 8);
    make_dynamic(&operands, 8);
    defer(free(ops.Data));
    defer(free(operands.Data));

    add(&ops, OP_SENTINEL);

    parse_e(stream, ops, operands);
    expect(stream, token());

    auto *result = operands[-1];
    return result;
}
