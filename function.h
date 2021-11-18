#pragma once

#include "object.h"

Object* EvalBoolValue(bool);
Object* CanEval(Object*, Scope*);
bool CheckAllNumbers(const std::vector<Object*>&, Scope*, std::vector<Object*>&);
bool MakeLogicOperation(bool, bool, bool);

class Function : public Object {
public:
    virtual ~Function() = default;
    std::string Print(bool) const override {
        throw NameError("Cannot be printed");
    }
};

class LambdaFunc : public Function {
public:
    template <class It>
    LambdaFunc(It begin, It end) : data_(begin, end) {
    }
    template <class It>
    Object* FindVal(Scope* s, It begin, It end);

    Object* Eval(Scope* gotten_scope) const override {
        if (!scope_) {
            scope_ = gotten_scope->MakeScope();
        }
        return std::remove_const_t<Object*>(this);
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : data_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    mutable Scope* scope_ = nullptr;
    std::vector<Object*> data_;
    friend class FunctionHolder;
};

class FunctionHolder : public Object {
public:
    template <class Iterator>
    FunctionHolder(Object* ptr, Iterator begin, Iterator end) : func_(ptr), args_(begin, end) {
    }
    Object* Eval(Scope* scope) const override {
        if (auto lambda = dynamic_cast<LambdaFunc*>(func_)) {
            lambda = static_cast<LambdaFunc*>(lambda->Eval(scope));
            return lambda->FindVal(scope, args_.begin(), args_.end());
        }
        if (auto fh = dynamic_cast<FunctionHolder*>(func_)) {
            auto function = dynamic_cast<Function*>((fh->Eval(scope)));
            if (!function) {
                throw NameError("Unexpected argument in evaluating");
            }
            return function->Eval(scope);
        }
        auto res = static_cast<Function*>(func_)->Eval(scope);
        if (auto other_lambda = dynamic_cast<LambdaFunc*>(res)) {
            std::vector<Object*> arg;
            return other_lambda->FindVal(scope, arg.begin(), arg.end());
        }
        return res->Eval(scope);
    }
    std::string Print(bool) const override {
        throw NameError("Cannot be printed");
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    Object* func_;
    std::vector<Object*> args_;
    friend class Define;
};

class Holder : public Object {
public:
    template <class Iterator>
    Holder(Iterator begin, Iterator end) : data_(begin, end) {
    }
    void Reload(const std::vector<Object*>& added_args, Scope* scope) {
        std::string name = static_cast<Symbol*>(data_[0])->GetName();
        auto first = scope->ResMemory(new Holder(data_.begin() + 1, data_.end()));
        std::vector<Object*> obj = {first};
        for (auto el : added_args) {
            obj.push_back(el);
        }
        scope->Set(name, scope->ResMemory(new LambdaFunc(obj.begin(), obj.end())));
        static_cast<LambdaFunc*>(scope->Get(name))->Eval(scope);
    }
    Object* operator[](size_t index) {
        return data_[index];
    }
    size_t Size() const {
        return data_.size();
    }
    Object* Eval(Scope* scope) const override {
        auto func = scope->Get(static_cast<Symbol*>(data_[0])->GetName());
        func = func->Eval(scope);
        return static_cast<LambdaFunc*>(func)->FindVal(scope, data_.begin() + 1, data_.end());
    }
    std::string Print(bool) const override {
        std::string res;
        for (size_t i = 0; i != data_.size(); ++i) {
            res += static_cast<Symbol*>(data_[i])->GetName();
            if (i + 1 < data_.size()) {
                res.push_back(' ');
            }
        }
        return "(" + res + ")";
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : data_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> data_;
    friend class Define;
};

template <class It>
Object* LambdaFunc::FindVal(Scope* s, It begin, It end) {
    auto cur = begin;
    auto my_scope = scope_->MakeScope();
    size_t i = 0;
    auto holder = static_cast<Holder*>(data_[0]);
    while (cur != end) {
        my_scope->Set(static_cast<Symbol*>((*holder)[i])->GetName(), (*cur)->Eval(s));
        ++cur;
        ++i;
    }
    if (data_.size() == 3) {
        auto symbol = dynamic_cast<Symbol*>(data_[1]->Eval(my_scope));
        if (symbol) {
            scope_->SetIfNotExist(symbol->GetName(), my_scope->Get(symbol->GetName()));
        }
    }
    return data_.size() == 3 ? data_[2]->Eval(my_scope) : data_[1]->Eval(my_scope);
}

struct ComparingStructure {
    ComparingStructure(size_t ind) : index(ind) {
    }

