
static MemoryLinearBuffer astBuffer;

class Parser
{
public:
    Parser(std::vector<Token> _tokens);

    std::vector<ASTNode*> parse();

private:
    void error();

    void eat(TokenType tpe);

    ASTNode* factor();
    ASTNode* term();
    ASTNode* expr();

    ASTNode* cond_expr();
    ASTNode* cond_equal();
    ASTNode* cond_and();
    ASTNode* cond_or();

    ASTNode* statement();


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

std::vector<ASTNode*> Parser::parse()
{
    std::vector<ASTNode*> statementSequence;

    while(currentTokenIndex < tokens.size() - 1)
    {
        statementSequence.push_back(statement());
        //eat(TokenType::EndOfLine);
    }

    return statementSequence;
}

ASTNode* Parser::statement()
{
    // statement : IDENTIFIER ASSIGN cond_or
    // statement : RETURN cond_or
    // statement : expr  // this is valid because a statement can be a function call
    // statement : IF cond_or statement (ELSE statement)? // todo change statement to statement sequence
    // todo : WHILE cond_or (statement sequence)

    if (currentToken.type == TokenType::Identifier)
    {
        auto t = currentToken;
        eat(TokenType::Identifier);
        eat(TokenType::AssignmentOperator);
        auto varNode =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTVariable), 32))
                        ASTVariable(t.text);
        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTAssignment), 8))
                        ASTAssignment(varNode, cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::Return)
    {
        eat(TokenType::Return);
        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTReturn), 8))
                        ASTReturn(cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::If)
    {
        eat(TokenType::If);
        ASTNode* condition = cond_or();
        ASTNode* ifCase = statement(); // todo sequence of statements
        ASTNode* elseCase = nullptr;
        if (currentToken.type == TokenType::Else)
        {
            eat(TokenType::Else);
            elseCase = statement(); // todo sequence of statements
        }

        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTBranch), 8))
                        ASTBranch(condition, ifCase, elseCase);
        return node;
    }
    else
    {
        return cond_or();
    }
}

ASTNode* Parser::cond_expr()
{
    // cond_expr : expr ((< | > | <= | =>) expr)?

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
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
                        ASTRelOp(op, node, expr());
    }

    return node;
}

ASTNode* Parser::cond_equal()
{
    // cond_equal : cond_expr ((== | !=) cond_expr)?
    // cond_equal : LPAREN cond_or RPAREN
    // cond_equal : NOT LPAREN cond_or RPAREN
    // cond_equal : NOT factor

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
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTLogicalNot), 8))
                            ASTLogicalNot(cond_or());
            eat(TokenType::RParen);
        }
        else
        {
            node =
                    new (MemoryLinearAllocate(&astBuffer, sizeof(ASTLogicalNot), 8))
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
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
                        ASTRelOp(op, node, cond_expr());
    }

    return node;
}

ASTNode* Parser::cond_and()
{
    // cond_and : cond_equal (AND cond_equal)?

    auto node = cond_equal();

    if(currentToken.type == TokenType::LogicalAnd)
    {
        eat(TokenType::LogicalAnd);

        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
                        ASTRelOp(RelOp::AND, node, cond_equal());
    }

    return node;
}

ASTNode* Parser::cond_or()
{
    // cond_or : cond_and (OR cond_and)?

    auto node = cond_and();

    if(currentToken.type == TokenType::LogicalOr)
    {
        eat(TokenType::LogicalOr);

        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
                        ASTRelOp(RelOp::OR, node, cond_and());
    }

    return node;
}

ASTNode* Parser::factor()
{
    // factor : NUMBER
    // factor : LPAREN expr RPAREN
    // factor : VARIABLEIDENTIFIER
    // factor : TRUE|FALSE
    // todo : function calls

    auto t = currentToken;
    ASTNode* node = nullptr;

    if (t.type == TokenType::NumberLiteral)
    {
        eat(TokenType::NumberLiteral);
        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTNumberTerminal), 8))
                        ASTNumberTerminal(atoi(t.text.c_str()));
        return node;
    }
    else if (t.type == TokenType::True)
    {
        eat(TokenType::True);
        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBooleanTerminal), 8))
                        ASTBooleanTerminal(true);
    }
    else if (t.type == TokenType::False)
    {
        eat(TokenType::False);
        node =
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBooleanTerminal), 8))
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
        eat(TokenType::Identifier);
        node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTVariable), 32))
                        ASTVariable(t.text);
    }
    else
    {
        error();
    }

    return node;
}

ASTNode* Parser::term()
{
    // term : factor ((MUL | DIV) factor)*

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
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), 8))
                        ASTBinOp(op, node, factor());
    }

    return node;
}

ASTNode* Parser::expr()
{
    // expr : term ((PLUS | MINUS) term)*

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
                new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), 8))
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
