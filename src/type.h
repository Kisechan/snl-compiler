#pragma once

#include <memory>
#include <string>
#include <vector>

namespace snl {

enum class TypeKind {
    Integer,
    Char,
    Boolean,
    Array,
    Record,
    Alias,
    Procedure,
    Error
};

struct Type;
using TypePtr = std::shared_ptr<Type>;

struct RecordField {
    std::string name;
    TypePtr type;
    int offset = 0;
    int size = 0;
};

struct ParameterType {
    std::string name;
    TypePtr type;
    bool is_var = false;
    int offset = 0;
    int size = 0;
};

struct Type {
    TypeKind kind = TypeKind::Error;
    std::string name;
    int size = 0;
    int low = 0;
    int high = -1;
    TypePtr element_type;
    TypePtr target;
    std::vector<RecordField> fields;
    std::vector<ParameterType> parameters;
};

TypePtr make_primitive_type(TypeKind kind);
TypePtr make_error_type();
TypePtr make_alias_type(std::string name, TypePtr target);
TypePtr make_array_type(int low, int high, TypePtr element_type);
TypePtr make_record_type(std::vector<RecordField> fields);
TypePtr make_procedure_type(std::vector<ParameterType> parameters, int level);

TypePtr resolve_alias(TypePtr type);
bool is_error_type(TypePtr type);
bool type_equivalent(TypePtr lhs, TypePtr rhs);
std::string type_kind_name(TypeKind kind);
std::string format_type(TypePtr type);

}  // namespace snl
