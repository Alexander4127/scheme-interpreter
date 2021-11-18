#include "memory_control.h"
#include "object.h"

void GarbageCollector::FindAllObjects(std::unordered_set<Object*>& need) {
    std::unordered_set<Scope*> useful_scopes;
    for (auto& scope : scopes_) {
        scope->MarkUseful(useful_scopes);
    }
    std::vector<std::unique_ptr<Scope>> cleaned_scopes;
    for (auto& scope : scopes_) {
        if (useful_scopes.contains(&*scope)) {
            cleaned_scopes.push_back(std::move(scope));
        }
    }
    scopes_.clear();
    for (auto& scope : cleaned_scopes) {
        scopes_.push_back(std::move(scope));
    }

    for (auto& scope : scopes_) {
        scope->FindAllObjects(need);
    }
    std::vector<std::unique_ptr<Object>> new_ptr;
    for (auto& ptr : obj_) {
        if (need.contains(&*ptr)) {
            new_ptr.push_back(std::move(ptr));
        }
    }
    obj_.clear();
    for (auto& el : new_ptr) {
        obj_.push_back(std::move(el));
    }
}

void Scope::MarkUseful(std::unordered_set<Scope*>& useful_scopes) {
    bool is_useful = objects_.empty();
    for (auto& [_, el] : objects_) {
        if (!dynamic_cast<Number*>(el)) {
            is_useful = true;
            break;
        }
    }
    if (is_useful) {
        auto cur = this;
        while (cur) {
            if (useful_scopes.contains(cur)) {
                break;
            }
            useful_scopes.insert(cur);
            cur = cur->parent_;
        }
    }
}
