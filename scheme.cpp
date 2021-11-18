#include "scheme.h"

std::string EvalRes(Object* ptr, Scope* scope) {
    if (!ptr || dynamic_cast<Cell*>(ptr)) {
        throw RuntimeError("List is given");
    } else if (auto np = dynamic_cast<Number*>(ptr)) {
        return np->Print(true);
    } else if (auto qp = dynamic_cast<Quote*>(ptr)) {
        return qp->Print(true);
    } else if (auto sp = dynamic_cast<Symbol*>(ptr)) {
        return sp->Print(true);
    } else if (auto bp = dynamic_cast<Boolean*>(ptr)) {
        return bp->Print(true);
    } else if (dynamic_cast<Function*>(ptr) || dynamic_cast<Holder*>(ptr) ||
               dynamic_cast<FunctionHolder*>(ptr)) {
        auto res = ptr->Eval(scope);
        if (res) {
            return res->Print(true);
        } else {
            return "()";
        }
    }
    throw RuntimeError("Wrong type of all expression");
}
