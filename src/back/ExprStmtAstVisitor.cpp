#include "emit.h"
#include "CompileAstVisitor.h"
#include "common/containers.h"
#include "front/type.h"

namespace anode { namespace back {
using namespace anode::front;

class ExprStmtAstVisitor : public CompileAstVisitor {
public:
    bool shouldVisitChildren() override { return false; }

private:
    bool valueIsSet_ = false;
    llvm::Value *llvmValue_ = nullptr;

    void setValue(llvm::Value *value) {
        ASSERT(!valueIsSet_ && "setValue(llvm::Value*) should only be invoked once.");
        llvmValue_ = value;
        valueIsSet_ = true;
    }

    llvm::Value *getLiteralIntLlvmValue(int value) {
        return llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, (uint64_t) value, true));
    }

    llvm::Value *getLiteralUIntLLvmValue(unsigned value) {
        return llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, (uint64_t) value, false));
    }

public:
    explicit ExprStmtAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) {}

    llvm::Value *llvmValue() {
        ASSERT(valueIsSet_ && "Value wasn't set!  Are you missing a visit* method?")
        return llvmValue_;
    }

    void visitingCompleteClassDefinition(ast::CompleteClassDefinition &) override {
        //this is handled in another visitor
        setValue(nullptr);
    }

    void visitingFuncDefStmt(ast::FuncDefStmt &) override {
        //this is handled in another visitor
        setValue(nullptr);
    }

    void visitingAnonymousTemplateExprStmt(ast::AnonymousTemplateExprStmt &) override {
        //this is never visited directly
        setValue(nullptr);
    }

    void visitingNamedTemplateExprStmt(ast::NamedTemplateExprStmt &) override {
        //this is never visited directly
        setValue(nullptr);
    }

    void visitMethodRefExpr(ast::MethodRefExpr &methodRefExpr) override {
        setValue(cc().getMappedValue(methodRefExpr.symbol()));
    }

    void visitingTemplateExpansionExprStmt(ast::TemplateExpansionExprStmt &expansion) override {
        setValue(emitExpr(*expansion.expandedTemplate(), cc()));
    }

    void visitedFuncCallExpr(ast::FuncCallExpr &expr) override {

        std::vector<llvm::Value *> args;

        if (expr.instanceExpr()) {
            args.push_back(emitExpr(*expr.instanceExpr(), cc()));
        }

        for (auto arg : expr.arguments()) {
            args.push_back(emitExpr(arg.get(), cc()));
        }

        llvm::Value *value = emitExpr(expr.funcExpr(), cc());

        auto *llvmFunc = llvm::cast<llvm::Function>(value);

        llvm::CallInst *result = cc().irBuilder().CreateCall(llvmFunc, args);

        setValue(result);
    }

    void visitedCastExpr(ast::CastExpr &expr) override {
        llvm::Value *value = emitExpr(expr.valueExpr(), cc());

        ASSERT(expr.exprType().isPrimitive());
        ASSERT(expr.valueExpr().exprType().isPrimitive());

        llvm::Value *castedValue = nullptr;
        llvm::Type *destLlvmType = cc().typeMap().toLlvmType(expr.exprType());
        switch (expr.valueExpr().exprType().primitiveType()) {
            //From int32
            case type::PrimitiveType::Int32:
                switch (expr.exprType().primitiveType()) {
                    //To bool
                    case type::PrimitiveType::Bool:
                        castedValue = cc().irBuilder().CreateICmpNE(value, cc().getDefaultValueForType(expr.valueExpr().exprType()));
                        break;
                        //To int
                    case type::PrimitiveType::Int32:
                        castedValue = value;
                        break;
                        //To float
                    case type::PrimitiveType::Float:
                        castedValue = cc().irBuilder().CreateSIToFP(value, destLlvmType);
                        break;
                    default:
                        ASSERT_FAIL("Unknown cast from int32");
                }
                break;
                //From float
            case type::PrimitiveType::Float:
                switch (expr.exprType().primitiveType()) {
                    //To to int32 or bool
                    case type::PrimitiveType::Bool:
                        castedValue = cc().irBuilder().CreateFCmpUNE(value, cc().getDefaultValueForType(expr.valueExpr().exprType()));
                        break;
                    case type::PrimitiveType::Int32:
                        castedValue = cc().irBuilder().CreateFPToSI(value, destLlvmType);
                        break;
                    case type::PrimitiveType::Float:
                        castedValue = value;
                        break;
                    default:
                        ASSERT_FAIL("Unknown cast from float");
                }
                break;
                //From bool
            case type::PrimitiveType::Bool:
                ASSERT_FAIL("cannot cast from bool to anything");
                break;
            default:
                ASSERT_FAIL("Unhandled type::PrimitiveType")
        }
        ASSERT(castedValue && "Cannot fail to create casted value...");
        setValue(castedValue);
    }

    void visitingNewExpr(ast::NewExpr &expr) override {
        auto structType = cc().typeMap().toLlvmType(expr.exprType())->getPointerElementType();
        uint64_t size = cc().llvmModule().getDataLayout().getTypeAllocSize(structType);

        std::vector<llvm::Value *> arguments;
        arguments.push_back(getLiteralUIntLLvmValue((unsigned int) size));
        llvm::Value *pointer = cc().irBuilder().CreateCall(cc().mallocFunc(), arguments);
        llvm::Value *castedValue = cc().irBuilder().CreatePointerCast(pointer, cc().typeMap().toLlvmType(expr.exprType()));

        setValue(castedValue);
    }

    void visitedVariableDeclExpr(ast::VariableDeclExpr &expr) override {
        switch (expr.symbol()->storageKind()) {
            case scope::StorageKind::Global: {
                llvm::GlobalVariable *globalVariable = cc().llvmModule().getNamedGlobal(expr.symbol()->fullyQualifiedName());
                ASSERT(globalVariable);
                globalVariable->setAlignment(ALIGNMENT);
                llvm::Constant *val = cc().getDefaultValueForType(expr.exprType());
                globalVariable->setInitializer(val);
                break;
            }
            case scope::StorageKind::Local: {
                ASSERT(expr.name().size() == 1 && "TODO: semantic error or refactor VariableDeclExpr and VariableRefExpr so they don't both have to use MultiPartIdentifier");
                llvm::Type *localVariableType = cc().typeMap().toLlvmType(expr.exprType());
                llvm::Value *localVariable = cc().irBuilder().CreateAlloca(localVariableType, nullptr, expr.name().front().text());
                cc().mapSymbolToValue(*expr.symbol(), localVariable);
                break;
            }
            default:
                ASSERT_FAIL("Unhandled StorageKind");
        }
        visitVariableRefExpr(expr);
    }

    void visitVariableRefExpr(ast::VariableRefExpr &expr) override {
        llvm::Value *pointer;

        if (expr.symbol()->storageKind() == scope::StorageKind::Instance) {
            //Variable is an instance field
            scope::VariableSymbol *thisSymbol = cc().currentFuncDefStmt()->symbol()->thisSymbol();
            auto classType = dynamic_cast<type::ClassType *>(&thisSymbol->type());
            ASSERT(classType);
            llvm::Value *pointerToPointerToStruct = cc().getMappedValue(thisSymbol);
            llvm::Value *pointerToStruct = cc().irBuilder().CreateLoad(pointerToPointerToStruct);
            ASSERT(expr.name().size() == 1);
            pointer = createStructGep(classType, pointerToStruct, expr.name().front().text());
        } else {
            //Variable is an argument or local variable.
            pointer = cc().getMappedValue(expr.symbol());
        }

        ASSERT(pointer);

        if (expr.variableAccess() == ast::VariableAccess::Write) {
            setValue(pointer);
            return;
        }

        if (!expr.exprType().isFunction()) {
            llvm::LoadInst *loadInst = cc().irBuilder().CreateLoad(pointer);
            loadInst->setAlignment(ALIGNMENT);
            setValue(loadInst);
        } else {
            setValue(pointer);
        }
    }

