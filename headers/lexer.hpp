#ifndef __LEXER_H
#define __LEXER_H
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <vector>
#include <algorithm>

enum Tokentype {
    TOKEN_ID,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_QUOTE,
    TOKEN_SINGLEQUOTE,
    TOKEN_INT,
    TOKEN_EQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_PLUSOP,
    TOKEN_MINUSOP,
    TOKEN_MULOP,
    TOKEN_DIVOP,
    TOKEN_EQOP,
    TOKEN_NEQOP,
    TOKEN_NOTOP,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GTE,
    TOKEN_LTE,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_RBRACKET,
    TOKEN_LBRACKET,
    TOKEN_COMMA,
    TOKEN_HASH,
    TOKEN_NEWLINE,
    TOKEN_BACKLASH,
    TOKEN_EOF
};

struct Token
{
    enum Tokentype TYPE;
    std::string value;
    int lineno;
    int charno;
    std::string sourceLine;
    std::string extra;
};

std::string typetostring(enum Tokentype TYPE)
{
    switch (TYPE)
    {
    case TOKEN_ID:
        return "IDENTIFIER";
    case TOKEN_KEYWORD:
        return "KEYWORD";
    case TOKEN_INT:
        return "INTEGER";
    case TOKEN_EQ:
        return "EQUALS";
    case TOKEN_LPAREN:
        return "LEFT_PARENTHESIS";
    case TOKEN_RPAREN:
        return "RIGHT_PARENTHESIS";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_QUOTE:
        return "QUOTE";
   
    case TOKEN_SINGLEQUOTE:
        return "SINGLE_QUOTE";
    case TOKEN_PLUSOP:
        return "PLUS_OPERATOR";
    case TOKEN_MINUSOP:
        return "MINUS_OPERATOR";
    case TOKEN_MULOP:
        return "MULTIPLY_OPERATOR";
    case TOKEN_DIVOP:
        return "DIVIDE_OPERATOR";
    case TOKEN_EQOP:
        return "EQUALITY_OPERATOR";
    case TOKEN_NEQOP:
        return "NOT_EQUAL_OPERATOR";
    case TOKEN_NOTOP:
        return "NOT_OPERATOR";
    case TOKEN_GT:
        return "GREATER_THAN";
    case TOKEN_LT:
        return "LESS_THAN";
    case TOKEN_GTE:
        return "GREATER_THAN_EQUAL";
    case TOKEN_LTE:
        return "LESS_THAN_EQUAL";
    case TOKEN_LBRACE:
        return "LEFT_BRACE";
    case TOKEN_RBRACE:
        return "RIGHT_BRACE";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_INCREMENT:
        return "INCREMENT";
    case TOKEN_DECREMENT:
        return "DECREMENT";
    case TOKEN_RBRACKET:
        return "RBRACKET";
    case TOKEN_LBRACKET:
        return "LBRACKET";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_HASH:
        return "HASH";
    case TOKEN_EOF:
        return "END_OF_FILE";
    case TOKEN_NEWLINE:
        return "NEWLINE";
    case TOKEN_BACKLASH:
        return "BACKLASH";
    default:
        return "UNKNOWN";
    }
}

class Lexer
{
    public:
        Lexer(std::string sourceCode) 
        {
            source = sourceCode;
            cursor = 0;
            size = source.size();
            current = (size > 0) ? source.at(cursor) : '\0';
            linenum = 1;
            charnum = 1;
            currentLine = std::stringstream();
            ctrl = false;
        }
        std::vector<std::string> keywords = {
            "if", "else", "while", "for", "return", "class", "import", "pass", "break", "continue", "def","type", "true", "false","print", "page", "app", "view", "text", "img", "input"
        };
        
        char advance()
        {
            if (cursor < size)
            {
                currentLine << current;
                char temp = current;
                cursor++;
                charnum++;
                current = (cursor < size) ? source.at(cursor) : '\0';
                return temp;
            }
            else
            {
                return '\0'; // End of source
            }
        }
        void checkAndSkip()
        {
            while (current == ' ' || current == '\t' || current == '\r')
            {   
                advance();
            }
        }