    bool operator()(int64_t f, int64_t s) {
        if (index == 0) {
            return f < s;
        } else if (index == 1) {
            return f > s;
        } else if (index == 2) {
            return f <= s;
        } else if (index == 3) {
            return f >= s;
        } else if (index == 4) {
            return f == s;
        } else {
            throw NameError("Unexpected index");
        }
    }

    size_t index;
};

class GoodOperatorFunc : public Function {
public:
    template <class Iterator>
    GoodOperatorFunc(size_t ind, Iterator begin, Iterator end) : args_(begin, end), index_(ind) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.empty()) {
            return scope->ResMemory(EvalBoolValue(true));
        }
        std::vector<Object*> eval_args;
        if (!CheckAllNumbers(args_, scope, eval_args)) {
            throw RuntimeError("Unexpected argument in comparing");
        }
        auto comparator = ComparingStructure(index_);
        int64_t prev_val = static_cast<Number*>(eval_args[0])->GetValue();
        bool res = true;
        for (size_t i = 1; i != args_.size(); ++i) {
            Object* ptr = args_[i];
            auto cur_val = static_cast<Number*>(ptr);
            res &= comparator(prev_val, cur_val->GetValue());
            prev_val = cur_val->GetValue();
        }
        return scope->ResMemory(EvalBoolValue(res));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
    size_t index_;
};

struct OperatorHelper {
    OperatorHelper(const std::string& s) {
        std::vector<std::string> val = {"+", "*", "-", "/"};
        for (size_t i = 0; i != val.size(); ++i) {
            if (s == val[i]) {
                index = i;
            }
        }
    }
    int64_t operator()(int64_t f, int64_t s) const {
        if (index == 0) {
            return f + s;
        } else if (index == 1) {
            return f * s;
        } else if (index == 2) {
            return f - s;
        } else if (index == 3) {
            return f / s;
        }
        throw NameError("Unexpected error in operators");
    }
    int64_t DefaultVal() const {
        if (index <= 1) {
            return index;
        }
        throw RuntimeError("No default value in such operation");
    }

    size_t index;
};

class AddSubOthersFunc : public Function {
public:
    template <class It>
    AddSubOthersFunc(const std::string& s, It begin, It end) : operation_(s), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.empty()) {
            int res = operation_.DefaultVal();
            return scope->ResMemory(new Number{res});
        }
        std::vector<Object*> eval_args;
        if (!CheckAllNumbers(args_, scope, eval_args)) {
            throw RuntimeError("Unexpected argument in operation('+', '-', '*', '/')");
        }
        int64_t res = static_cast<Number*>(eval_args[0])->GetValue();
        for (size_t i = 1; i != eval_args.size(); ++i) {
            res = operation_(res, static_cast<Number*>(eval_args[i])->GetValue());
        }
        return scope->ResMemory(new Number(res));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    OperatorHelper operation_;
    std::vector<Object*> args_;
};

class MinMax : public Function {
public:
    template <class It>
    MinMax(const std::string& func, It begin, It end) : is_max_(func == "max"), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.empty()) {
            throw RuntimeError("Empty arg list in min/max");
        }
        std::vector<Object*> eval_args;
        if (!CheckAllNumbers(args_, scope, eval_args)) {
            throw RuntimeError("Unexpected argument in min/max");
        }
        int64_t def =
            (is_max_) ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int64_t>::max();
        for (auto el : eval_args) {
            auto cur_number = static_cast<Number*>(el)->GetValue();
            def = (is_max_) ? std::max(def, cur_number) : std::min(def, cur_number);
        }
        return scope->ResMemory(new Number(def));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    bool is_max_;
    std::vector<Object*> args_;
};

