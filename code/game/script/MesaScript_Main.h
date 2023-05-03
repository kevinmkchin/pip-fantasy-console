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

table : L  RSQBRACKET



TABLEIDENTIFIER.VARIABLEIDENTIFIER



*/

// program : procedure_decl (program)*
// program : procedure_call
// TODO program : global variable assignment (program)*

// procedure_decl : SYMBOL LPAREN (SYMBOL)* RPAREN statement_list

// procedure_call : PROCEDURESYMBOL LPAREN (cond_or (COMMA cond_or)*)? RPAREN

// statement_list : LBRACE statement* RBRACE

// statement : IDENTIFIER ASSIGN cond_or
// statement : RETURN cond_or
// statement : PRINT cond_or
// statement : procedure_call
// statement : IF cond_or statement_list (ELSE statement_list)?
// todo : WHILE cond_or (statement sequence)

// cond_expr : expr ((< | > | <= | =>) expr)?

// cond_equal : cond_expr ((== | !=) cond_expr)?
// cond_equal : LPAREN cond_or RPAREN
// cond_equal : NOT LPAREN cond_or RPAREN
// cond_equal : NOT factor

// cond_and : cond_equal (AND cond_equal)?

// cond_or : cond_and (OR cond_and)?

// factor : NUMBER
// factor : LPAREN expr RPAREN
// factor : VARIABLEIDENTIFIER
// factor : TRUE|FALSE
// factor : procedure_call

// term : factor ((MUL | DIV) factor)*

// expr : term ((PLUS | MINUS) term)*

// Requires definition of keywords and operators (e.g. what does "return" do? it ends the current procedure etc.)

void RunMesaScriptInterpreterOnFile(const char* pathFromWorkingDir);