        Token * tokenizeID_KEYWORD()
        {
            std::stringstream buffer;
            buffer << advance();

            while (isalnum(current) || current == '_')
            {
                
                buffer << advance();
            }
            Token * newtoken = new Token();
            newtoken->TYPE = (std::find(keywords.begin(), keywords.end(), buffer.str()) != keywords.end()) ? TOKEN_KEYWORD : TOKEN_ID;
            newtoken->value = buffer.str();
            newtoken->lineno = linenum;
            newtoken->charno = charnum - buffer.str().length();
            newtoken->sourceLine = currentLine.str();
            return newtoken;
        }
        Token  * tokenizeOP(enum Tokentype Type, char expected) {
            std::stringstream buffer;
            buffer << advance();
            if (current == expected)
            {
                buffer << advance();
            }
            Token * newtoken = new Token();
            newtoken->TYPE = Type;
            newtoken->value = buffer.str();
            newtoken->lineno = linenum;
            newtoken->charno = charnum - buffer.str().length();
            newtoken->sourceLine = currentLine.str();
            return newtoken;
        }

        Token * tokenizespecial(enum Tokentype TYPE)
        {
            Token * newtoken = new Token();
            newtoken->TYPE = TYPE;
            newtoken->value = std::string(1, advance());
            newtoken->lineno = linenum;
            newtoken->charno = charnum;
            newtoken->sourceLine = currentLine.str();
            return newtoken;
        }

        Token * tokenizeINT()
        {
            std::stringstream buffer;
            while (isdigit(current))
            {
                buffer << advance();
            }
            Token * newtoken = new Token();
            newtoken->TYPE = TOKEN_INT;
            newtoken->value = buffer.str();
            newtoken->lineno = linenum;
            newtoken->charno = charnum - buffer.str().length();
            newtoken->sourceLine = currentLine.str();
            return newtoken;
        }
        Token * tokenizeSTR(enum Tokentype TYPE) 
        {
            // current should be the opening quote (either ' or ")
            char opening = current;
            int start_lineno = linenum;
            int start_charno = charnum;

            // consume opening quote
            advance();

            std::string value;
            bool closed = false;

            while (current != '\0' && current != '\n') {
                if (current == '\\') {
                    // escape sequence: consume backslash then interpret next char literally (or common escapes)
                    advance();
                    if (current == '\0' || current == '\n') break; // unterminated
                    char esc = advance();
                    switch (esc) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        case 'r': value += '\r'; break;
                        case '\\': value += '\\'; break;
                        case '\'': value += '\''; break;
                        case '"': value += '"'; break;
                        default: value += esc; break;
                    }
                    continue;
                }

                if (current == opening) {
                    // consume closing quote and mark closed
                    advance();
                    closed = true;
                    break;
                }

                // normal character
                value += advance();
            }

            if (!closed) {
                std::cerr << "\nParserError: UnTerminated String at line " << start_lineno
                          << ", column " << start_charno << "\n";
                std::cerr << "  " << start_lineno << " | " << currentLine.str() << "\n";
                exit(1);
            }

