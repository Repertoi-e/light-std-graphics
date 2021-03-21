#include <lstd/fmt/fmt.h>
#include <lstd/parse.h>

#include "state.h"

void ui_main() {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("CDock Window", null, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("CDock");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("VSync", "", GameMemory->MainWindow->Flags & window::VSYNC))
                GameMemory->MainWindow->Flags ^= window::VSYNC;
            ImGui::EndMenu();
        }
        ImGui::TextDisabled("Info");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("This is an awesome calculator.");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("* Camera controls");
            ImGui::TextUnformatted("      Ctrl + Left Mouse -> Pan");
            ImGui::TextUnformatted("      Ctrl + Right Mouse -> Scale");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("This project is under the MIT license.");
            ImGui::TextUnformatted("Source code: github.com/Repertoi-e/light-std-graphics/");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void ui_scene_properties() {
    auto *cam = &GameState->Camera;

    ImGui::Begin("Scene", null);
    {
        ImGui::Text("Frame information:");
        ImGui::Text("  %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Clear color:");
        ImGui::ColorPicker3("", &GameState->ClearColor.x, ImGuiColorEditFlags_NoAlpha);
        if (ImGui::Button("Reset color")) GameState->ClearColor = {0.0f, 0.017f, 0.099f, 1.0f};
    }
    {
        if (ImGui::Button("Reset camera")) camera_reinit(cam);

        ImGui::Text("Position: %.3f, %.3f", cam->Position.x, cam->Position.y);
        ImGui::Text("Scale (zoom): %.3f, %.3f", cam->Scale.x, cam->Scale.y);

        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Pan speed", &cam->PanSpeed);
        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Zoom speed", &cam->ZoomSpeed);
        ImGui::InputFloat2("Zoom min/max", &cam->ZoomMin);
        if (ImGui::Button("Default camera constants")) camera_reset_constants(cam);
    }
    ImGui::End();
}

s32 op_precedence(string op) {
    if (op == "sentinel") return 0;

    if (op == "+") return 1;
    if (op == "-") return 1;

    if (op == "+unary") return 2;
    if (op == "-unary") return 2;
    if (op == "*") return 2;
    if (op == "*impl") return 2;
    if (op == "/") return 2;

    if (op == "^") return 3;

    assert(false);
    return -1;
}

constexpr s32 LEFT_ASSOCIATIVE = 0;
constexpr s32 RIGHT_ASSOCIATIVE = 1;

s32 op_associativity(string op) {
    if (op == "^") return RIGHT_ASSOCIATIVE;
    return LEFT_ASSOCIATIVE;
}

struct token {
    enum type { NONE = 0,
                REAL_NUMBER,
                WHOLE_NUMBER,
                OPERATOR,
                PARENTHESIS,
                VARIABLE };

    type Type;
    string Str;  // Always gets set to the initial representation of the token, used for error reporting

    union {
        u32 U32Value;
        f64 F64Value;
    };

    constexpr token(type t = NONE, string s = " ") : Type(t), Str(s) {}
    constexpr token(type t, string s, f64 f) : Type(t), Str(s), F64Value(f) {}
    constexpr token(type t, string s, u32 u) : Type(t), Str(s), U32Value(u) {}
};

// Only makes sense for Token_Type.OPERATOR.
// Decides if one operator should take precedence over the other
// and also takes into account operator associativity(left vs right).
bool operator>(const token &a, const token &b) {
    s32 p1 = op_precedence(a.Str);
    s32 p2 = op_precedence(b.Str);

    s32 a1 = op_associativity(a.Str);
    s32 a2 = op_associativity(b.Str);

    if (p1 > p2) return true;
    if (p1 == p2 && a1 == LEFT_ASSOCIATIVE) return true;
    return false;
}

bool operator==(const token &a, const token &b) { return a.Type == b.Type && a.Str == b.Str; }
bool operator!=(const token &a, const token &b) { return !(a == b); }

constexpr auto OP_SENTINEL = token(token::OPERATOR, "sentinel");

struct token_stream {
    string Expression;  // The original tokenized expression, used for error reporting.

    array<token> Tokens;
    array<token> It;  // We eat tokens from here, initially it equals Tokens, we must save Tokens in order to free the array.
                      // Note: We free the array, the tokens themselves are substrings and point in _Expression_ so they shouldn't be freed

    string Error = "";  // If there was an error, it gets stored here
};

void free(token_stream &stream) {
    free(stream.Tokens);
}

bool is_op(utf32 ch) { return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^'; }
bool is_unary_op(utf32 ch) { return ch == '+' || ch == '-'; }
bool is_parenthesis(utf32 ch) { return ch == '(' || ch == ')'; }

// errorPos = -1 means we calculate from _It_,
// if we are tokenizing, then the caller must pass the correct position (since we don't have a full token array yet!)
void error(token_stream &stream, string message, s64 errorPos = -1) {
    if (stream.Error) return;
    append_string(stream.Error, message);
    append_string(stream.Error, "\n  ");
    append_string(stream.Error, stream.Expression);

    if (errorPos == -1) {
        if (stream.It) {
            errorPos = stream.It[0].Str.Data - stream.Expression.Data;
        } else {
            errorPos = stream.Expression.Length;
        }
    }
    append_string(stream.Error, tsprint("\n  {: >{}}", "^", errorPos + 1));
}

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
                    append(stream.Tokens, {token::REAL_NUMBER, str, fltValue});
                } else {
                    // There was no dot.
                    auto str = s(0, -string(rest).Length);

                    s = rest;
                    append(stream.Tokens, {token::WHOLE_NUMBER, str, value});
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
bool is_token_a_number(token t) {
    return t.Type == token::WHOLE_NUMBER || t.Type == token::REAL_NUMBER;
}

void validate_e(token_stream &stream);

void validate_p(token_stream &stream) {
    auto next = peek(stream);
    if (next.Type == token::VARIABLE || is_token_a_number(next)) {
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
    while (!stream.Error && (peek(stream).Type == token::OPERATOR || peek(stream).Str == "(")) {
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

void ui_functions() {
    ImGui::Begin("Functions", null);
    {
        ImGui::Text("Enter expression:");
        if (ImGui::InputText("", GameState->Formula, GameState->FORMULA_INPUT_BUFFER_SIZE)) {
            string msg;
            WITH_ALLOC(Context.Temp) {
                auto tokens = tokenize(string(GameState->Formula));
                if (tokens.Error) {
                    msg = tokens.Error;
                } else {
                    validate_expression(tokens);
                    if (tokens.Error) {
                        msg = tokens.Error;
                    } else {
                        msg = "Success!";
                    }
                }
            }
            clone(&GameState->FormulaMessage, msg);
        }
        ImGui::Text(to_c_string(GameState->FormulaMessage, Context.Temp));
    }
    ImGui::End();
}