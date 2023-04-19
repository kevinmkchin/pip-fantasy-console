#pragma once

/** TODO

    - tables (require gc)
    - strings (require gc)
    - garbage collecting

    - floats

    - elif
    - while
    - for
    
 Way the fuck down the line:
    - use custom assert for mesascript
    - replace all std::vectors with custom data struct
    - REFACTOR
*/

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