private:
    llvm::Value *createStructGep(type::ClassType *classType, llvm::Value *instance, const std::string &memberName) {
        ASSERT(classType != nullptr);
        type::ClassField *classField = classType->findField(memberName);
        unsigned ordinal = classField->ordinal();

        //First emit code to calculate the address of the member field
        llvm::Value *ptr = cc().irBuilder().CreateStructGEP(nullptr, instance, ordinal, classField->name());
        return ptr;
    }

protected:

    void visitedDotExpr(ast::DotExpr &expr) override {
        llvm::Value *instance = emitExpr(expr.lValue(), cc());

        auto classType = dynamic_cast<type::ClassType *>(expr.lValue().exprType().actualType());
        ASSERT(classType != nullptr && "lvalues of dot operator must be a ClassType (did the semantic check fail?)");

        llvm::Value *ptrOrValue = createStructGep(classType, instance, expr.memberName().text());

        if (expr.isWrite() || expr.exprType().isFunction()) {
            setValue(ptrOrValue);
            return;
        }

        ptrOrValue = cc().irBuilder().CreateLoad(ptrOrValue);
        setValue(ptrOrValue);
    }

    void visitLiteralInt32Expr(ast::LiteralInt32Expr &expr) override {
        setValue(getLiteralIntLlvmValue(expr.value()));
    }

    void visitLiteralFloatExpr(ast::LiteralFloatExpr &expr) override {
        setValue(llvm::ConstantFP::get(cc().llvmContext(), llvm::APFloat(expr.value())));
    }

    void visitLiteralBoolExpr(ast::LiteralBoolExpr &expr) override {
        setValue(llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(1, (uint64_t) expr.value(), true)));
    }

    void visitingBinaryExpr(ast::BinaryExpr &expr) override {
        switch (expr.binaryExprKind()) {
            case ast::BinaryExprKind::Logical:
                emitBinaryLogical(expr);
                break;
            case ast::BinaryExprKind::Arithmetic:
                emitBinaryArithmetic(expr);
                break;
            default:
                ASSERT_FAIL("Unhandled BinaryExprKind");
        }
    }