            Token * newtoken = new Token();
            newtoken->TYPE = TOKEN_STRING;
            newtoken->value = value;
            newtoken->lineno = start_lineno;
            newtoken->charno = start_charno;
            newtoken->sourceLine = currentLine.str();
            newtoken->extra = std::string(1, opening);
            return newtoken;
        }
        std::vector<Token *> tokenize()
        {
            std::vector<Token *> tokens;
            
            bool noteof = true;
            while (cursor < size && noteof)
            {
                checkAndSkip();
                if(isalpha(current) || current == '_')
                {
                    tokens.push_back(tokenizeID_KEYWORD());
                    continue;
                }

                if (isdigit(current))
                {
                    tokens.push_back(tokenizeINT());
                    continue;
                }
                
                switch (current)
                {
                case '=':
                    {
                            if (peak(1) == '=') {
                                tokens.push_back(tokenizeOP(TOKEN_EQOP, '='));
                                break;
                                continue;
                            } else {
                                tokens.push_back(tokenizespecial(TOKEN_EQ));
                         
                            }
                            break;
                        } 
                
                    case '(':
                    {tokens.push_back(tokenizespecial(TOKEN_LPAREN));
                    break;}

                    case ')':
                    {tokens.push_back(tokenizespecial(TOKEN_RPAREN));
                    break;}
                case '"':
                        {tokens.push_back(tokenizeSTR(TOKEN_QUOTE));
                        break;}
                case '\'':
                        {tokens.push_back(tokenizeSTR(TOKEN_SINGLEQUOTE));
                        break;}
                case '+':
                        {
                            if (peak(1) == '+') {
                                tokens.push_back(tokenizeOP(TOKEN_INCREMENT, '+'));
                                break;
                                continue;
                            } else {
                                tokens.push_back(tokenizespecial(TOKEN_PLUSOP));

                            }
                            break;
                        }
                case '-':
                        {
                            if (peak(1) == '=') {
                                tokens.push_back(tokenizeOP(TOKEN_DECREMENT, '-'));
                                break;
                                continue;
                            } else {
                                tokens.push_back(tokenizespecial(TOKEN_MINUSOP));
                                advance(); // consume '='
                            }
                            break;
                        }
                case '*':
                        {tokens.push_back(tokenizespecial(TOKEN_MULOP));
                        break;}
                case '/':
                        {tokens.push_back(tokenizespecial(TOKEN_DIVOP));
                        break;}
                case '\\':
                        {tokens.push_back(tokenizeSTR(TOKEN_BACKLASH));
                        break;}
                case '#':
                        {tokens.push_back(tokenizespecial(TOKEN_HASH));
                        break;}
                case '!':
                        {
                            if (peak(1) == '=') {
                                tokens.push_back(tokenizeOP(TOKEN_NEQOP, '='));
                                break;
                                continue;
                            } else {
                                tokens.push_back(tokenizespecial(TOKEN_NOTOP));
                                advance(); // consume '='
                            }
                            break;
                        } 
                case '>':
                        {
                            if (peak(1) == '=') {
                                tokens.push_back(tokenizeOP(TOKEN_GTE, '='));
                                break;
                                continue;
                            } else {
                                tokens.push_back(tokenizespecial(TOKEN_GT));
                                advance(); // consume '='
                            }
                            break;
                        }
                case '<':
                        {
                            if (peak(1) == '=') {
                                tokens.push_back(tokenizeOP(TOKEN_LTE, '='));
                                break;
                                continue;
                            } else {
                                tokens.push_back(tokenizespecial(TOKEN_LT));
                                advance(); // consume '='
                            }
                            break;
                        }
                case '{':
                        {tokens.push_back(tokenizespecial(TOKEN_LBRACE));
                        break;}
                case '}':
                        {tokens.push_back(tokenizespecial(TOKEN_RBRACE));
                        break;}
                case '[':
                        {tokens.push_back(tokenizespecial(TOKEN_LBRACKET));
                        break;}
                case ']':
                        {tokens.push_back(tokenizespecial(TOKEN_RBRACKET));
                        break;}
                case ',':
                        {tokens.push_back(tokenizespecial(TOKEN_COMMA));
                        break;}
                case ':':
                        {tokens.push_back(tokenizespecial(TOKEN_COLON));
                        break;}
                case '\n':
                        {
                            tokens.push_back(tokenizespecial(TOKEN_NEWLINE));
                            linenum++;
                            charnum = 1;
                            
                            currentLine = std::stringstream();
                            
                            break;
                        }
                case '\0':
                        {Token * newtoken = new Token();
                         newtoken->TYPE = TOKEN_EOF;
                         newtoken->value = "EOF";
                         newtoken->lineno = linenum;
                         newtoken->sourceLine = currentLine.str();
            newtoken->charno = charnum;
                         tokens.push_back(newtoken);
                         noteof = false;
                         break;}
                
                default:
                   {
                    currentLine << current;
                    charnum++;
                    std::cerr << "\nParserError: " << "Unknown character: "
                    << " at line " << linenum
                    << ", column " << charnum << "\n";

            // show the entire line from the source
            std::cerr << "  " << linenum << " | " << currentLine.str() << "\n";

            // pointer to the error character
            std::cerr << "    ";
            for (int i = 1; i < charnum; ++i)
                std::cerr << " ";
            std::cerr << " ^\n\n";
                    std::cout << "Unknown character: " << current << std::endl;
                    std::cout << "at line: " << linenum << " char: " << charnum << std::endl;
                    exit(1);
                   }
                }
                
            }
            return tokens;
        }
        char peak(int offset)
        {
            if (cursor + offset < size)
            {
                return source.at(cursor + offset);
            }
            else
            {
                return '\0'; // Out of bounds
            }
        }
    private:
        std::string source;
        int cursor;
        int size;
        char current;
        std::stringstream currentLine;
        int linenum;
        int charnum;
        bool ctrl;
};


#endif