
static MemoryLinearBuffer astBuffer;

class Parser
{
public:
    Parser(std::vector<Token> _tokens);

    void parse();

private:
    void error();

    void eat(TokenType tpe);

    ASTNode* procedure_call();
    ASTNode* factor();
    ASTNode* term();
    ASTNode* expr();

    ASTNode* cond_expr();
    ASTNode* cond_equal();
    ASTNode* cond_and();
    ASTNode* cond_or();

    ASTNode* statement();
    ASTStatementList* statement_list();
    PID procedure();


private:
    std::vector<Token> tokens;
    Token currentToken;
    size_t currentTokenIndex;
};

Parser::Parser(std::vector<Token> _tokens)
        : tokens(_tokens)
        , currentToken(tokens.front())
        , currentTokenIndex(0)
{}

void Parser::parse()
{
    while (currentToken.type != TokenType::EndOfFile)
    {
        procedure();
    }
}

PID Parser::procedure()
{
    ASSERT(currentToken.type == TokenType::Identifier);

    auto procedureNameToken = currentToken;

    eat(TokenType::Identifier);
    eat(TokenType::LParen);

    PROCEDURES_DATABASE.PushBack(ProcedureDefinition());
    const PID createdProcedureId = PROCEDURES_DATABASE.count - 1;
    auto& argsVector = PROCEDURES_DATABASE.At((unsigned int)createdProcedureId).args;

    while(currentToken.type != TokenType::RParen)
    {
        argsVector.push_back(currentToken.text);
        eat(TokenType::Identifier);
    }
    eat(TokenType::RParen);

    PROCEDURES_DATABASE.At((unsigned int)createdProcedureId).body = statement_list();

    TValue functionVariable;
    functionVariable.procedureId = createdProcedureId;
    functionVariable.type = TValue::ValueType::Function;

    if (MESASCRIPT_SCOPE.GLOBAL_TABLE.TableContainsKey(procedureNameToken.text))
    {
        MESASCRIPT_SCOPE.GLOBAL_TABLE.TableAccessElement(procedureNameToken.text) = functionVariable;
    }
    else
    {
        MESASCRIPT_SCOPE.GLOBAL_TABLE.TableCreateElement(procedureNameToken.text, functionVariable);
    }

    return createdProcedureId;
}

ASTNode* Parser::procedure_call()
{
    auto procSymbol = currentToken;
    eat(TokenType::Identifier);
    eat(TokenType::LParen);

    auto node =
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTProcedureCall), alignof(ASTProcedureCall)))
                    ASTProcedureCall(procSymbol.text);

    while(currentToken.type != TokenType::RParen)
    {
        node->argsExpressions.push_back(cond_expr());
        if (currentToken.type != TokenType::RParen) eat(TokenType::Comma);
    }

    eat(TokenType::RParen);

    return node;
}

ASTStatementList* Parser::statement_list()
{
    auto statement_list = new (MemoryLinearAllocate(&astBuffer, sizeof(ASTStatementList), alignof(ASTStatementList)))
            ASTStatementList();
    eat(TokenType::LBrace);
    while(currentToken.type != TokenType::RBrace)
    {
        statement_list->statements.push_back(statement());
    }
    eat(TokenType::RBrace);
    return statement_list;
}

ASTNode* Parser::statement()
{
    if (currentToken.type == TokenType::Identifier)
    {
        auto nextToken = tokens.at(currentTokenIndex + 1);
        if (nextToken.type == TokenType::LParen)
        {
            return procedure_call();
        }
        else
        {
            auto t = currentToken;
            eat(TokenType::Identifier);
            eat(TokenType::AssignmentOperator);
            auto varNode =
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTVariable), alignof(ASTVariable)))
                            ASTVariable(t.text);
            auto node =
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTAssignment), alignof(ASTAssignment)))
                            ASTAssignment(varNode, cond_or());
            return node;
        }
    }
    else if (currentToken.type == TokenType::Return)
    {
        eat(TokenType::Return);
        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTReturn), alignof(ASTReturn)))
                        ASTReturn(cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::Print)
    {
        eat(TokenType::Print);
        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTPrint), alignof(ASTPrint)))
                        ASTPrint(cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::If)
    {
        eat(TokenType::If);
        ASTNode* condition = cond_or();
        ASTNode* ifCase = statement_list();
        ASTNode* elseCase = nullptr;
        if (currentToken.type == TokenType::Else)
        {
            eat(TokenType::Else);
            elseCase = statement_list();
        }

        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTBranch), alignof(ASTBranch)))
                        ASTBranch(condition, ifCase, elseCase);
        return node;
    }
    else
    {
        error();
    }
}

ASTNode* Parser::cond_expr()
{
    auto node = expr();

    if(ISANYOF4(currentToken.type, TokenType::LessThan, TokenType::LessThanOrEqual,
                TokenType::GreaterThan, TokenType::GreaterThanOrEqual))
    {
        auto t = currentToken;
        RelOp op = RelOp::LT;

        if (t.type == TokenType::LessThan)
        {
            eat(TokenType::LessThan);
            op = RelOp::LT;
        }
        else if (t.type == TokenType::LessThanOrEqual)
        {
            eat(TokenType::LessThanOrEqual);
            op = RelOp::LE;
        }
        else if (t.type == TokenType::GreaterThan)
        {
            eat(TokenType::GreaterThan);
            op = RelOp::GT;
        }
        else if (t.type == TokenType::GreaterThanOrEqual)
        {
            eat(TokenType::GreaterThanOrEqual);
            op = RelOp::GE;
        }

        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(op, node, expr());
    }

    return node;
}

