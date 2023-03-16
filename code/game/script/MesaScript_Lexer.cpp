
bool IsValueType(TokenType type)
{
    return
            type == TokenType::NumberLiteral ||
            type == TokenType::Identifier ||
            type == TokenType::RParen;
}

// char -> bool
// return true if char is one of [0, 9]
bool IsDigit(char c)
{
    return ('0' <= c && c <= '9');
}

bool IsCharacter(char c)
{
    return
            ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            ('_' == c);
}

bool IsWhitespace(char c)
{
    return c == ' ' /* or some other shit*/;
}

// string -> list of Token
std::vector<Token> Lexer(const std::string& code)
{
    std::vector<Token> retval;
    u32 currentIndex = 0;

    bool flag_NegativeNumberAhead = false;
    while(currentIndex < code.length())
    {
        u32 tokenStartIndex = currentIndex;
        char lookAhead = code.at(currentIndex);
        if(IsWhitespace(lookAhead))
        {
            ++currentIndex;
        }
        else if(lookAhead == '-')
        {
            ++currentIndex;
            if (!IsValueType(retval.back().type) && currentIndex < code.length() && IsDigit(code.at(currentIndex)))
            {
                flag_NegativeNumberAhead = true;
            }
            else
            {
                retval.push_back({ TokenType::SubOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
        }
        else if(lookAhead == '<' || lookAhead == '>' || lookAhead == '!' || lookAhead == '=')
        {
            ++currentIndex;
            if (currentIndex < code.length() && code.at(currentIndex) == '=') // check if next char is =
            {
                ++currentIndex;
                if (lookAhead == '<')
                {
                    retval.push_back({ TokenType::LessThanOrEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
                else if (lookAhead == '>')
                {
                    retval.push_back({ TokenType::GreaterThanOrEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
                else if (lookAhead == '!')
                {
                    retval.push_back({ TokenType::NotEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
                else if (lookAhead == '=')
                {
                    retval.push_back({ TokenType::Equal, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
            }
            else if (lookAhead == '<')
            {
                retval.push_back({ TokenType::LessThan, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (lookAhead == '>')
            {
                retval.push_back({ TokenType::GreaterThan, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (lookAhead == '=')
            {
                retval.push_back({ TokenType::AssignmentOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (lookAhead == '!')
            {
                retval.push_back({TokenType::LogicalNegation, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
        }
        else if(IsDigit(lookAhead))
        {
            if (flag_NegativeNumberAhead)
            {
                tokenStartIndex -= 1;
                flag_NegativeNumberAhead = false;
            }
            while (currentIndex < code.length() && IsDigit(code.at(currentIndex)))
            {
                ++currentIndex;
            }
            retval.push_back({ TokenType::NumberLiteral, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
        }
        else if(IsCharacter(lookAhead))
        {
            NiceArray<char, 65> wordCharsBuffer; // then collect string of character to form word
            wordCharsBuffer.ResetToZero();
            while(currentIndex < code.length() && (IsCharacter(code.at(currentIndex)) || IsDigit(code.at(currentIndex))))
            {
                wordCharsBuffer.PushBack(code.at(currentIndex));
                ++currentIndex;
            } // at this point, we have a fully formed word

            auto word = std::string(wordCharsBuffer.data);
            if (word == "return")
            {
                retval.push_back({ TokenType::Return, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "print")
            {
                retval.push_back({ TokenType::Print, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "true")
            {
                retval.push_back({ TokenType::True, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "false")
            {
                retval.push_back({ TokenType::False, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "and")
            {
                retval.push_back({TokenType::LogicalAnd, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "or")
            {
                retval.push_back({TokenType::LogicalOr, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "if")
            {
                retval.push_back({ TokenType::If, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "else")
            {
                retval.push_back({ TokenType::Else, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else // otherwise, word is function call or identifier
            {
                retval.push_back({ TokenType::Identifier, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
        }
        else
        {
            ++currentIndex;
            TokenType tokenType;
            switch(lookAhead)
            {
                case '+': { tokenType = TokenType::AddOperator; } break;
                case '*': { tokenType = TokenType::MulOperator; } break;
                case '/': { tokenType = TokenType::DivOperator; } break;
                case '(': { tokenType = TokenType::LParen; } break;
                case ')': { tokenType = TokenType::RParen; } break;
                case '{': { tokenType = TokenType::LBrace; } break;
                case '}': { tokenType = TokenType::RBrace; } break;
                case ',': { tokenType = TokenType::Comma; } break;
                case '\n': { continue; /*tokenType = TokenType::EndOfLine;*/ } break;
                default:{
                    printf("error: unrecognized character in Lexer");
                    continue;
                }
            }
            retval.push_back({ tokenType, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
        }
    }
    retval.push_back({TokenType::EndOfFile, "<EOF>", (u32)code.length()});
    return retval;
}