class AbsFunc : public Function {
public:
    template <class It>
    AbsFunc(It begin, It end) : args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 1) {
            throw RuntimeError("Wrong number of arguments in 'abs'");
        }
        Object* eval_el = args_[0];
        if (auto eval = CanEval(args_[0], scope)) {
            eval_el = eval;
        }
        if (auto number = dynamic_cast<Number*>(eval_el)) {
            return scope->ResMemory(new Number(std::abs(number->GetValue())));
        }
        throw RuntimeError("Unexpected argument in 'abs'");
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class BoolNotFunc : public Function {
public:
    template <class It>
    BoolNotFunc(It begin, It end) : args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 1) {
            throw RuntimeError("Incorrect number of arguments in 'not'");
        }
        Object* eval_el = args_[0];
        if (auto eval = CanEval(args_[0], scope)) {
            eval_el = eval;
        }
        if (auto boolean = dynamic_cast<Boolean*>(eval_el)) {
            return scope->ResMemory(EvalBoolValue(boolean->GetVal() == "#f"));
        }
        return scope->ResMemory(EvalBoolValue(false));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class BoolLogicOperation : public Function {
public:
    template <class It>
    BoolLogicOperation(const std::string& s, It begin, It end)
        : is_and_(s == "and"), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.empty()) {
            return scope->ResMemory(EvalBoolValue(is_and_));
        }
        bool def = is_and_;
        for (size_t i = 0; i != args_.size(); ++i) {
            auto cur = args_[i];
            if (i == args_.size() - 1) {
                return cur->Eval(scope);
            }
            if (auto eval = CanEval(cur, scope)) {
                cur = eval;
            }
            if (auto cur_val = dynamic_cast<Boolean*>(&*cur)) {
                def = MakeLogicOperation(is_and_, def, cur_val->GetVal() == "#t");
            } else {
                def = MakeLogicOperation(is_and_, def, true);
            }
            if (is_and_ ^ def) {
                break;
            }
        }
        return scope->ResMemory(EvalBoolValue(def));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    bool is_and_;
    std::vector<Object*> args_;
};

class IsType : public Function {
public:
    template <class It>
    IsType(const std::string& s, It begin, It end) : name_(s), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 1) {
            throw RuntimeError("Wrong count of args_ in '<type>?' func");
        }
        auto ptr = args_[0];
        if (auto quote = dynamic_cast<Quote*>(ptr)) {
            ptr = quote->Eval(scope);
        }
        if (!ptr) {
            return scope->ResMemory(EvalBoolValue(name_ == "null?" || name_ == "list?"));
        } else if (name_ == "symbol?" && dynamic_cast<Symbol*>(ptr)) {
            return scope->ResMemory(EvalBoolValue(true));
        } else if (auto cell = dynamic_cast<Cell*>(ptr)) {
            bool is_list = cell->IsList();
            if ((is_list && name_ == "list?") || (!is_list && name_ == "pair?")) {
                return scope->ResMemory(EvalBoolValue(true));
            }
            if (cell->CheckListAndPair() && (name_ == "list?" || name_ == "pair?")) {
                return scope->ResMemory(EvalBoolValue(true));
            }
        } else if (name_ == "number?" && dynamic_cast<Number*>(ptr)) {
            return scope->ResMemory(EvalBoolValue(true));
        } else if (name_ == "boolean?" && dynamic_cast<Boolean*>(ptr)) {
            return scope->ResMemory(EvalBoolValue(true));
        }
        return scope->ResMemory(EvalBoolValue(false));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::string name_;
    std::vector<Object*> args_;
};

