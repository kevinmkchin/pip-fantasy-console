#pragma once

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



// program : procedure_decl (program)*
// program : procedure_call
// todo program : global variable assignment (program)*

// procedure_decl : SYMBOL LPAREN (SYMBOL)* RPAREN statement_list

// procedure_call : PROCEDURESYMBOL LPAREN (cond_or (COMMA cond_or)*)? RPAREN

// statement_list : LBRACE statement* RBRACE

// statement : IDENTIFIER (LSQBRACK expr RSQBRACK)? ASSIGN table_or_cond_or
// statement : RETURN cond_or
// statement : PRINT cond_or
// statement : procedure_call
// statement : IF cond_or statement_list /*todo (ELIF statement_list)* */ (ELSE statement_list)?
// todo : WHILE cond_or (statement sequence)

// cond_expr : expr ((< | > | <= | =>) expr)?

// cond_equal : cond_expr ((== | !=) cond_expr)?
// cond_equal : LPAREN cond_or RPAREN
// cond_equal : NOT LPAREN cond_or RPAREN
// cond_equal : NOT factor

// cond_and : cond_equal (AND cond_equal)?

// cond_or : cond_and (OR cond_and)?

// table_or_cond_or : (LBRACE  RBRACE | cond_or)

// factor : NUMBER
// factor : LPAREN expr RPAREN
// factor : IDENTIFIER (LSQBRACK expr RSQBRACK)?
// factor : TRUE|FALSE
// factor : procedure_call

// term : factor ((MUL | DIV) factor)*

// expr : term ((PLUS | MINUS) term)*

// Requires definition of keywords and operators (e.g. what does "return" do? it ends the current procedure etc.)

void RunMesaScriptInterpreterOnFile(const char* pathFromWorkingDir);

