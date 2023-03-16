#pragma once

// todo procedure : SYMBOL LPAREN (SYMBOL)* RPAREN statement_list

// statement_list : LBRACE statement* RBRACE

// statement : IDENTIFIER ASSIGN cond_or
// statement : RETURN cond_or
// statement : expr  // this is valid because a statement can be a procedure call
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
// todo factor : PROCEDURESYMBOL LPAREN (cond_or)* RPAREN

// term : factor ((MUL | DIV) factor)*

// expr : term ((PLUS | MINUS) term)*




void TestProc();

