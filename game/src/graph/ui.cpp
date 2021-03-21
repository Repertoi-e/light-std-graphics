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

    type Type = NONE;
    union {
        string StrValue;
        u32 U32Value;
        f64 F64Value;
    };

    constexpr token() : StrValue(string()) {}
    constexpr token(type t, string s) : Type(t), StrValue(s) {}
    constexpr token(type t, f64 f) : Type(t), F64Value(f) {}
    constexpr token(type t, u32 u) : Type(t), U32Value(u) {}
};

// Only makes sense for Token_Type.OPERATOR.
// Decides if one operator should take precedence over the other
// and also takes into account operator associativity(left vs right).
bool operator>(const token &a, const token &b) {
    s32 p1 = op_precedence(a.StrValue);
    s32 p2 = op_precedence(b.StrValue);

    s32 a1 = op_associativity(a.StrValue);
    s32 a2 = op_associativity(b.StrValue);

    if (p1 > p2) return true;
    if (p1 == p2 && a1 == LEFT_ASSOCIATIVE) return true;
    return false;
}

bool operator==(const token &a, const token &b) { return a.Type == b.Type && a.StrValue == b.StrValue; }
bool operator!=(const token &a, const token &b) { return !(a == b); }

constexpr auto OP_SENTINEL = token(token::OPERATOR, "sentinel");

struct token_stream {
    array<token> Tokens, It;

    string Error = "";
};

void free(token_stream &stream) {
    free(stream.Tokens);
}

bool is_op(utf32 ch) { return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^'; }
bool is_unary_op(utf32 ch) { return ch == '+' || ch == '-'; }
bool is_parenthesis(utf32 ch) { return ch == '(' || ch == ')'; }

template <>
struct lstd::formatter<token> {
    void format(const token &t, fmt_context *f) {
        if (t.Type == token::NONE) {
            format_tuple(f, "Token").field("None")->finish();
        } else if (t.Type == token::REAL_NUMBER) {
            format_tuple(f, "Token").field("REAL_NUMBER")->field(t.F64Value)->finish();
        } else if (t.Type == token::WHOLE_NUMBER) {
            format_tuple(f, "Token").field("WHOLE_NUMBER")->field(t.U32Value)->finish();
        } else {
            string typeStr = "INVALID";
            if (t.Type == token::OPERATOR) typeStr = "OPERATOR";
            if (t.Type == token::PARENTHESIS) typeStr = "PARENTHESIS";
            if (t.Type == token::VARIABLE) typeStr = "VARIABLE";
            format_tuple(f, "Token").field(typeStr)->field(t.StrValue)->finish();
        }
    }
};

void error(token_stream &stream, string message) {
    append_string(stream.Error, message);
    auto tokens = stream.It;
    if (tokens) {
        append_string(stream.Error, tsprint(", remaining tokens: ", tokens));
    }
}

extern "C" double strtod(const char *str, char **endptr);

[[nodiscard("Leak")]] token_stream tokenize(string s) {
    token_stream result;

    s = trim_end(s);

    while (true) {
        if (!s) break;
        s = trim_start(s);
        if (!s) break;

        if (is_digit(s[0]) || (s[0] == '.' && s.Length > 1 && is_digit(s[1]))) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(s);

            if (status == PARSE_INVALID || status == PARSE_EXHAUSTED) {
                error(result, tsprint("Invalid number while parsing: \"{}\"", s));
                return result;
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                error(result, tsprint("Number was too large: \"{}\"", s));
                return result;
            } else {
                // PARSE_SUCCESS
                if (rest && rest[0] == '.') {
                    // If there was a dot, roll back and parse a floating point number

                    auto *ch = to_c_string(s);
                    char *end;
                    f64 fltValue = strtod(ch, &end);
                    // u32 read = Numbers_DecToNum_flt64(&fltValue, to_c_string(s));
                    // if (read == 0) {
                    //     error(result, "Number was too large");
                    //     return result;
                    // } else if (read == -1) {
                    //     error(result, "Invalid number");
                    //     return result;
                    // }

                    s = s(end - ch, s.Length);
                    append(result.Tokens, {token::REAL_NUMBER, fltValue});
                } else {
                    // There was no dot.
                    s = rest;
                    append(result.Tokens, {token::WHOLE_NUMBER, value});
                }
            }
        } else if (is_op(s[0])) {
            append(result.Tokens, {token::OPERATOR, s(0, 1)});
            s = s(1, s.Length);
        } else if (is_parenthesis(s[0])) {
            append(result.Tokens, {token::PARENTHESIS, s(0, 1)});
            s = s(1, s.Length);
        } else if (is_alpha(s[0])) {
            append(result.Tokens, {token::VARIABLE, s(0, 1)});
            s = s(1, s.Length);
        } else {
            error(result, tsprint("Unexpected character when parsing: \"{}\"", s));
            return result;
        }
    }

    result.It = result.Tokens;

    return result;
}

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

/*
def validate_p(tokens):
    next_token = tokens.peek()
    if next_token.type in {Token_Type.VARIABLE, Token_Type.NUMBER}:
        tokens.consume()
    elif next_token.value == "(":
        tokens.consume()
        validate_e(tokens)
        tokens.expect(Token(Token_Type.PARENTHESIS, ")"))
    elif is_unary_op(next_token.value):
        tokens.consume()
        validate_p(tokens)
    else:
        tokens.error("Unexpected token")
    
def validate_e(tokens):
    validate_p(tokens)

    # "(" without operator beforehand means implicit * 
    while is_op(tokens.peek().value) or tokens.peek().value == "(":
        # We consume the operator but not the ( since that is part of "p"
        if tokens.peek().value != "(":
            tokens.consume()
        validate_p(tokens)

def validate_expression(tokens):
    validate_e(tokens)
    tokens.expect(Token(Token_Type.NONE))

                */

void validate_expression() {
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
                    msg = "Success!";
                }
            }
            clone(&GameState->FormulaMessage, msg);
        }
        ImGui::Text(to_c_string(GameState->FormulaMessage, Context.Temp));
    }
    ImGui::End();
}