ASTNode* Parser::cond_equal()
{
    ASTNode* node = nullptr;

    if (currentToken.type == TokenType::LParen)
    {
        eat(TokenType::LParen);
        node = cond_or();
        eat(TokenType::RParen);
        return node;
    }
    else if (currentToken.type == TokenType::LogicalNegation)
    {
        eat(TokenType::LogicalNegation);
        if(currentToken.type == TokenType::LParen)
        {
            eat(TokenType::LParen);
            node =
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTLogicalNot), alignof(ASTLogicalNot)))
                            ASTLogicalNot(cond_or());
            eat(TokenType::RParen);
        }
        else
        {
            node =
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTLogicalNot), alignof(ASTLogicalNot)))
                            ASTLogicalNot(factor());
        }
        return node;
    }

    node = cond_expr();

    if(ISANYOF2(currentToken.type, TokenType::Equal, TokenType::NotEqual))
    {
        auto t = currentToken;
        RelOp op = RelOp::EQ;

        if (t.type == TokenType::Equal)
        {
            eat(TokenType::Equal);
            op = RelOp::EQ;
        }
        else if (t.type == TokenType::NotEqual)
        {
            eat(TokenType::NotEqual);
            op = RelOp::NEQ;
        }

        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(op, node, cond_expr());
    }

    return node;
}

ASTNode* Parser::cond_and()
{
    auto node = cond_equal();

    if(currentToken.type == TokenType::LogicalAnd)
    {
        eat(TokenType::LogicalAnd);

        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(RelOp::AND, node, cond_equal());
    }

    return node;
}

ASTNode* Parser::cond_or()
{
    auto node = cond_and();

    if(currentToken.type == TokenType::LogicalOr)
    {
        eat(TokenType::LogicalOr);

        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(RelOp::OR, node, cond_and());
    }

    return node;
}

ASTNode* Parser::factor()
{
    auto t = currentToken;
    ASTNode* node = nullptr;

    if (t.type == TokenType::NumberLiteral)
    {
        eat(TokenType::NumberLiteral);
        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTNumberTerminal), alignof(ASTNumberTerminal)))
                        ASTNumberTerminal(atoi(t.text.c_str()));
        return node;
    }
    else if (t.type == TokenType::True)
    {
        eat(TokenType::True);
        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBooleanTerminal), alignof(ASTBooleanTerminal)))
                        ASTBooleanTerminal(true);
    }
    else if (t.type == TokenType::False)
    {
        eat(TokenType::False);
        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBooleanTerminal), alignof(ASTBooleanTerminal)))
                        ASTBooleanTerminal(false);
    }
    else if (t.type == TokenType::LParen)
    {
        eat(TokenType::LParen);
        node = expr();
        eat(TokenType::RParen);
    }
    else if (t.type == TokenType::Identifier)
    {
        auto nextToken = tokens.at(currentTokenIndex + 1);
        if (nextToken.type == TokenType::LParen)
        {
            node = procedure_call();
        }
        else
        {
            eat(TokenType::Identifier);
            node =
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTVariable), alignof(ASTVariable)))
                            ASTVariable(t.text);
        }
    }
    else
    {
        error();
    }

    return node;
}

ASTNode* Parser::term()
{
    auto node = factor();

    while(ISANYOF2(currentToken.type, TokenType::MulOperator, TokenType::DivOperator))
    {
        auto t = currentToken;
        BinOp op = BinOp::Mul;

        if (t.type == TokenType::MulOperator)
        {
            eat(TokenType::MulOperator);
            op = BinOp::Mul;
        }
        else if (t.type == TokenType::DivOperator)
        {
            eat(TokenType::DivOperator);
            op = BinOp::Div;
        }

        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), alignof(ASTBinOp)))
                        ASTBinOp(op, node, factor());
    }

    return node;
}

ASTNode* Parser::expr()
{
    auto node = term();

    while(ISANYOF2(currentToken.type, TokenType::AddOperator, TokenType::SubOperator))
    {
        auto t = currentToken;
        BinOp op = BinOp::Add;

        if (t.type == TokenType::AddOperator)
        {
            eat(TokenType::AddOperator);
            op = BinOp::Add;
        }
        else if (t.type == TokenType::SubOperator)
        {
            eat(TokenType::SubOperator);
            op = BinOp::Sub;
        }

        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), alignof(ASTBinOp)))
                        ASTBinOp(op, node, term());
    }

    return node;
}

void Parser::error()
{
    printf("oof\n");
    ASSERT(0);
}

void Parser::eat(TokenType tpe)
{
    // compare the current token type with the passed token
    // type and if they match then "eat" the current token
    // and assign the next token to currentToken, otherwise
    // error.

    if(currentToken.type == tpe)
    {
        currentToken = tokens[++currentTokenIndex];
    }
    else
    {
        error();
    }
}
