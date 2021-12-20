# scheme-interpreter

This project is a simple implementation of a Scheme language interpreter based on C++.

The project supports above 20 functions from language (with `lambda`) and includes storing variables in addition to "one-string" operations.

It contains three stages of text processing. 

1. The first one is contained by `tokenizer`. Input stream dividing into tokens and after that gotten data goes to the next part of the recognition.

2. The second objective is to make `Objects` from `Tokens`. For this goal is used `object`, `function` and `parser` files which take part in the third stage too. Parser detects syntax problems unnoticed by tokenizer and adds objects into the memory of Interpreter.

3. Lastly, the `Run` method calls `Eval` in taken `Object`. This operation is concerned with several challenges:

  - `Eval` should be common to all objects. This problem has been solved by inheritance inserting. Some structures cannot be evaluated by `(expression)`, others are not appropriate to print (make string to show the operation result).

  - Implementation of `lambda functions` requires work with memory to avoid cycled relationships between `Scope` and `Function`. And `memory_control` exists for achieving correct interaction - `GarbageCollector` memorizes all objects and after each calling `Run` function cleans collector to delete unsaved in `Scope`(-s) and other already unnecessary information.

Example:

```
#include "scheme.h"

Interpreter interpreter;

// std::string Interpreter::Run(const std::string&);

interpreter.Run("(> 5 4 3 2 1)"); // -> #t
interpreter.Run("(max 100 -1 0 10 15)"); // -> 100
interpreter.Run("(/ 100 25 2)"); // -> 2

interpreter.Run("(and (= 2 2) (> 1 0) 10)"); // -> 10
interpreter.Run("(and (= 2 2) (> 0 1) 10)"); // -> #f

interpreter.Run("'(1 . (2 . (3 . ())))"); // -> (1 2 3) 
interpreter.Run("(list-ref '(1 2 3 4 5) 2)"); // -> 3

interpreter.Run("(define (fact x) (if (= x 0) 1 (* (fact (- x 1)) x)))");
interpreter.Run("(fact 5)"); // -> 120
```
