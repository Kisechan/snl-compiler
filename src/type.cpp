#include "type.h"

#include <sstream>
#include <utility>

namespace snl {

TypePtr make_primitive_type(TypeKind kind) {
    auto type = std::make_shared<Type>();
    type->kind = kind;
    switch (kind) {
        case TypeKind::Integer:
            type->name = "integer";
            type->size = 4;
            break;
        case TypeKind::Char:
            type->name = "char";
            type->size = 4;
            break;
        case TypeKind::Boolean:
            type->name = "boolean";
            type->size = 4;
            break;
        default:
            type->kind = TypeKind::Error;
            type->name = "error";
            type->size = 0;
            break;
    }
    return type;
}

TypePtr make_error_type() {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Error;
    type->name = "error";
    return type;
}

TypePtr make_alias_type(std::string name, TypePtr target) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Alias;
    type->name = std::move(name);
    type->target = std::move(target);
    type->size = type->target ? type->target->size : 0;
    return type;
}

TypePtr make_array_type(int low, int high, TypePtr element_type) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Array;
    type->name = "array";
    type->low = low;
    type->high = high;
    type->element_type = std::move(element_type);
    const int length = high >= low ? high - low + 1 : 0;
    type->size = length * (type->element_type ? type->element_type->size : 0);
    return type;
}

TypePtr make_record_type(std::vector<RecordField> fields) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Record;
    type->name = "record";
    type->fields = std::move(fields);
    int size = 0;
    for (RecordField& field : type->fields) {
        field.offset = size;
        field.size = field.type ? field.type->size : 0;
        size += field.size;
    }
    type->size = size;
    return type;
}

TypePtr make_procedure_type(std::vector<ParameterType> parameters, int level) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Procedure;
    type->name = "procedure";
    type->parameters = std::move(parameters);
    int offset = 0;
    for (ParameterType& parameter : type->parameters) {
        parameter.offset = offset;
        parameter.size = parameter.is_var ? 4 : (parameter.type ? parameter.type->size : 0);
        offset += parameter.size;
    }
    type->size = offset;
    type->low = level;
    return type;
}

TypePtr resolve_alias(TypePtr type) {
    while (type && type->kind == TypeKind::Alias && type->target) {
        type = type->target;
    }
    return type;
}

bool is_error_type(TypePtr type) {
    TypePtr resolved = resolve_alias(std::move(type));
    return !resolved || resolved->kind == TypeKind::Error;
}

bool type_equivalent(TypePtr lhs, TypePtr rhs) {
    lhs = resolve_alias(std::move(lhs));
    rhs = resolve_alias(std::move(rhs));
    if (!lhs || !rhs) {
        return false;
    }
    if (lhs->kind == TypeKind::Error || rhs->kind == TypeKind::Error) {
        return true;
    }
    if (lhs == rhs) {
        return true;
    }
    if (lhs->kind != rhs->kind) {
        return false;
    }
    switch (lhs->kind) {
        case TypeKind::Integer:
        case TypeKind::Char:
        case TypeKind::Boolean:
            return true;
        case TypeKind::Array:
            return lhs->low == rhs->low && lhs->high == rhs->high &&
                   type_equivalent(lhs->element_type, rhs->element_type);
        case TypeKind::Record:
            if (lhs->fields.size() != rhs->fields.size()) {
                return false;
            }
            for (std::size_t i = 0; i < lhs->fields.size(); ++i) {
                if (lhs->fields[i].name != rhs->fields[i].name ||
                    !type_equivalent(lhs->fields[i].type, rhs->fields[i].type)) {
                    return false;
                }
            }
            return true;
        case TypeKind::Procedure:
            return false;
        case TypeKind::Alias:
        case TypeKind::Error:
            return true;
    }
    return false;
}

std::string type_kind_name(TypeKind kind) {
    switch (kind) {
        case TypeKind::Integer:
            return "integer";
        case TypeKind::Char:
            return "char";
        case TypeKind::Boolean:
            return "boolean";
        case TypeKind::Array:
            return "array";
        case TypeKind::Record:
            return "record";
        case TypeKind::Alias:
            return "alias";
        case TypeKind::Procedure:
            return "procedure";
        case TypeKind::Error:
            return "error";
    }
    return "unknown";
}

std::string format_type(TypePtr type) {
    if (!type) {
        return "<null>";
    }
    if (type->kind == TypeKind::Alias) {
        return "alias " + type->name + " -> " + format_type(type->target);
    }

    TypePtr resolved = resolve_alias(type);
    if (!resolved) {
        return "<null>";
    }

    std::ostringstream out;
    switch (resolved->kind) {
        case TypeKind::Array:
            out << "array[" << resolved->low << ".." << resolved->high << "] of "
                << format_type(resolved->element_type);
            return out.str();
        case TypeKind::Record:
            out << "record{";
            for (std::size_t i = 0; i < resolved->fields.size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << resolved->fields[i].name << ":" << format_type(resolved->fields[i].type);
            }
            out << "}";
            return out.str();
        case TypeKind::Procedure:
            out << "procedure(";
            for (std::size_t i = 0; i < resolved->parameters.size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << (resolved->parameters[i].is_var ? "var " : "value ")
                    << resolved->parameters[i].name << ":"
                    << format_type(resolved->parameters[i].type);
            }
            out << ")";
            return out.str();
        default:
            return type_kind_name(resolved->kind);
    }
}

}  // namespace snl
