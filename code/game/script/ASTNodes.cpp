enum class ASTNodeType
{
    ASSIGN,
    VARIABLE,
    RETURN,
    WHILE,
    NUMBER,
    BOOLEAN,
    BINOP,
    RELOP
};


class ASTNode
{
public:
    ASTNode(ASTNodeType type);
    inline ASTNodeType GetType() { return nodeType; }
private:
    ASTNodeType nodeType;
};

class ASTAssignment : public ASTNode
{
public:
    ASTAssignment(ASTNode* id, ASTNode* expr);
public:
    ASTNode* id;
    ASTNode* expr;
};

class ASTVariable : public ASTNode
{
public:
    ASTVariable(const std::string& id);
public:
    std::string id;
};

class ASTReturn : public ASTNode
{
public:
    ASTReturn(ASTNode* expr);
public:
    ASTNode* expr;
};

class ASTWhile : public ASTNode
{
public:
    ASTWhile();
public:
    ASTNode* condition;
    ASTNode* body;
};

class ASTNumberTerminal : public ASTNode
{
public:
    ASTNumberTerminal(i32 num);
public:
    i32 value;
};

class ASTBooleanTerminal : public ASTNode
{
public:
    ASTBooleanTerminal(bool v);
public:
    bool value;
};

enum class BinOp
{
    Add,
    Sub,
    Mul,
    Div
};

class ASTBinOp : public ASTNode
{
public:
    ASTBinOp(BinOp op, ASTNode* left, ASTNode* right);
public:
    BinOp op;
    ASTNode* left;
    ASTNode* right;
};

enum class RelOp
{
    LT,
    GT,
    LE,
    GE,
    EQ,
    NEQ,
    AND, // and logical AND OR
    OR
};

class ASTRelOp : public ASTNode
{
public:
    ASTRelOp(RelOp op, ASTNode* left, ASTNode* right);
public:
    RelOp op;
    ASTNode* left;
    ASTNode* right;
};


ASTNode::ASTNode(ASTNodeType type)
    : nodeType(type)
{}


ASTAssignment::ASTAssignment(ASTNode* id, ASTNode* expr)
    : ASTNode(ASTNodeType::ASSIGN)
    , id(id)
    , expr(expr)
{}

ASTVariable::ASTVariable(const std::string& id)
    : ASTNode(ASTNodeType::VARIABLE)
    , id(id)
{}

ASTReturn::ASTReturn(ASTNode* expr)
    : ASTNode(ASTNodeType::RETURN)
    , expr(expr)
{}

ASTNumberTerminal::ASTNumberTerminal(i32 num)
    : ASTNode(ASTNodeType::NUMBER)
    , value(num)
{}

ASTBooleanTerminal::ASTBooleanTerminal(bool v)
    : ASTNode(ASTNodeType::BOOLEAN)
    , value(v)
{}

ASTBinOp::ASTBinOp(BinOp op, ASTNode* left, ASTNode* right)
    : ASTNode(ASTNodeType::BINOP)
    , op(op)
    , left(left)
    , right(right)
{}

ASTRelOp::ASTRelOp(RelOp op, ASTNode* left, ASTNode* right)
    : ASTNode(ASTNodeType::RELOP)
    , op(op)
    , left(left)
    , right(right)
{}



