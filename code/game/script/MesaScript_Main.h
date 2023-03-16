#pragma once

// todo program : procedure (program)*
//      program : global variable assignment (program)*
//      program : procedure_call

// todo procedure : SYMBOL LPAREN (SYMBOL)* RPAREN statement_list

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

// procedure_call : PROCEDURESYMBOL LPAREN RPAREN
// todo procedure_call : PROCEDURESYMBOL LPAREN (cond_or (COMMA cond_or)*)? RPAREN
// before procedure call, if arguments will overwrite existing variables, then cache those variables in a stack, then
// bring them back after procedure call.

// term : factor ((MUL | DIV) factor)*

// expr : term ((PLUS | MINUS) term)*




void TestProc();

