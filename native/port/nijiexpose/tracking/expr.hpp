#pragma once
#include <string>
#include <cstdint>
namespace nijiexpose::tracking {
void insInitExpressions();
void insCleanupExpressions();
class Expression {
    std::string exprName_,expressionSource_,lastError_;
    bool updateState();
public:
    Expression(std::string signature,std::string source);
    std::string signature() const {return exprName_;}
    std::string expression() const {return expressionSource_;}
    bool expression(std::string value) {expressionSource_=std::move(value);return updateState();}
    std::string lastError() const {return lastError_;}
    float call();
};
std::string insExpressionGenerateSignature(uint32_t uuid,int axis);
}