class ConsFunc : public Function {
public:
    template <class It>
    ConsFunc(It begin, It end) : args_(begin, end) {
    }
    Object* Eval(Scope* scope) const override {
        if (args_.size() != 2) {
            throw RuntimeError("Wrong count of args_ in making pair function");
        }
        return scope->ResMemory(new Cell(args_[0]->Eval(scope), args_[1]->Eval(scope)));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class GetPairElemFunc : public Function {
public:
    template <class It>
    GetPairElemFunc(const std::string& f, It begin, It end)
        : is_first_(f == "car"), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 1) {
            throw RuntimeError("Wrong count of args_ in cutting-pair function");
        }
        auto elem = args_[0];
        if (!CanEval(elem, scope)) {
            throw RuntimeError("Unexpected argument in 'car/cdr' function");
        }
        if (auto cell = dynamic_cast<Cell*>(elem->Eval(scope))) {
            return (is_first_) ? cell->GetFirst() : cell->GetSecond();
        }
        throw RuntimeError("Trying get element not from a pair");
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    bool is_first_;
    std::vector<Object*> args_;
};

class MakeList : public Function {
public:
    template <class It>
    MakeList(It begin, It end) : args_(begin, end) {
    }
    Object* Eval(Scope* scope) const override {
        if (args_.empty()) {
            return nullptr;
        }
        return scope->ResMemory(new Cell(args_, scope));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class GetPartList : public Function {
public:
    template <class It>
    GetPartList(const std::string& s, It begin, It end)
        : is_ref_(s == "list-ref"), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 2) {
            throw RuntimeError("Wrong count of args_ in cutting-list function");
        }
        auto quote = dynamic_cast<Quote*>(args_[0]);
        auto number = dynamic_cast<Number*>(args_[1]);
        if (!quote || !number) {
            throw RuntimeError("Unexpected argument in cutting-list function");
        }
        auto cell = dynamic_cast<Cell*>(quote->Eval(scope));
        if (!cell || !(cell->IsList())) {
            throw RuntimeError("Quote arg is not a list");
        }
        if (is_ref_) {
            auto ref = cell->GetElement(number->GetValue());
            if (!ref) {
                throw RuntimeError("Cannot get tail/ref, index is too large");
            }
            return ref;
        } else {
            return cell->GetTail(number->GetValue());
        }
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    bool is_ref_;
    std::vector<Object*> args_;
};

class Define : public Function {
public:
    template <class It>
    Define(It begin, It end) : args_(begin, end) {
    }
    Object* Eval(Scope* scope) const override {
        if (args_.empty()) {
            throw SyntaxError("Wrong count of args_ in 'define'");
        }
        if (auto hp = dynamic_cast<Holder*>(args_[0])) {
            hp->Reload(std::vector<Object*>(args_.begin() + 1, args_.end()), scope);
            return nullptr;
        }
        if (args_.size() != 2) {
            throw SyntaxError("Wrong count of args_ in 'define'");
        }
        Object* value = args_[1];
        Symbol* name = dynamic_cast<Symbol*>(args_[0]);
        if (!name) {
            throw SyntaxError("Non-symbol value as variable in 'define'");
        }
        if (auto eval = CanEval(args_[1], scope)) {
            value = eval;
        }
        scope->Set(name->GetName(), value);
        return nullptr;
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class Set : public Function {
public:
    template <class It>
    Set(It begin, It end) : args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 2) {
            throw SyntaxError("Wrong count of args_ in 'set'");
        }
        Object* value = args_[1];
        Symbol* name = dynamic_cast<Symbol*>(args_[0]);
        if (!name) {
            throw SyntaxError("Non-symbol value as variable in 'set'");
        }
        if (auto eval = CanEval(args_[1], scope)) {
            value = eval;
        }
        name->Eval(scope);
        scope->SetExisted(name->GetName(), value);
        return nullptr;
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class IfFunc : public Function {
public:
    template <class It>
    IfFunc(It begin, It end) : args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() < 2 || args_.size() > 3) {
            throw SyntaxError("Wrong count of args_ in 'if' function");
        }
        auto cond = args_[0];
        if (auto eval = CanEval(args_[0], scope)) {
            cond = eval;
        }
        auto bp = dynamic_cast<Boolean*>(cond);
        if (!bp) {
            throw SyntaxError("Non-boolean arg in 'if' condition");
        }
        if (bp->GetVal() == "#t") {
            return args_[1]->Eval(scope);
        } else {
            if (args_.size() != 3) {
                return nullptr;
            }
            return args_[2]->Eval(scope);
        }
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    std::vector<Object*> args_;
};

class SetPairElem : public Function {
public:
    template <class It>
    SetPairElem(const std::string& s, It begin, It end)
        : is_first_(s == "set-car!"), args_(begin, end) {
    }

    Object* Eval(Scope* scope) const override {
        if (args_.size() != 2) {
            throw SyntaxError("Wrong count of args_ in 'set-pair-elem' func");
        }
        auto elem = static_cast<Cell*>(args_[0]->Eval(scope));
        auto value = args_[1]->Eval(scope);
        if (is_first_) {
            elem->first_ = value;
        } else {
            elem->second_ = value;
        }
        return nullptr;
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        for (auto el : args_) {
            if (el) {
                el->MarkAll(need);
            }
        }
    }

private:
    bool is_first_;
    std::vector<Object*> args_;
};
