#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "function.h"
#include "tokenizer.h"

Object* TryRead(Tokenizer*, GarbageCollector*);
Object* ReadList(Tokenizer*, GarbageCollector*);
Object* Read(Tokenizer*, GarbageCollector*);
