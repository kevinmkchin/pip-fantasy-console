#pragma once

#include "MesaCommon.h"
#include "MesaUtility.h"

#include <unordered_map>
#include <vector>
#include <sstream>

/*
 * stuff can be one of :
 * - keyword
 *   - if, ret, else, proc, end, int, float, bool, struct, string, while, for, array, list
 * - identifier
 *   - x, y, fib, Start, Update
 * - terminal
 *   - 2, 514, 3.14, "hello world", true, false
 * - operator
 *   - +, -, *, /, <, >, is, <=, >=, isnt, and, or
 *   - = assignment operator
 *
 * */

// LANGUAGE FEATURES
// https://en.wikipedia.org/wiki/Reference_counting
// Tables
// Strings


// Access := no ref count increase. just looking at the value.



// -- FORMAL GRAMMAR --

/*

VALUE 0

a = VALUE
b = a

a = OTHERVALUE
b = OTHERVALUE


fn (a)
{
  
}

var = "some string " + 32

t = {  } ~then t is the first reference of the table, if we assign another value to t and t is still the only reference, then we deallocate the table
t = {x, 32}.x
t@0
t@1
t@(1) + 2
list@1
list@i = fef
t[2]
t


something : IDENTIFIER_table PERIOD string_without_quotations
TABLEIDENTIFIER["integer"] == TABLEIDENTIFIER[integer]
TABLEIDENTIFIER["VARIABLEIDENTIFIER"] == TABLEIDENTIFIER.VARIABLEIDENTIFIER


for (i is 0 to 100)
{
    
}
for (declare variables; exit condition; do at end of every loop) { loop }
for (i = 0; i < 100; i = i + 1)
{
    
}
i = 0
while (i < 100)
{
    i = i + 1
}


a = "aefjioawjlfkaw"

a[0]


table : LBRACE ()* RBRACE
table : IDENTIFIER_table
statement : variable ASSIGN (table | cond_or)
variable : IDENTIFIER_table LSQBRACK (expr | string) RSQBRACK
variable : IDENTIFIER_table PERIOD string_without_quotations
variable : IDENTIFIER_numbersandshit

IDENTIFIER["integer"] == IDENTIFIER[integer]
IDENTIFIER["IDENTIFIER"] == IDENTIFIER.IDENTIFIER

*/

// rvalue nodes can go on the right hand side of an assignment expression



// program : (procedure_decl | statement) (program)*

// procedure_decl : SYMBOL LPAREN (SYMBOL)* RPAREN statement_list

// procedure_call : PROCEDURESYMBOL LPAREN (cond_or (COMMA cond_or)*)? RPAREN

// statement_list : LBRACE statement* RBRACE

// declaration : IDENTIFIER (LSQBRACK expr RSQBRACK)? ASSIGN table_or_list_or_cond_or
// declaration : statement

// statement : 
// statement : RETURN cond_or
// statement : PRINT cond_or
// statement : procedure_call
// statement : IF cond_or statement_list /*todo (ELIF statement_list)* */ (ELSE statement_list)?
// statement : WHILE cond_or statement_list
// todo : FOR LPAREN statement? cond_or? statement? RPAREN statement_list

// cond_expr : expr ((< | > | <= | =>) expr)?

// cond_equal : cond_expr ((== | !=) cond_expr)?
// cond_equal : LPAREN cond_or RPAREN
// cond_equal : NOT LPAREN cond_or RPAREN
// cond_equal : NOT factor

// cond_and : cond_equal (AND cond_equal)?

// cond_or : cond_and (OR cond_and)?

// table_or_list_or_cond_or : (LBRACE (todo)* RBRACE | LSQBRACK (table_or_list_or_cond_or)* RSQBRACK | cond_or)

// factor : NUMBER
// factor : LPAREN expr RPAREN
// factor : IDENTIFIER (LSQBRACK expr RSQBRACK)?
// factor : TRUE|FALSE
// factor : procedure_call

// term : factor ((MUL | DIV) factor)*

