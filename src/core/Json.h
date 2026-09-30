#pragma once
#include <string>
#include <utility>
#include <vector>

namespace district_fury {

// Lector JSON minimo (sin dependencias) para manifiestos y datos externos.
// Soporta objeto, arreglo, cadena, numero, booleano y null. No escribe JSON.
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<JsonValue> array;
    std::vector<std::pair<std::string, JsonValue>> object;

    bool IsObject() const { return type == Type::Object; }
    bool IsArray() const { return type == Type::Array; }
    bool IsNumber() const { return type == Type::Number; }
    bool IsString() const { return type == Type::String; }

    // nullptr si no es objeto o no existe la clave.
    const JsonValue* Find(const std::string& key) const;
    double NumberOr(double fallback) const { return IsNumber() ? number : fallback; }
    bool BoolOr(bool fallback) const { return type == Type::Bool ? boolean : fallback; }
    std::string StringOr(const std::string& fallback) const { return IsString() ? string : fallback; }
};

// Devuelve true y llena `out` si el texto es JSON valido; si no, `error` describe
// el problema con su posicion.
bool ParseJson(const std::string& text, JsonValue& out, std::string& error);

}  // namespace district_fury
