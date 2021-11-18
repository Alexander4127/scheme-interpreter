#pragma once

#include <vector>

#include "object.h"
#include "tokenizer.h"
#include "parser.h"

Object* Read(Tokenizer* obj, GarbageCollector* collector) {
    auto res = TryRead(obj, collector);
    if (!obj->IsEnd()) {
        obj->ReadAll();
        throw SyntaxError("Not only one expr");
    }
    return res;
}

Object* TryRead(Tokenizer* obj, GarbageCollector* collector) {
    if (obj->IsEnd()) {
        throw SyntaxError("The end of enter");
    }
    Token next = obj->GetToken();

    if (BracketToken* bt = std::get_if<BracketToken>(&next)) {
        if (*bt == BracketToken::OPEN) {
            obj->Next();
            auto next_next = obj->GetToken();
            Object* res;
            if (std::get_if<BracketToken>(&next_next) &&
                std::get<1>(next_next) == BracketToken::CLOSE) {
                res = nullptr;
            } else {
                res = ReadList(obj, collector);
            }
            obj->Next();
            return res;
        } else {
            throw SyntaxError("Close bracket into read");
        }
    } else if (std::get_if<QuoteToken>(&next)) {
        obj->Next();
        auto object = TryRead(obj, collector);
        return collector->AddObj(new Quote(object));
    } else if (std::get_if<DotToken>(&next)) {
        obj->Next();
        auto object = TryRead(obj, collector);
        return collector->AddObj(new Pair(nullptr, object));
    } else if (SymbolToken* st = std::get_if<SymbolToken>(&next)) {
        obj->Next();
        if (st->name == "#t" || st->name == "#f") {
            return collector->AddObj(new Boolean(st->name));
        }
        return collector->AddObj(new Symbol(st->name));
    } else if (ConstantToken* ct = std::get_if<ConstantToken>(&next)) {
        obj->Next();
        return collector->AddObj(new Number(ct->value));
    }

    throw SyntaxError("Token was not found");
}

Object* GetIfFunction(const std::vector<Object*>& objects, GarbageCollector* collector) {
    if (dynamic_cast<Function*>(objects[0]) || dynamic_cast<FunctionHolder*>(objects[0])) {
        return collector->AddObj(
            new FunctionHolder(objects[0], objects.begin() + 1, objects.end()));
    }
    Symbol* first_one = dynamic_cast<Symbol*>(objects[0]);
    if (!first_one) {
        return nullptr;
    }
    std::string command = first_one->GetName();

    if (command == "quote") {
        return collector->AddObj(new Quote(objects[1]));
    }
    std::vector<std::string> operators = {"<", ">", "<=", ">=", "="};
    for (size_t i = 0; i != operators.size(); ++i) {
        if (operators[i] == command) {
            return collector->AddObj(new GoodOperatorFunc(i, objects.begin() + 1, objects.end()));
        }
    }
    std::unordered_set<std::string> operations = {"+", "*", "-", "/"};
    if (operations.contains(command)) {
        return collector->AddObj(new AddSubOthersFunc(command, objects.begin() + 1, objects.end()));
    }
    if (command == "min" || command == "max") {
        return collector->AddObj(new MinMax(command, objects.begin() + 1, objects.end()));
    } else if (command == "not") {
        return collector->AddObj(new BoolNotFunc(objects.begin() + 1, objects.end()));
    } else if (command == "abs") {
        return collector->AddObj(new AbsFunc(objects.begin() + 1, objects.end()));
    } else if (command == "or" || command == "and") {
        return collector->AddObj(
            new BoolLogicOperation(command, objects.begin() + 1, objects.end()));
    } else if (command.back() == '?') {
        return collector->AddObj(new IsType(command, objects.begin() + 1, objects.end()));
    } else if (command == "cons") {
        return collector->AddObj(new ConsFunc(objects.begin() + 1, objects.end()));
    } else if (command == "car" || command == "cdr") {
        return collector->AddObj(new GetPairElemFunc(command, objects.begin() + 1, objects.end()));
    } else if (command == "list") {
        return collector->AddObj(new MakeList(objects.begin() + 1, objects.end()));
    } else if (command == "list-ref" || command == "list-tail") {
        return collector->AddObj(new GetPartList(command, objects.begin() + 1, objects.end()));
    } else if (command == "define") {
        return collector->AddObj(new Define(objects.begin() + 1, objects.end()));
    } else if (command == "set!") {
        return collector->AddObj(new Set(objects.begin() + 1, objects.end()));
    } else if (command == "set-car!" || command == "set-cdr!") {
        return collector->AddObj(new SetPairElem(command, objects.begin() + 1, objects.end()));
    } else if (command == "if") {
        return collector->AddObj(new IfFunc(objects.begin() + 1, objects.end()));
    } else if (command == "lambda") {
        if (objects.size() <= 2) {
            throw SyntaxError("Wrong count of args in lambda init");
        }
        return collector->AddObj(new LambdaFunc(objects.begin() + 1, objects.end()));
    } else {
        return collector->AddObj(new Holder(objects.begin(), objects.end()));
    }
}

Object* ReadList(Tokenizer* obj, GarbageCollector* collector) {
    if (obj->IsEnd()) {
        throw SyntaxError("Unexpected end in read list");
    }
    Token next;

    std::vector<Object*> objects;
    while (true) {
        next = obj->GetToken();
        if (BracketToken* bt = std::get_if<BracketToken>(&next)) {
            if (*bt == BracketToken::CLOSE) {
                break;
            }
        }
        auto ptr = TryRead(obj, collector);
        if (!ptr) {
            objects.push_back(nullptr);
        } else if (auto np = dynamic_cast<Number*>(ptr)) {
            objects.push_back(np);
        } else if (auto sp = dynamic_cast<Symbol*>(ptr)) {
            objects.push_back(sp);
        } else if (auto bp = dynamic_cast<Boolean*>(ptr)) {
            objects.push_back(bp);
        } else if (auto pp = dynamic_cast<Pair*>(ptr)) {
            if (objects.empty()) {
                throw SyntaxError("Bad pair expression");
            }
            objects.back() = collector->AddObj(new Pair(objects.back(), pp->GetSecond()));
        } else if (auto cp = dynamic_cast<Cell*>(ptr)) {
            objects.push_back(cp);
        } else if (auto qp = dynamic_cast<Quote*>(ptr)) {
            objects.push_back(qp);
        } else if (auto hp = dynamic_cast<Function*>(ptr)) {
            objects.push_back(hp);
        } else if (auto lh = dynamic_cast<FunctionHolder*>(ptr)) {
            objects.push_back(lh);
        } else if (auto h = dynamic_cast<Holder*>(ptr)) {
            objects.push_back(h);
        } else {
            throw SyntaxError("Unknown object during a list");
        }
    }
    auto func = GetIfFunction(objects, collector);
    if (func) {
        return func;
    }
    Object* res;
    if (!objects.back()) {
        res = collector->AddObj(new Cell(objects.back(), nullptr));
    } else if (auto pp = dynamic_cast<Pair*>(&*objects.back())) {
        res = collector->AddObj(new Cell(*pp));
    } else {
        res = collector->AddObj(new Cell(objects.back(), nullptr));
    }
    if (objects.size() == 1) {
        return res;
    }
    for (int i = objects.size() - 2; i != -1; --i) {
        if (objects[i] && dynamic_cast<Pair*>(&*objects[i])) {
            throw SyntaxError("Pair into middle or beginning of list");
        } else {
            res = collector->AddObj(new Cell(objects[i], res));
        }
    }
    return res;
}