// expr : term ((PLUS | MINUS) term)*

// Requires definition of keywords and operators (e.g. what does "return" do? it ends the current procedure etc.)
// Requires semantics definitions

typedef size_t PID;
#define PID_MAX 256

struct MesaTValue
{
    enum class ValueType
    {
        Invalid, // pointless
        Integer, // just use real
        Real,
        Boolean,
        Function,
        GCObject
    };

    union
    {
        i64 integerValue;
        double realValue;
        bool boolValue;
        PID procedureId;
        i64 GCReferenceObject;
    };

    ValueType type = ValueType::Invalid;
};

struct MesaGCObject
{
    enum class GCObjectType
    {
        Invalid,
        Table,
        List,
        String
    };

public:
    i32 refCount = 0;
    i64 selfId = 0;
private:
    GCObjectType type = GCObjectType::Invalid;
public:
    MesaGCObject(GCObjectType in_type) { type = in_type; }
    inline GCObjectType GetType() { return type; }
};

struct MesaScript_String
{
    MesaGCObject base;

    std::string text;

    MesaScript_String()
        : base(MesaGCObject::GCObjectType::String)
    {}
};

struct MesaScript_List
{
    MesaGCObject base;

    std::vector<MesaTValue> list;

    MesaScript_List()
        : base(MesaGCObject::GCObjectType::List)
    {}

    /// Simply returns the value at index. Does not increment reference count.
    MesaTValue AccessListEntry(const i64 index)
    {
        return list.at(index);
    }

    /// Append value to end of list. Increments reference count.
    void Append(const MesaTValue value);

    /// Replace the value at the given index.
    void ReplaceListEntryAtIndex(const i64 index, const MesaTValue value);

    /// Should only be called before deletion.
    void DecrementReferenceCountOfEveryListEntry();

    //void Insert();
    //void ArrayFront();
    //void ArrayBack();
    // Length
    // Contains
};

struct MesaScript_Table
{
    MesaGCObject base;

    std::unordered_map<std::string, MesaTValue> table;

    MesaScript_Table()
        : base(MesaGCObject::GCObjectType::Table)
    {}

    bool Contains(const std::string& key);

    /// Simply returns the value at key. Does not increment reference count.
    MesaTValue AccessMapEntry(const std::string& key);

    /// Create a new key value pair entry. Increments reference count.
    void CreateNewMapEntry(const std::string& key, const MesaTValue value);

    /// Assign new value to existing entry perform proper reference counting.
    void ReplaceMapEntryAtKey(const std::string& key, const MesaTValue value);

    /// Should only be called before deletion.
    void DecrementReferenceCountOfEveryMapEntry();
};

/// Simply returns the MesaScript_Table associated with the given GCObject id.
MesaScript_Table* AccessMesaScriptTable(i64 gcObjectId);
void IncrementReferenceGCObject(i64 gcObjectId);
void ReleaseReferenceGCObject(i64 gcObjectId);
i64 RequestNewGCObject(MesaGCObject::GCObjectType gcObjectType);



MesaScript_Table* EmplaceMapInGlobalScope(const std::string& id);
MesaScript_Table* AccessMapInGlobalScope(const std::string& id);

void CompileAndRunMesaScriptCode(const std::string& script, MesaScript_Table *scriptScope);
void SetEnvironmentScope(MesaScript_Table *scriptScope);
void ClearEnvironmentScope();

void CallFunction_Parameterless(const char* functionIdentifier);
void CallFunction_OneParam(const char* functionIdentifier, MesaTValue arg0);

void InitializeLanguageCompilerAndRuntime();

void RunProfilerOnScript(const std::string& script, std::ostringstream& profilerOutput);
void SimplyRunScript(const std::string& script);

void SimplyRunScriptFromFile(const std::string& pathFromWorkingDir);

// WRITING CPP BOUND MESASCRIPT FUNCTIONS INTERFACE

void pipl_bind_cpp_fn(const char *fn_name, int argc, void *fn_ptr);
// pipl_cpp_assert_arg_is_x

