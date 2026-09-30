#include "core/Json.h"
#include <cctype>
#include <cstdlib>

namespace district_fury {
namespace {

class Parser {
public:
    Parser(const std::string& t, std::string& e) : text_(t), error_(e) {}

    bool ParseDocument(JsonValue& out) {
        SkipSpace();
        if (!ParseValue(out, 0)) return false;
        SkipSpace();
        if (pos_ != text_.size()) return Fail("texto sobrante tras el valor raiz");
        return true;
    }

private:
    static constexpr int kMaxDepth = 64;

    bool Fail(const std::string& msg) {
        if (error_.empty()) error_ = msg + " (posicion " + std::to_string(pos_) + ")";
        return false;
    }
    void SkipSpace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    bool Consume(char c) {
        if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; }
        return false;
    }
    bool Literal(const char* word) {
        std::size_t n = 0;
        while (word[n]) ++n;
        if (text_.compare(pos_, n, word) != 0) return false;
        pos_ += n;
        return true;
    }

    bool ParseValue(JsonValue& v, int depth) {
        if (depth > kMaxDepth) return Fail("anidamiento excesivo");
        SkipSpace();
        if (pos_ >= text_.size()) return Fail("fin inesperado");
        const char c = text_[pos_];
        if (c == '{') return ParseObject(v, depth);
        if (c == '[') return ParseArray(v, depth);
        if (c == '"') { v.type = JsonValue::Type::String; return ParseString(v.string); }
        if (Literal("true"))  { v.type = JsonValue::Type::Bool; v.boolean = true; return true; }
        if (Literal("false")) { v.type = JsonValue::Type::Bool; v.boolean = false; return true; }
        if (Literal("null"))  { v.type = JsonValue::Type::Null; return true; }
        return ParseNumber(v);
    }

    bool ParseNumber(JsonValue& v) {
        const char* begin = text_.c_str() + pos_;
        char* end = nullptr;
        const double d = std::strtod(begin, &end);
        if (end == begin) return Fail("valor no reconocido");
        pos_ += static_cast<std::size_t>(end - begin);
        v.type = JsonValue::Type::Number;
        v.number = d;
        return true;
    }

    bool ParseString(std::string& out) {
        if (!Consume('"')) return Fail("se esperaba '\"'");
        out.clear();
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') return true;
            if (c != '\\') { out.push_back(c); continue; }
            if (pos_ >= text_.size()) break;
            const char e = text_[pos_++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    if (pos_ + 4 > text_.size()) return Fail("escape \\u incompleto");
                    const unsigned long cp = std::strtoul(text_.substr(pos_, 4).c_str(), nullptr, 16);
                    pos_ += 4;
                    // UTF-8 (solo plano basico; suficiente para textos de dialogo).
                    if (cp < 0x80) out.push_back(static_cast<char>(cp));
                    else if (cp < 0x800) {
                        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                    } else {
                        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                    }
                    break;
                }
                default: return Fail("escape invalido");
            }
        }
        return Fail("cadena sin cerrar");
    }

    bool ParseArray(JsonValue& v, int depth) {
        Consume('[');
        v.type = JsonValue::Type::Array;
        SkipSpace();
        if (Consume(']')) return true;
        while (true) {
            v.array.emplace_back();
            if (!ParseValue(v.array.back(), depth + 1)) return false;
            SkipSpace();
            if (Consume(',')) continue;
            if (Consume(']')) return true;
            return Fail("se esperaba ',' o ']'");
        }
    }

    bool ParseObject(JsonValue& v, int depth) {
        Consume('{');
        v.type = JsonValue::Type::Object;
        SkipSpace();
        if (Consume('}')) return true;
        while (true) {
            SkipSpace();
            std::string key;
            if (!ParseString(key)) return false;
            SkipSpace();
            if (!Consume(':')) return Fail("se esperaba ':'");
            v.object.emplace_back(std::move(key), JsonValue{});
            if (!ParseValue(v.object.back().second, depth + 1)) return false;
            SkipSpace();
            if (Consume(',')) continue;
            if (Consume('}')) return true;
            return Fail("se esperaba ',' o '}'");
        }
    }

    const std::string& text_;
    std::string& error_;
    std::size_t pos_ = 0;
};

}  // namespace

const JsonValue* JsonValue::Find(const std::string& key) const {
    if (type != Type::Object) return nullptr;
    for (const auto& kv : object) if (kv.first == key) return &kv.second;
    return nullptr;
}

bool ParseJson(const std::string& text, JsonValue& out, std::string& error) {
    error.clear();
    out = JsonValue{};
    Parser parser(text, error);
    return parser.ParseDocument(out);
}

}  // namespace district_fury
