#pragma once

#include <memory>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "error.h"

class Scope;

class Object {
public:
    virtual ~Object() = default;
    virtual Object* Eval(Scope*) const = 0;
    virtual std::string Print(bool) const = 0;
    virtual void MarkAll(std::unordered_set<Object*>&) const = 0;
};

class GarbageCollector {
public:
    Object* AddObj(Object* ptr) {
        obj_.emplace_back(ptr);
        return &*obj_.back();
    }
    void AddScope(Scope* ptr) {
        scopes_.emplace_back(ptr);
    }
    void FindAllObjects(std::unordered_set<Object*>& need);

private:
    std::vector<std::unique_ptr<Object>> obj_;
    std::vector<std::unique_ptr<Scope>> scopes_;
    friend class LambdaFunc;
};

class Scope {
public:
    Scope(Scope* ptr, GarbageCollector* collector) : parent_(ptr) {
        collector_ = collector;
    }
    Scope(Scope* other) : objects_(other->objects_), parent_(other), collector_(other->collector_) {
    }
    void Set(const std::string& name, Object* obj) {
        objects_[name] = obj;
    }
    void MarkUseful(std::unordered_set<Scope*>& useful_scopes);
    void FindAllObjects(std::unordered_set<Object*>& need) {
        for (auto [_, obj] : objects_) {
            obj->MarkAll(need);
        }
    }
    void SetExisted(const std::string& name, Object* obj) {
        if (!Contains(name)) {
            if (!parent_) {
                throw NameError("No such variable in scopes");
            }
            parent_->SetExisted(name, obj);
            return;
        }
        objects_[name] = obj;
    }
    void SetIfNotExist(const std::string& name, Object* obj) {
        auto cur = this;
        while (cur) {
            if (cur->Contains(name)) {
                return;
            }
            cur = cur->parent_;
        }
        objects_[name] = obj;
    }
    bool Contains(const std::string& name) const {
        return objects_.contains(name);
    }
    Object* Get(const std::string& name) {
        if (!objects_.contains(name)) {
            if (!parent_) {
                throw NameError("No such variable in scope");
            }
            return parent_->Get(name);
        }
        return objects_[name];
    }
    Object* ResMemory(Object* obj) {
        return collector_->AddObj(obj);
    }
    Scope* Parent() const {
        return parent_;
    }
    Scope* MakeScope() {
        Scope* new_scope = new Scope(std::remove_const_t<Scope*>(this), collector_);
        collector_->AddScope(new_scope);
        return new_scope;
    }

private:
    std::unordered_map<std::string, Object*> objects_;
    Scope* parent_ = nullptr;
    GarbageCollector* collector_;
    friend class LambdaFunc;
};
