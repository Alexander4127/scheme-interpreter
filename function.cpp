#include "function.h"

Object* EvalBoolValue(bool b) {
    return (b) ? new Boolean{"#t"} : new Boolean{"#f"};
}

Object* CanEval(Object* obj, Scope* scope) {
    if (dynamic_cast<Holder*>(obj) || dynamic_cast<Function*>(obj) || dynamic_cast<Symbol*>(obj) ||
        dynamic_cast<Quote*>(obj)) {
        return obj->Eval(scope);
    }
    return nullptr;
}

bool CheckAllNumbers(const std::vector<Object*>& args, Scope* scope,
                     std::vector<Object*>& eval_args) {
    for (size_t i = 0; i != args.size(); ++i) {
        if (auto eval_arg = CanEval(args[i], scope)) {
            eval_args.push_back(eval_arg);
        } else {
            eval_args.push_back(args[i]);
        }
        if (!dynamic_cast<Number*>(eval_args[i])) {
            return false;
        }
    }
    return true;
}

bool MakeLogicOperation(bool is_and, bool cur, bool next) {
    if (is_and) {
        return cur && next;
    } else {
        return cur || next;
    }
}