private:
    void emitBinaryArithmetic(ast::BinaryExpr &expr) {
        ASSERT(expr.lValue().exprType().isSameType(expr.rValue().exprType()) && "data types must match");

        llvm::Value *rValue = emitExpr(expr.rValue(), cc());

        llvm::Value *lValue = emitExpr(expr.lValue(), cc());

        if (expr.operation() == ast::BinaryOperationKind::Assign) {
            cc().irBuilder().CreateStore(rValue, lValue); //(args swapped on purpose)
            //TODO:  I think this will call rValue a second time if rValue is a call site.  Improve this...
            setValue(rValue);
            return;
        }
        llvm::Value *resultValue = nullptr;
        ASSERT(expr.exprType().isPrimitive() && "Only primitive types currently supported here");
        switch (expr.operandsType().primitiveType()) {
            case type::PrimitiveType::Bool:
                switch (expr.operation()) {
                    case ast::BinaryOperationKind::Eq:
                        resultValue = cc().irBuilder().CreateICmpEQ(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::NotEq:
                        resultValue = cc().irBuilder().CreateICmpNE(lValue, rValue);
                        break;
                    default:
                        ASSERT_FAIL("Unsupported BinaryOperationKind for bool primitive type");
                }
                break;
            case type::PrimitiveType::Int32:
                switch (expr.operation()) {
                    case ast::BinaryOperationKind::Eq:
                        resultValue = cc().irBuilder().CreateICmpEQ(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::NotEq:
                        resultValue = cc().irBuilder().CreateICmpNE(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Add:
                        resultValue = cc().irBuilder().CreateAdd(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Sub:
                        resultValue = cc().irBuilder().CreateSub(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Mul:
                        resultValue = cc().irBuilder().CreateMul(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Div:
                        resultValue = cc().irBuilder().CreateSDiv(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::GreaterThan:
                        resultValue = cc().irBuilder().CreateICmpSGT(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::GreaterThanOrEqual:
                        resultValue = cc().irBuilder().CreateICmpSGE(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::LessThan:
                        resultValue = cc().irBuilder().CreateICmpSLT(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::LessThanOrEqual:
                        resultValue = cc().irBuilder().CreateICmpSLE(lValue, rValue);
                        break;
                    default:
                        ASSERT_FAIL("Unhandled BinaryOperationKind (supposed to be an arithmetic operator)");
                }
                break;
            case type::PrimitiveType::Float:
                switch (expr.operation()) {
                    case ast::BinaryOperationKind::Eq:
                        resultValue = cc().irBuilder().CreateFCmpOEQ(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::NotEq:
                        resultValue = cc().irBuilder().CreateFCmpONE(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Add:
                        resultValue = cc().irBuilder().CreateFAdd(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Sub:
                        resultValue = cc().irBuilder().CreateFSub(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Mul:
                        resultValue = cc().irBuilder().CreateFMul(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::Div:
                        resultValue = cc().irBuilder().CreateFDiv(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::GreaterThan:
                        resultValue = cc().irBuilder().CreateFCmpOGT(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::GreaterThanOrEqual:
                        resultValue = cc().irBuilder().CreateFCmpOGE(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::LessThan:
                        resultValue = cc().irBuilder().CreateFCmpOLT(lValue, rValue);
                        break;
                    case ast::BinaryOperationKind::LessThanOrEqual:
                        resultValue = cc().irBuilder().CreateFCmpOLE(lValue, rValue);
                        break;
                    default:
                        ASSERT_FAIL("Unhandled BinaryOperationKind (supposed to be an arithmetic operator)");
                }
                break;
            default:
                ASSERT_FAIL("Unhandled PrimitiveType");
        }
        ASSERT(resultValue);
        setValue(resultValue);
    }

    void emitBinaryLogical(ast::BinaryExpr &expr) {
        //Prepare the basic blocks
        llvm::BasicBlock *lValueBlock = cc().irBuilder().GetInsertBlock();
        llvm::Function *currentFunc = lValueBlock->getParent();
        llvm::BasicBlock *rValueBlock = llvm::BasicBlock::Create(cc().llvmContext(), "rValueBlock");
        llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "endBlock");

        //Emit the lValue, which is always the first operand of a logical operation to be evaluated
        llvm::Value *lValue = emitExpr(expr.lValue(), cc());

        bool lValueConst;
        switch (expr.operation()) {
            //For the && operator, only evaluate the lValue if the rValue is true
            case ast::BinaryOperationKind::LogicalAnd:
                cc().irBuilder().CreateCondBr(lValue, rValueBlock, endBlock);
                lValueConst = false;
                break;
            case ast::BinaryOperationKind::LogicalOr:
                cc().irBuilder().CreateCondBr(lValue, endBlock, rValueBlock);
                lValueConst = true;
                break;
            default:
                ASSERT_FAIL("Unhandled BinaryOperationKind (supposed to be a logical operator)")
        }
        currentFunc->getBasicBlockList().push_back(rValueBlock);
        cc().irBuilder().SetInsertPoint(rValueBlock);
        llvm::Value *rValue = emitExpr(expr.rValue(), cc());
        cc().irBuilder().CreateBr(endBlock);

        currentFunc->getBasicBlockList().push_back(endBlock);
        cc().irBuilder().SetInsertPoint(endBlock);
        llvm::PHINode *phi = cc().irBuilder().CreatePHI(cc().typeMap().toLlvmType(type::ScalarType::Bool), 2, "logical_tmp");

        llvm::ConstantInt *shortCircuitLlvmValue
            = llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(1, (uint64_t) lValueConst, false));
        phi->addIncoming(shortCircuitLlvmValue, lValueBlock);
        phi->addIncoming(rValue, rValueBlock);

        setValue(phi);
    }

protected:
    void visitingIfExpr(ast::IfExprStmt &ifExpr) override {
        //This function is modeled after: https://llvm.org/docs/tutorial/LangImpl08.html (ctrl-f for "IfExprAST::codegen")
        //Emit the condition
        llvm::Value *condValue = emitExpr(ifExpr.condition(), cc());

        //Prepare the BasicBlocks
        llvm::Function *currentFunc = cc().irBuilder().GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(cc().llvmContext(), "thenBlock");
        llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(cc().llvmContext(), "elseBlock");
        llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "endBlock");

        //Branch to then or else blocks, depending on condition.
        cc().irBuilder().CreateCondBr(condValue, thenBlock, elseBlock);

        //Emit the thenBlock
        currentFunc->getBasicBlockList().push_back(thenBlock);
        cc().irBuilder().SetInsertPoint(thenBlock);
        llvm::Value *thenValue = emitExpr(ifExpr.thenExpr(), cc());
        thenBlock = cc().irBuilder().GetInsertBlock();

        //Jump to the endBlock, skipping the elseBlock
        cc().irBuilder().CreateBr(endBlock);

        //Emit the elseBlock
        currentFunc->getBasicBlockList().push_back(elseBlock);
        cc().irBuilder().SetInsertPoint(elseBlock);

        llvm::Value *elseValue = nullptr;
        if (ifExpr.elseExpr()) {
            elseValue = emitExpr(*ifExpr.elseExpr(), cc());
        }
        cc().irBuilder().CreateBr(endBlock);
        elseBlock = cc().irBuilder().GetInsertBlock();

        //Emit the endBlock
        currentFunc->getBasicBlockList().push_back(endBlock);
        cc().irBuilder().SetInsertPoint(endBlock);
        if (ifExpr.exprType().primitiveType() != type::PrimitiveType::Void) {
            llvm::PHINode *phi = cc().irBuilder().CreatePHI(cc().typeMap().toLlvmType(ifExpr.exprType()),
                                                            ifExpr.elseExpr() ? 2 : 1, "iftmp");
            phi->addIncoming(thenValue, thenBlock);
            if (ifExpr.elseExpr()) {
                phi->addIncoming(elseValue, elseBlock);
            }
            setValue(phi);
        } else {
            setValue(nullptr);
        }
    }

    void visitingWhileExpr(ast::WhileExpr &whileExpr) override {

        //Prepare the BasicBlocks
        llvm::Function *currentFunc = cc().irBuilder().GetInsertBlock()->getParent();
        llvm::BasicBlock *conditionBlock = llvm::BasicBlock::Create(cc().llvmContext(), "whileCondition");
        llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(cc().llvmContext(), "whileBody");
        llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "whileEnd");

        cc().irBuilder().CreateBr(conditionBlock);
        currentFunc->getBasicBlockList().push_back(conditionBlock);
        cc().irBuilder().SetInsertPoint(conditionBlock);

        //Emit the condition
        llvm::Value *condValue = emitExpr(whileExpr.condition(), cc());

        //If the condition is true branch to the body block, otherwise branch to the end block
        cc().irBuilder().CreateCondBr(condValue, bodyBlock, endBlock);

        //Emit the body block
        currentFunc->getBasicBlockList().push_back(bodyBlock);
        cc().irBuilder().SetInsertPoint(bodyBlock);
        emitExpr(whileExpr.body(), cc());

        //Loop back to the condition
        cc().irBuilder().CreateBr(conditionBlock);

        //Emit the end block
        currentFunc->getBasicBlockList().push_back(endBlock);
        cc().irBuilder().SetInsertPoint(endBlock);
        setValue(nullptr);
    }

private:

    void visitExpressions(const gc_ref_vector<ast::ExprStmt> &expressions) {
        llvm::Value *lastValue = nullptr;
        for (auto expr : expressions) {
            llvm::Value *temp = emitExpr(expr.get(), cc());
            if (temp) {
                lastValue = temp;
            }
        }
        setValue(lastValue);

    }

    void visitingCompoundExpr(ast::CompoundExpr &compoundExpr) override {
        visitExpressions(compoundExpr.expressions());
    }

    void visitingExpressionList(ast::ExpressionList &expressionList) override {
        visitExpressions(expressionList.expressions());
    }

    void visitingAssertExprStmt(ast::AssertExprStmt &assert) override {
        //Emit the condition
        llvm::Value *condValue = emitExpr(assert.condition(), cc());

        //Prepare the BasicBlocks
        llvm::Function *currentFunc = cc().irBuilder().GetInsertBlock()->getParent();
        llvm::BasicBlock *passBlock = llvm::BasicBlock::Create(cc().llvmContext(), "assertPassBlock");
        llvm::BasicBlock *failBlock = llvm::BasicBlock::Create(cc().llvmContext(), "assertFailBlock");
        llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc().llvmContext(), "assertEndBlock");

        //Branch to then or else blocks, depending on condition.
        cc().irBuilder().CreateCondBr(condValue, passBlock, failBlock);

        //Emit the passBlock
        currentFunc->getBasicBlockList().push_back(passBlock);
        cc().irBuilder().SetInsertPoint(passBlock);
        cc().irBuilder().CreateCall(cc().assertPassFunc());

        //Jump to the endBlock, skipping the failBlock
        cc().irBuilder().CreateBr(endBlock);

        //Emit the failBlock
        currentFunc->getBasicBlockList().push_back(failBlock);
        cc().irBuilder().SetInsertPoint(failBlock);

        //Call the assert_failed function
        std::vector<llvm::Value *> arguments;
        source::SourceSpan span = assert.condition().sourceSpan();
        arguments.push_back(cc().getDeduplicatedStringConstant(span.name()));
        arguments.push_back(getLiteralUIntLLvmValue((unsigned int) span.start().line()));
        cc().irBuilder().CreateCall(cc().assertFailFunc(), arguments);

        //Jump to the endBlock,
        cc().irBuilder().CreateBr(endBlock);

        //Emit the endBlock
        currentFunc->getBasicBlockList().push_back(endBlock);
        cc().irBuilder().SetInsertPoint(endBlock);
        setValue(nullptr);
    }
};

llvm::Value *emitExpr(ast::ExprStmt &exprStmt, CompileContext &cc) {
    ExprStmtAstVisitor visitor{cc};
    exprStmt.accept(visitor);

    return visitor.llvmValue();
}

}}
