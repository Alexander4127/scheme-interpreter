#pragma once

#include <cassert>
#include <iostream>
#include <variant>
#include <optional>
#include <istream>

#include "error.h"

struct SymbolToken {
    std::string name;

    bool operator==(const SymbolToken& other) const {
        return name == other.name;
    }
};

struct QuoteToken {
    bool operator==(const QuoteToken&) const {
        return true;
    }
};

struct DotToken {
    bool operator==(const DotToken&) const {
        return true;
    }
};

enum class BracketToken { OPEN, CLOSE };

struct ConstantToken {
    int value;

    bool operator==(const ConstantToken& other) const {
        return value == other.value;
    }
};

using Token = std::variant<ConstantToken, BracketToken, SymbolToken, QuoteToken, DotToken>;

class Tokenizer {
public:
    Tokenizer(std::istream* in) : stream_(in) {
    }

    void ReadAll() {
        was_read_ = true;
    }

    bool IsEnd() {
        auto start = stream_->rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
        if (was_read_) {
            return true;
        }
        while (isspace(stream_->peek()) || stream_->peek() == EOF) {
            if (stream_->get() == EOF) {
                GoBack(start);
                return true;
            }
        }
        GoBack(start);
        return false;
    }

    void Next() {
        if (next_pos_ == -1) {
            GetToken();
        }
        stream_->seekg(next_pos_);
        next_pos_ = -1;
        if (!ValidChar(stream_->peek()) && stream_->peek() != EOF && !isspace(stream_->peek())) {
            throw SyntaxError("Next switched to unknown letter");
        }
    }

    bool ValidChar(int ch) {
        if (isalpha(ch) || isdigit(ch) || ch == '+' || ch == '-') {
            return true;
        }
        if (ch == ')' || ch == '(' || ch == '.' || ch == '\'') {
            return true;
        }
        if (ch == '<' || ch == '=' || ch == '>' || ch == '*' || ch == '#' || ch == '/') {
            return true;
        }
        return false;
    }

    bool StringChar(int ch) {
        return (ValidChar(ch) && !(ch == '-') && !(ch == '+') && !(ch == ')') && !(ch == '(')) || ch == '!' ||
               ch == '?' ||
               ch == '-';
    }

    void GoBack(std::istream::pos_type start) {
        stream_->seekg(start);
    }

    Token GetToken() {
        std::istream::pos_type start = stream_->tellg();
        Token result;
        int ch = stream_->peek();
        while (isspace(stream_->peek())) {
            stream_->get();
            ch = stream_->peek();
        }
        if (!ValidChar(ch)) {
            throw SyntaxError("Unknown letter");
        }
        if (ch == '(') {
            stream_->get();
            result = Token(BracketToken::OPEN);
        } else if (ch == ')') {
            stream_->get();
            result = Token(BracketToken::CLOSE);
        } else if (ch == '.') {
            stream_->get();
            result = Token(DotToken());
        } else if (ch == '\'') {
            stream_->get();
            result = Token(QuoteToken());
        } else if (isdigit(ch) || ch == '+' || ch == '-') {
            std::string res(1, stream_->get());
            if ((ch == '-' || ch == '+') && !isdigit(stream_->peek())) {
                result = Token(SymbolToken{res});
            } else {
                while (isdigit(stream_->peek())) {
                    res.push_back(stream_->get());
                }
                result = Token(ConstantToken{std::stoi(res)});
            }
        } else {
            std::string res(1, stream_->get());
            while (StringChar(stream_->peek())) {
                res.push_back(stream_->get());
            }
            result = Token(SymbolToken{res});
        }

        next_pos_ = stream_->rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
        GoBack(start);
        return result;
    }

private:
    bool was_read_ = false;
    std::istream* stream_;
    std::istream::pos_type next_pos_ = -1;
};
