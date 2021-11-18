#pragma once

#include "tokenizer.h"
#include "parser.h"
#include "function.h"

#include <iostream>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <random>
#include <tuple>
#include <sstream>
#include <vector>

std::string EvalRes(Object* ptr, Scope* scope);

class Interpreter {
public:
    Interpreter() : collector_(), scope_(nullptr, &collector_) {
    }

    std::string Run(const std::string& text) {
        Clean();
        std::stringstream ss{text};
        Tokenizer tokenizer{&ss};
        Object* obj = Read(&tokenizer, &collector_);

        if (!obj) {
            throw RuntimeError("Empty list is given");
        } else if (auto sp = dynamic_cast<Symbol*>(obj)) {
            if (scope_.Contains(sp->GetName())) {
                return EvalRes(scope_.Get(sp->GetName()), &scope_);
            }
            throw NameError("Symbol is given");
        }
        return EvalRes(obj, &scope_);
    }

    void Clean() {
        std::unordered_set<Object*> need;
        scope_.FindAllObjects(need);
        collector_.FindAllObjects(need);
    }

private:
    GarbageCollector collector_;
    Scope scope_;
};
