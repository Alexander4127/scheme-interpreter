#pragma once

#include <memory>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "error.h"
#include "memory_control.h"

class Quote : public Object {
public:
    Quote(Object* ptr) : ptr_(ptr) {
    }
    Quote(const Quote&) = default;
    std::string Print(bool) const override {
        if (!ptr_) {
            return "()";
        }
        return ptr_->Print(true);
    }
    Object* Eval(Scope*) const override {
        return ptr_;
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
        ptr_->MarkAll(need);
    }

private:
    Object* ptr_;
};

class Number : public Object {
public:
    Number(int64_t val) : value_(val) {
    }

    Number(const Number&) = default;

    int64_t GetValue() const {
        return value_;
    }

    std::string Print(bool) const override {
        return std::to_string(value_);
    }
    Object* Eval(Scope* scope) const override {
        return scope->ResMemory(new Number(*this));
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
    }

private:
    int64_t value_;
};

class Boolean : public Object {
public:
    Boolean(const std::string& s) : val_(s) {
    }
    Boolean(const Boolean&) = default;
    std::string Print(bool) const override {
        return val_;
    }
    std::string GetVal() const {
        return val_;
    }
    Object* Eval(Scope*) const override {
        return std::remove_const_t<Object*>(this);
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
    }

private:
    std::string val_;
};

class Symbol : public Object {
public:
    Symbol(const std::string& s) : name_(s) {
    }

    Symbol(const Symbol&) = default;

    const std::string& GetName() const {
        return name_;
    }

    std::string Print(bool) const override {
        return name_;
    }
    Object* Eval(Scope* scope) const override {
        while (!scope->Contains(name_)) {
            if (!scope->Parent()) {
                throw NameError("No such variable");
            }
            scope = scope->Parent();
        }
        return scope->Get(name_);
    }
    void MarkAll(std::unordered_set<Object*>& need) const override {
        need.insert(std::remove_const_t<Object*>(this));
    }

private:
    std::string name_;
};

class Pair : public Object {
public:
    Pair(Object* f, Object* s) : first_(f), second_(s) {
    }

    Object* GetFirst() const {
        return first_;
    }

    Object* GetSecond() const {
        return second_;
    }

    std::string Print(bool) const override {
        throw NameError("Pair should not be printed");
    }
    Object* Eval(Scope*) const override {
        throw NameError("Pair should not be evaluated");
    }
    void MarkAll(std::unordered_set<Object*>&) const override {
        throw std::runtime_error("Pair cannot be used");
    }

private:
    Object* first_;
    Object* second_;
};

class Cell : public Object {
public:
    Cell(Object* f, Object* s) : first_(f), second_(s) {
    }

    Cell(const Pair& p) : first_(p.GetFirst()), second_(p.GetSecond()) {
    }

    Cell(const std::vector<Object*>& obj, GarbageCollector* collector)
        : first_(nullptr), second_(nullptr) {
        Cell* cur = nullptr;
        for (size_t i = 0; i != obj.size(); ++i) {
            auto new_node = static_cast<Cell*>(collector->AddObj(new Cell(obj[i], nullptr)));
            if (cur) {
                cur->second_ = new_node;
                cur = new_node;
            } else {
                first_ = new_node->first_;
                second_ = new_node->second_;
                cur = this;
            }
        }
    }

    Cell(const std::vector<Object*>& obj, Scope* scope) : first_(nullptr), second_(nullptr) {
        Cell* cur = nullptr;
        for (size_t i = 0; i != obj.size(); ++i) {
            auto new_node = static_cast<Cell*>(scope->ResMemory(new Cell(obj[i], nullptr)));
            if (cur) {
                cur->second_ = new_node;
                cur = new_node;
            } else {
                first_ = new_node->first_;
                second_ = new_node->second_;
                cur = this;
            }
        }
    }

    Object* GetFirst() const {
        return first_;
    }

    Object* GetSecond() const {
        return second_;
    }

    Object* GetTail(size_t ind) {
        if (!IsList()) {
            return GetSecond();
        }
        const Object* res = this;
        for (size_t i = 0; i < ind; ++i) {
            auto cp = static_cast<const Cell*>(res);
            if (!cp->GetSecond() && i + 1 < ind) {
                throw RuntimeError("No such tail, index is too large");
            } else if (!cp->GetSecond()) {
                return nullptr;
            }
            res = cp->GetSecond();
        }
        return std::remove_const_t<Object*>(res);
    }

    bool CheckListAndPair() const {
        if (first_ && second_) {
            if (auto cp = dynamic_cast<Cell*>(&*second_)) {
                if (!cp->GetSecond()) {
                    return true;
                }
            }
        }
        return false;
    }

    bool IsList() const {
        const Object* res = this;
        while (true) {
            auto cp = static_cast<const Cell*>(res);
            if (!cp->GetSecond()) {
                return true;
            }
            if (cp->GetFirst() && !dynamic_cast<Cell*>(&*cp->GetSecond())) {
                return false;
            }
            res = cp->GetSecond();
        }
    }

    Object* GetElement(size_t number) const noexcept {
        const Object* res = this;
        for (size_t i = 0; i < number; ++i) {
            auto cp = dynamic_cast<const Cell*>(res);
            if (!cp->GetSecond()) {
                return nullptr;
            }
            res = cp->GetSecond();
        }
        auto cp = dynamic_cast<const Cell*>(res);
        return cp->GetFirst();
    }

    void MarkAll(std::unordered_set<Object*>& need) const override {
        if (need.contains(std::remove_const_t<Object*>(this))) {
            return;
        }
        need.insert(std::remove_const_t<Object*>(this));
        if (first_) {
            first_->MarkAll(need);
        }
        if (second_) {
            second_->MarkAll(need);
        }
    }

    std::string Print(bool b) const override {
        std::string res = (first_) ? first_->Print(true) : "()";
        if (second_ && !dynamic_cast<Cell*>(&*second_)) {
            res += " . " + second_->Print(false);
        } else {
            res += (second_ ? " " + second_->Print(false) : "");
        }
        if (b) {
            res = "(" + res + ")";
        }
        return res;
    }

    Object* Eval(Scope*) const override {
        throw NameError("List should not be evaluated");
    }

private:
    Object* first_;
    Object* second_;
    friend class SetPairElem;
};

template <class T>
std::shared_ptr<T> As(Object* obj) {
    return std::shared_ptr<T>(new T(*static_cast<T*>(&*obj)));
}

template <class T>
bool Is(Object* obj) {
    return dynamic_cast<T*>(&*obj);
}
