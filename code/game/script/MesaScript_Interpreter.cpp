static TValue returnValue;
static bool returnValueSetFlag = false;
static bool returnRequestedFlag = false;

static void
InterpretStatementList(ASTNode* statements);

static TValue
InterpretProcedureCall(ASTProcedureCall* procedureCall);

static TValue
InterpretExpression(ASTNode* ast)
{
    switch(ast->GetType())
    {
        case ASTNodeType::BINOP: {
            auto v = static_cast<ASTBinOp*>(ast);
            TValue l = InterpretExpression(v->left);
            TValue r = InterpretExpression(v->right);
            // both integer, then integer
            // both float, then float
            // one int, one float, then float
            TValue::ValueType retValType = TValue::ValueType::Integer;
            switch (v->op)
            {
                case BinOp::Add: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.integerValue = l.integerValue + r.integerValue;
                        result.type = TValue::ValueType::Integer;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.realValue + r.integerValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.integerValue + r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.realValue = l.realValue + r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else
                    {
                        // todo error
                    }
                } break;
                case BinOp::Sub: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.integerValue = l.integerValue - r.integerValue;
                        result.type = TValue::ValueType::Integer;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.realValue - r.integerValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.integerValue - r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.realValue = l.realValue - r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else
                    {
                        // todo error
                    }
                } break;
                case BinOp::Mul: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.integerValue = l.integerValue * r.integerValue;
                        result.type = TValue::ValueType::Integer;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.realValue * r.integerValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.integerValue * r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.realValue = l.realValue * r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else
                    {
                        // todo error
                    }
                } break;
                case BinOp::Div: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.integerValue = l.integerValue / r.integerValue;
                        result.type = TValue::ValueType::Integer;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.realValue / r.integerValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        TValue result;
                        result.realValue = l.integerValue / r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        TValue result;
                        result.realValue = l.realValue / r.realValue;
                        result.type = TValue::ValueType::Real;
                        return result;
                    }
                    else
                    {
                        // todo error
                    }
                } break;
            }
        } break;
        case ASTNodeType::RELOP: {
            auto v = static_cast<ASTRelOp*>(ast);
            TValue l = InterpretExpression(v->left);
            TValue r = InterpretExpression(v->right);
            if (l.type == TValue::ValueType::Integer)
            {
                l.realValue = float(l.integerValue);
                l.type = TValue::ValueType::Real;
            }
            else if (l.type == TValue::ValueType::Boolean)
            {
                l.realValue = l.boolValue ? 1.f : 0.f;
                l.type = TValue::ValueType::Real;
            }
            if (r.type == TValue::ValueType::Integer)
            {
                r.realValue = float(r.integerValue);
                r.type = TValue::ValueType::Real;
            }
            else if (r.type == TValue::ValueType::Boolean)
            {
                r.realValue = l.boolValue ? 1.f : 0.f;
                r.type = TValue::ValueType::Real;
            }
            switch (v->op)
            {
                case RelOp::LT: {
                    TValue result;
                    result.boolValue = l.realValue < r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break; 
                case RelOp::LE: {
                    TValue result;
                    result.boolValue = l.realValue <= r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break;
                case RelOp::GE: {
                    TValue result;
                    result.boolValue = l.realValue >= r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break;
                case RelOp::EQ: {
                    TValue result;
                    result.boolValue = l.realValue == r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break;
                case RelOp::NEQ: {
                    TValue result;
                    result.boolValue = l.realValue != r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break;
                case RelOp::AND: {
                    TValue result;
                    result.boolValue = (l.realValue == 1.f && r.realValue == 1.f);
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break;
                case RelOp::OR: {
                    TValue result;
                    result.boolValue = (l.realValue == 1.f || r.realValue == 1.f);
                    result.type = TValue::ValueType::Boolean;
                    return result;
                } break;
            }
        } break;
        case ASTNodeType::LOGICALNOT: {
            auto v = static_cast<ASTLogicalNot*>(ast);
            auto result = InterpretExpression(v->boolExpr);
            result.boolValue = !result.boolValue;
            return result;
        } break;
        case ASTNodeType::VARIABLE: {
            auto v = static_cast<ASTVariable*>(ast);
            try {
                if (MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.KeyExists(v->id))
                {
                    return MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.AccessAtKey(v->id);
                }
                else if (MESASCRIPT_SCOPE.GLOBAL_TABLE.TableContainsKey(v->id))
                {
                    return MESASCRIPT_SCOPE.GLOBAL_TABLE.TableAccessElement(v->id);
                }
                else
                {
                    // error;
                }
            } catch (const std::exception& e) {
                // todo error
            }
        } break;
        case ASTNodeType::NUMBER: {
            auto v = static_cast<ASTNumberTerminal*>(ast);
            TValue result;
            result.integerValue = v->value;
            result.type = TValue::ValueType::Integer;
            return result;
        } break;
        case ASTNodeType::BOOLEAN: {
            auto v = static_cast<ASTBooleanTerminal*>(ast);
            TValue result;
            result.boolValue = v->value;
            result.type = TValue::ValueType::Boolean;
            return result;
        } break;
        case ASTNodeType::PROCEDURECALL: {
            auto v = static_cast<ASTProcedureCall*>(ast);
            return InterpretProcedureCall(v);
        } break;
    }
}

static void
InterpretStatement(ASTNode* statement)
{
    switch (statement->GetType())
    {
    case ASTNodeType::PROCEDURECALL: {
        auto v = static_cast<ASTProcedureCall*>(statement);
        InterpretProcedureCall(v);
    } break;
    case ASTNodeType::ASSIGN: {
        auto v = static_cast<ASTAssignment*>(statement);
        auto result = InterpretExpression(v->expr);
        ASSERT(v->id->GetType() == ASTNodeType::VARIABLE);
        std::string key = static_cast<ASTVariable*>(v->id)->id;
        if (MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.KeyExists(key))
        {
            MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.AccessAtKey(key) = result;
        }
        else if (MESASCRIPT_SCOPE.GLOBAL_TABLE.TableContainsKey(key))
        {
            MESASCRIPT_SCOPE.GLOBAL_TABLE.TableAccessElement(key) = result;
        }
        else
        {
            MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.EmplaceNewElement(key, result);
        }
    } break;
    case ASTNodeType::RETURN: {
        auto v = static_cast<ASTReturn*>(statement);
        TValue result = InterpretExpression(v->expr);
        returnValue = result;
        returnValueSetFlag = true;
        returnRequestedFlag = true;
    } break;
    case ASTNodeType::PRINT: {
        auto v = static_cast<ASTPrint*>(statement);
        TValue result = InterpretExpression(v->expr);
        if (result.type == TValue::ValueType::Integer)
        {
            printf("printed %lld\n", result.integerValue);
        }
        else if (result.type == TValue::ValueType::Boolean)
        {
            printf("printed %s\n", (result.boolValue ? "true" : "false"));
        }
        else if (result.type == TValue::ValueType::Real)
        {
            printf("printed %f\n", result.realValue);
        }
    } break;
    case ASTNodeType::BRANCH: {
        auto v = static_cast<ASTBranch*>(statement);
        auto condition = InterpretExpression(v->condition);
        ASSERT(condition.type == TValue::ValueType::Boolean);
        if (condition.boolValue)
            InterpretStatementList(v->if_body);
        else
            InterpretStatementList(v->else_body);
    } break;
    }
}

static void
InterpretStatementList(ASTNode* statements)
{
    ASSERT(statements->GetType() == ASTNodeType::STATEMENTLIST);
    auto v = static_cast<ASTStatementList*>(statements);
    for (const auto& s : v->statements)
    {
        InterpretStatement(s);
        if (returnRequestedFlag) return;
    }
}

NiceArray<TValue, 256> valueCache;

static TValue
InterpretProcedureCall(ASTProcedureCall* procedureCall)
{
    TValue procedureVariable;
    if (MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.KeyExists(procedureCall->id))
    {
        procedureVariable = MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.AccessAtKey(procedureCall->id);
    }
    else if (MESASCRIPT_SCOPE.GLOBAL_TABLE.TableContainsKey(procedureCall->id))
    {
        procedureVariable = MESASCRIPT_SCOPE.GLOBAL_TABLE.TableAccessElement(procedureCall->id);
    }
    auto procedureDefinition = PROCEDURES_DATABASE.At((unsigned int)procedureVariable.procedureId);


    MesaScript_Table functionScope;
    for (int i = 0; i < procedureDefinition.args.size(); ++i)
    {
        auto argn = procedureDefinition.args[i];
        auto argexpr = procedureCall->argsExpressions[i];
        TValue argv = InterpretExpression(argexpr);
        functionScope.TableCreateElement(argn, argv);
    }
    MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.PushScope(functionScope);

    for (int arg = 0; arg < procedureDefinition.args.size(); ++arg) // todo replace hacky way of assigning argument values
    {
        auto argn = procedureDefinition.args[arg];
        auto argv = procedureCall->argsExpressions[arg];
    }

    TValue retval;
    returnRequestedFlag = false;
    returnValueSetFlag = false;

    InterpretStatementList(procedureDefinition.body);

    if (returnValueSetFlag) retval = returnValue;

    returnRequestedFlag = false;
    returnValueSetFlag = false;

    MESASCRIPT_SCOPE.ACTIVE_SCRIPT_TABLE.PopScope();

    return retval;
}


//static i8 printAstIndent = 0;
//static void
//PrintAST(ASTNode* ast)
//{
//    printAstIndent += 3;
//    if(ast == nullptr)
//    {
//        printf("%s\n", (std::string(printAstIndent, ' ') + std::string("null")).c_str());
//        printAstIndent -= 3;
//        return;
//    }
//    switch(ast->GetType())
//    {
//        case ASTNodeType::ASSIGN:  {
//            auto v = static_cast<ASTAssignment*>(ast);
//            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("assign ")).c_str());
//            PrintAST(v->id);
//            PrintAST(v->expr);
//        } break;
//        case ASTNodeType::RETURN: {
//            auto v = static_cast<ASTReturn*>(ast);
//            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("return ")).c_str());
//            PrintAST(v->expr);
//        } break;
//        case ASTNodeType::BINOP: {
//            auto v = static_cast<ASTBinOp*>(ast);
//            const char* opName = nullptr;
//            switch (v->op)
//            {
//                case BinOp::Add: opName = "add"; break;
//                case BinOp::Sub: opName = "sub"; break;
//                case BinOp::Mul: opName = "mul"; break;
//                case BinOp::Div: opName = "div"; break;
//            }
//            printf("%s%s%s\n", (std::string(printAstIndent, ' ') + std::string("binop l ")).c_str(), opName, " r");
//            PrintAST(v->left);
//            PrintAST(v->right);
//        } break;
//        case ASTNodeType::RELOP: {
//            auto v = static_cast<ASTRelOp*>(ast);
//            const char* opName = nullptr;
//            switch (v->op)
//            {
//                case RelOp::LT: opName = "less than"; break;
//                case RelOp::GT: opName = "greater than"; break;
//                case RelOp::LE: opName = "less than or equal"; break;
//                case RelOp::GE: opName = "greater than or equal"; break;
//                case RelOp::EQ: opName = "equal"; break;
//                case RelOp::NEQ: opName = "not equal"; break;
//                case RelOp::AND: opName = "and"; break;
//                case RelOp::OR: opName = "or"; break;
//            }
//            printf("%s%s%s\n", (std::string(printAstIndent, ' ') + std::string("relop l ")).c_str(), opName, " r");
//            PrintAST(v->left);
//            PrintAST(v->right);
//        } break;
//        case ASTNodeType::LOGICALNOT: {
//            auto v = static_cast<ASTLogicalNot*>(ast);
//            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("not")).c_str());
//            PrintAST(v->boolExpr);
//        } break;
//        case ASTNodeType::BRANCH: {
//            auto v = static_cast<ASTBranch*>(ast);
//            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("branch (cond, if, else)")).c_str());
//            PrintAST(v->condition);
//            PrintAST(v->if_body);
//            PrintAST(v->else_body);
//        } break;
//        case ASTNodeType::VARIABLE: {
//            auto v = static_cast<ASTVariable*>(ast);
//            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("var ") + v->id).c_str());
//        } break;
//        case ASTNodeType::NUMBER: {
//            auto v = static_cast<ASTNumberTerminal*>(ast);
//            printf("%s%d\n", (std::string(printAstIndent, ' ') + std::string("num ")).c_str(), v->value);
//        } break;
//        case ASTNodeType::BOOLEAN: {
//            auto v = static_cast<ASTBooleanTerminal*>(ast);
//            printf("%s%s\n", (std::string(printAstIndent, ' ') + std::string("bool ")).c_str(), (v->value ? "true" : "false"));
//        } break;
//    }
//    printAstIndent -= 3;
//}

