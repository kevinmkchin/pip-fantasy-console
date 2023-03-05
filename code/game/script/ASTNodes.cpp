
class ASTNode
{

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
    BinOp op; // add, sub, mul, div
    ASTNode* left;
    ASTNode* right;
};



ASTAssignment::ASTAssignment(ASTNode* id, ASTNode* expr)
    : id(id)
    , expr(expr)
{}

ASTVariable::ASTVariable(const std::string& id)
    : id(id)
{}

ASTReturn::ASTReturn(ASTNode* expr)
    : expr(expr)
{}

ASTNumberTerminal::ASTNumberTerminal(i32 num)
    : value(num)
{}

ASTBinOp::ASTBinOp(BinOp op, ASTNode* left, ASTNode* right)
    : op(op)
    , left(left)
    , right(right)
{}

