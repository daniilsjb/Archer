# Crafting Interpreters

## Introduction

The making of programming languages is a seemingly mysterious craft. Every programmer, by definition, has used at least one programming language in their life. They make it easy to express solutions and ideas in a human-readable form, as opposed to the rather obscure native language of machines. In fact, it feels so natural that sometimes you may even forget that your program is nothing more than a blob of primitive instructions that manipulate data, stored as binary numbers, by reading it from memory, performing arithmetic operations, making conditional choices, and then loading it back in.

Indeed, modern programming languages are really abstract. This empowers us as creators since today it takes orders of magnitude less time to create a program that would take years to develop even a few decades ago. It's really cool to be able to fit hundreds of Assembly instructions into a handful of high-level code. Yet on some days you may catch yourself on a thought that you don't really have a clue of what's going on under the hood. How does the language itself work? How is it implemented? How can you take a text file, explain its meaning to the computer, and then execute it?

It may seem daunting at first, but then you realize that, to implement a programming language, you really just need a compiler or an interpreter, which on its own is simply a program. Funnily, to implement that program you will likely use one of the already existing programming languages, thanks to bootstraping. Now, how does one create a compiler or an interpreter?

Before answering that, there is a different but not less important question. What *is* a compiler, and how is it different from an interpreter? The answer is actually pretty simple. To execute a program, the computer needs a set of instructions. Because a computer is nothing more than a machine, it is essentially wired-up to read and decode instructions written in a certain format called *machine language*, and each computer architecture has its own machine language. If you want to execute a program written in a higher-level language, all you need to do is translate it to the machine language for your particular machine.

A *compiler* is a program which takes high-level code, reads it, and produces a program with identical semantics but in a different form. That form may be the machine language, in which case the program can be run as-is by the OS. It may be an intermediary form such as bytecode. It may evenbe code for another high-level language, in which case the compiler is technically called a *transpiler*. However, there is an important aspect to note here: the compiler does *not* execute the program itself, it merely translates it to a form that either can be executed or may be translated further.

An *interpreter* is a program which takes code in some form and executes it. This may feel like a very general definition, because it is. In reality there are many types of interpreters: tree-traversal interpreters read the source code, produce an inner representation of the program and then immediately execute it; virtual machines take a set of instructions written in a custom imaginary machine code, referred to as *bytecode*, and then execute it. In general, if a program isn't translated directly into native machine code, the OS cannot run it on its own, so an interpreter helps out.

This may raise a question - which one is better? And the matter of fact is, neither. They do completely different things. In many cases you even want to *combine* them. A really popular approach is to write a compiler that produces bytecode, and then a virtual machine interpreter that executes it. This way you don't need to bother with the specifics of any particular computer architecture, since your bytecode will always be identical. If you implement your virtual machine in a language that is already supported by most architectures, you get portability for free.

Well, almost for free. The problem is, interpreting a program is slower than letting the chip handle it. You do get simplicity and portability, but now you've effectively created a program which runs another program and needs to be separately installed. Does this mean interpreters are bad? Not at all. To compile code into the machine code requires a profound understanding of numerous computer architectures that constantly evolve and change. Naturally, it also requires a ridiculous amount of time.

Of course, it's not all that awful. Compilers usually have separate layers to handle different parts of translation. The front-end is responsible for reading the source code and usually transforms it into some intermediary form. Then for each architecture there is a separate back-end which reads that intermediary code and translates it into native code. There are many back-ends already created and maintained by experts, so the only thing left to create is a front-end. It does take away some freedom, though, because now you have to abide by the rules of the back-end.

Ultimately, it doesn't matter. The programming language itself is nothing more than a specification. However you implement it is a matter of preference and design goals, and many implementations take advantage of both techniques to do even more advanced things like Just-In-Time compilation, where the compiler tracks hotspots in the program at runtime and quickly generates machine code for them. What matters is the understanding that these techniques are different and what advantages they bring to the table.

## Part 1 - jlox

The first part of the book is dedicated to jlox - a tree-traversal interpreter for a toy programming language called Lox, written in Java. This implementation serves as an introduction to general techniques behind lexical analysis (also known as scanning), parsing, abstract code representation, expression evaluation, statement execution, name binding and resolution, and lexical scopes. It also shows the main principles behind how features like control flow, variables, functions, closures, and classes are implemented.

### Chapter 1: Scanning

Implementation of a programming language may be thought of as a pipeline. At the very start you begin with a text file - literally a string of characters - which gets processed in stages and is then executed or translated into something else. The very first step is to take that string of characters and retrieve information from it that is meaningful.

The technical term for "information that is meaningul" is *token*. A token can be thought of as an atomic piece of data for the language, and any program written in that language is ultimately comprised of tokens. For example, any keyword, operator, literal, or identifier is a token.

Scanning, or lexical analysis, is the process of splitting a program into tokens. Each type of token is defined by some set of rules. For example, a number may be a sequence of characters that starts with a digit and only contains digits. A string may be anything between a pair of double-quotes. A keyword is, obviously, a sequence of characters that exactly matches one of the reserved words in the language.

A scanner, or lexer, is thus a system that reads the source code, one character at a time, and decides what tokens they belong to. It can be thought of as a finite state machine - it reads in a character, determines what token may start with this character, and then transitions to a state where it treats a series of subsequent characters as if they are part of this token. After it reaches some kind of end condition, it outputs the token. This process gets repeated until the end of the source string.

For example, if a double-quote is encountered, the scanner shall read all characters until it reaches another double-quote, accumulating each character into a *lexeme* - a literal piece of text that describes how the token looked like in the user program - and then produce a string token. Some tokens only take up one character, so they are much easier to recognize. Some tokens may start with the same sequences of characters, in which case the scanner prefers the longest token it may find. Things like whitespaces and comments are ignored, but they also separate tokens.

Interestingly, the fact that a scanner behaves like a state machine is not arbitrary. There is a whole field of linguistics, mathematics, and computer science known as "formal languages", which outdates compilers. In short, it's built around the idea that you can take a series of *letters* over an *alphabet* (the available set of letters), and combine these into *words*. Then you can define a *grammar* - set of rules by which the characters can be combined into words. As the result you have a *language* - the total set of words that you can produce with a given grammer over a given alphabet.

The grammar is expressed as a set of *production rules*. Each production rule consists of two sides (the left side and the right side). Production rules are straightforward: given a string of symbols on the left side, you can substitute ut with the string of symbols on the right side. Each symbol may either be *terminal*, meaning it cannot be substituted any further, or *non-terminal*, which is the opposite. There may be multiple production rules for the same string of symbols, in which case the choice of substitution is arbitrary. Production rules may also be recursive, which allows languages to be practically infinite. This is where programming languages' flexibility comes from.

Knowing how to define a language, a useful thing to be able to do is classify it. For that purpose there is a classification system called "Chomsky Hierarchy", named after American linguist Noam Chomsky. In short, it describes four types of languages with different restrictions: regular, context-free, context-sensitive, and recursively enumerable. These languages appear in the order from most restrictive to least restrictive. Regular languages may be recognized with a finite state machine, context-free languages with stack machines, context-sensitive with linear bounded Turing machines, and recursively enumerable with any Turing machine (instinctively enough, as the whole production rules logic is similar to the primary operation of a Turing machine).

We are interested in the regular languages - they may be defined with regular expressions, which are essentially a notation for state machines. Because languages that fall into this category can be analysed with a state machine, they don't require any extra memory than is required to encode the state machine itself. In other words, they don't need RAM.

The majority of languages from the C family are regular - their grammar is simple enough and lack ambiguities. Languages like Python and Haskell aren't, however. Because they use indentation to determine the boundaries of statement blocks, they need to keep track of the current indentation level, which means they do require memory, which in turn means they cannot be described by a state machine.

A nice feature of regular languages is that you only need a set of regular expressions to fully define its grammar, which means the scanning code itself is pretty standard - all the important logic is in the regexes. A long while ago people made this observation and created so-called "compiler compilers", tools that can take a handful of regular expressions and generate appropriate lexer code.

At the end of the scanning stage, the scanner has either encountered a lexical error such as an unexpected character, or produced a series of tokens for the program containing various meta-data - the type of the token, its lexeme, its location in the file, and such - ignoring non-important things like whitespaces and comments. From now on, the language pipeline doesn't bother with indidividual characters anymore - now it deals with tokens.

**Key Points:**

* The first step towards understanding the user's program is scanning it
* Scanner reads the program character at a time and produces meaningful tokens
* The rules for extracting tokens are defined with a grammar
* There are different types of language grammars, but the most common in scanning are regular languages that may be expressed with regular expressions

### Chapter 2: Representing Code

Before moving on to the next stage of the language pipeline, the program, which is now broken down into tokens, needs to be represented in some way according to the grammar of the language itself. For example, given a mathematical expression, we need to break it down into a nested list of sub-expressions - binary operations, unary operations, groupings, and actual numbers to operate on.

To achieve this, the language itself needs grammar. This time it's not lexical grammar, but the actual language grammar. Whereas previously we operated on characters and formed tokens, we now want to take in tokens and form expressions. Indeed, we're still in the formal language territory, but one level of granularity higher. Expressions, however, are more complex because they are *recursive* - they can nest. We now need to keep track of where we are in the grammar, meaning that at this point we're dealing with a context-free grammar. The description of the grammar may be created using the *Backus-Naur Form*. It was initially invented for ALGOL, but today there are numerous variations of this notation used by many languages.

Because language syntax is recursive, an obvious choice is to represent expressions as a tree. Indeed, the common data structure designed for this particular task is *abstract syntax tree*, or AST for short. Each *node* of an AST represents a single expression, and has a type associated with it, such as binary, unary, or literal for numbers. Each expression contains tokens and sub-expressions necessary to describe that expression. As such, during the next stage of the language pipeline - namely, parsing - the goal is to fuse tokens into a tree of expression nodes.

The AST will later be traversed by various systems in the interpreter. We may be interested in printing nodes for debugging purposes, statically resolving information, and evaluating expressions. In a language like Java, an obvious solution would be to add a single method for each operation to the node itself. Whenever we add a new node type, we implement all of these operations in one place. On the other hand, adding a new type of operation means going through each already existing node and modifying it. This happens because nodes know exactly how they are used.

If we used a functional language like Haskell, the problem would actually flip, since in these languages we define functions with types that they may operate on. Adding a new function means implementing it for each node type in one place, but adding a new node type requires us to modify each already existing function.

This problem was discovered by people who were working on context-free grammar languages dealing with expression representations, which is why this problem is called the *expression problem*.

It is a good practice to separate data from how it is used, so for jlox we choose functional approach. Java doesn't support pattern matching, but luckily we can emulate similar behavior using the *Visitor pattern*. The Visitor is an interface which declares a function for each type of object it may operate on. The object itself may then *accept* the Visitor by passing itself to the appropriate function. This allows us to define different traversal strategies with different Visitor implementations.

**Key Points:**

* The code needs to be represented in some way. For tree-traversal interpreters, a suitable data structure is an abstract syntax tree, where each node represents a single expression
* Expressions are built from tokens and other expressions, which make them recursive
* Recursive languages are not regular because a parser needs to keep track of where it is in the language grammar
* In practice, most programming languages are context-free, meaning they can be parsed with a stack. Technically, it may be the implementation language's stack
* Visitor pattern may be used to mitigate the expression problem when implementing traversal strategies for abstract syntax tree

### Chapter 3: Parsing Expressions

The next stage in the language pipeline is generating an AST from the tokens produced by a scanner. This process is known as *parsing*, and it is done by a *parser*. The AST will be generated in accordance with the language's grammar rules.

Parsing is slightly more complicated than lexical analysis due to ambiguities. There may be seemingly ways to parse an expression. The best example for this are mathematical expressions - due to difference in precedence and associativity between operators, the expression may not always be evaluated from left to right. Because the AST will be used to evaluate expressions, we need to make sure its structure accounts for precedence and associativity.

Precedence is commonly handled with a clever observation: a lower-precedence operator takes in results of higher-precedence operators as its operands. Because multiplication is done before addition, we can effectively say that the operands of addition are the results of multiplications. Thus, we build the production rules of our grammar as a hierarchy, where each rule only allows nested expressions of higher precedence. This applies to all rules, not just mathematical expressions.

Associativity may be handled in coordination with precedence. When parsing an expression, we may allow sub-expressions to have the same level of precedence as the enclosing expression to make it right-associative. That allows accumulating expressions into a chain. For left-associative expressions we only allow sub-expressions if they are at least one level higher.

There are many techniques used for parsing: LL(k), LR(1), LAL, parser combinators, Earley parsers, shunting yard algorithm. One of the easiest and most effective parsing algorithms is *recursive descent parsing*. It is considered a top-down parser, meaning it starts at the top-most grammar rule and descends deeper into sub-expressions, recursively. To implement one, you translate each production rule of your grammar into a function. This function reads tokens and other expressions, defined by other functions, and forms AST nodes out of them. Thus we effectively construct a tree.

In case the parser encounters an error in the user's code, at the very least it is responsible for reporting it and not crashing or hanging. Ideally, it should report as many distinct errors as possible, while also making sure the amount of *cascading* errors is reduced. Cascading errors are caused whenever the parser is confused, since erroneous code in one part of the expression means that the entire expression likely doesn't abide by the grammar's rules. To do this, the parser enters *panic mode*. While in that mode, it discards everything until it is sure that it's hit a different expression or statement, which is often determined by semi-colons.

**Key Points:**

* Parsing is the process of reading tokens in accordance with the grammar's rules to generate a representation of code, such as an abstract syntax tree or bytecode
* Precedence and associativity must be taken in account when parsing expressions, which may be done by structuring the production rules of a grammar such that each rule only contains expressions with higher precedence
* A common technique for parsing is recursive descent, in which the grammar is handled from its top-most rule (least precedence) and descents deeper, recursively
* Each production rule in recursive descent is handled by a single function
* If an error is encountered, the parser might enter panic mode and discard subsequent tokens until it enters a different expression or statement to minimize cascading errors

### Chapter 4: Evaluating Expressions

At this point the language pipeline has generated a meaningful representation of the user's program in the form of an AST. Each node in the AST is an expression, so the next logical step is to perform expression evaluation. This is done by implementing a traversal strategy using the Visitor pattern mentioned earlier, which reads expressions, extracts values from them, and performs all of the necessary computation depending on the expression type.

For values, Lox supports four basic types of data: numbers, Booleans, strings, and nil. Each of these has a respective Java representation, which we will use. Determining data type of a given Lox object at runtime is thus possible using the `instanceof` operator. This allows us to easily evaluate various expressions: literal expressions simply produce values, unary and binary operations modify these values, equality and comparison check values against it each other and produce Booleans, and truthiness is interpreted - only `nil` and `false` are considered to be falsey.

In case user is trying to perform an illegal operation, such as multiplying non-numeric numbers or dividing by zero, a runtime error is thrown. The errors are represented with Java exceptions, which are then handled by the interpreter to provide user with information on where the error occurred.

**Key Points:**

* Evaluating expressions is done easily with the Visitor pattern, which has a single method for handling each type of expression
* Expressions produce values that may then be used by other expressions
* Java provides us with necessary mechanisms to check the type of a value at runtime
* In case something goes wrong when evaluating an expression, a runtime error is thrown and reported

### Chapter 5: Statements and State

A crucial part of any programming language is ability to store data in computer's memory. Another important feature is to make use of that data by producing side effects, such as writing to a standard input or a file, sending data to connected devices to play audio or draw graphics, or making a network request. Writing data to memory is also a side effect.

To address these tasks, programming languages have notion of *statements*. The main difference between expressions and statements is that the former produce values, whereas the latter use these values to produce side effects. Sometimes, these two are combined into an *expression statement*. These look like ordinary expressions, but they are treated as statements by the programming language. As it turns out, we use these very often. For example, a statement consisting of a single function call is an expression statement. Thus, a program can be described as a sequence of statements. We treat statements as an extension of expressions, meaning they are part of the AST. Thus, the Visitor we've been using so far also learns to execute each available type of statement.

To write data to memory, we require one of the most basic statements - variable declaration. When declaring a variable, we give it an *identifier*. Whenever we want to use the value of the variable we do so by its identifier. As such, the language binds each identifier to a memory location, and any usage of that identifier is resolved to that location. Because Lox is a dynamic language, we want to be able to look up variables by name at runtime. A suitable data structure for this is a hash map, which we usually call an *environment* in the context of variable binding. Thus, declaring a variable is as simple as adding a new entry into the environment with either a default or user-provided value.

Assignment is a unique expression in the sense that it produces a side effect like a statement by mutating the value of a variable, but also produces the newly assigned value like an expression. Because it's used on variables, it accepts only *l-values* as its operands. People familiar with C++ may recognize the terminology, but in short, *l-values* are values that have physical memory associated with them (i.e. variables), and *r-values* are ordinary values that are floating around (i.e. literals, temporaries, or even values *read* from variables).

Environments are also used to implement scoping. Each time a new scope is entered, we create a new environment. To keep track of all variables available at any given time, we structure our environments into a data structure known as a *parent-pointer tree*, or a *cactus stack*. Each environment has a pointer to its enclosing environment, or parent. Thus, when looking up a variable, we start from the innermost environment and traverse up into the list. Because we start at the innermost environment, variables from enclosing environments are shadowed correctly.

**Key Points:**

* Expressions produce values, while statements use these values to perform side effects like writing data to memory
* Each variable has a string identifier associated with it, which binds it to a particular memory location
* To look up variables at runtime, a common technique used by dynamic languages is to use hash maps, also referred to as environments in this context
* Scoping is done by nesting environments as a tree where environment points to its enclosing environment
* Shadowing is done by resolving variables from the innermost environment

### Chapter 6: Control Flow

Another crucial part to any programming language are *control flow* statements. Not long before computers were invented, mathematicians encountered a peculiar problem. As it turned out, mathematics was vulnerable to *paradoxes*. A very famous one was the *Russell's Paradox*, which stated that if `R is the set of all sets that don't contain themselves`, does `R` contain itself? If not, then it should, according to the second rule. If it does, then it cannot contain itself, again according to the second rule.

Worried, mathematicians started questioning the foundation of mathematics, and to address this crisis they were going to start from the very beginning and rebuild mathematics using only a handful of axioms and logic rules. As part of that process, they wanted to rigorously answer questions like, `Can all true statements be proven?`, `Are all functions that can be defined computable`, and `What does it mean for a function to be computable?`.

As part of that search for answers, two mathematicians, Alan Turing and Alonzo Church, independently invented two of the most widely used models of computation that served as the theoretical foundation of computer science - Turing machine and lambda calculus, respectively. Both of these models were simple, tiny systems with only a handful of straightforward rules that were just enough to perform any *possible* computation.

The Turing machine can be thought of as an upgraded state machine. At any given point in time it can be in any of a finite set of states. While in that state, it reads a tape of symbols, which serves as memory. Given the symbol read from the tape and the current state of the machine, it can make a decision, which may involve writing a new symbol onto the tape and transitioning to another state. The process repeats until the machine reaches a special sentinel state, the *halting state*, which finishes the execution of computation, allowing the result to be read from the tape.

Because this set of rules is enough to perform any possible computation, it is said that a system such as a computer or a programming language is *Turing-complete* if it is expressive enough to simulate a Turing machine, and as such is capable of computing anything that is computable. In reality, however, this is not exactly true because Turing machines assume that the memory tape is infinite and give no constraint to the time necessary to perform computation. In practice, computers are only capable of computing what is reasonable in terms of time and space.

A crual part to simulating a Turing machine is the ability to make decisions and transition to other instructions based on these decisions. For that purpose, there are two types of control flow: *branching* and *looping*. Branching is the ability to selectively execute code, and looping is the ability to repeat the same instruction while a certain condition is met. To express conditions, we also need logical operators, namely *not*, *and*, and *or*.

When executing a conditional statement, we use Java's built-in control flow mechanisms to evaluate condition expressions. Based on the truthiness of the produced value, we select an appropriate branch or decide whether to continue iterating over a loop or not. Logical expressions are implemented similarly because they have the ability to *short-circuit* - if the first term of an *and* is false, the entire expression is false. Likewise, if the first term of an *or* and true, the entire expression is true.

A technique we may use for parsing `for` loops is called *desugaring*. The loop consists of three parts: *initializer*, *condition*, and *increment*. Initializer is executed before the loop, condition is evaluated before each iteration of the loop, and the increment happens at the very end of each iteration. The identical semantics may be expressed with an ordinary `while` loop. For that reason, a `for` loop may actually be transformed into a `while` loop during parsing and then executed in the same way.

**Key Points:**

* Control flow is crucial towards making a programming language expressive enough to perform computation
* It is implemented by conditionally executing branches of code or executing same code multiple times
* Logical expressions are a type of control flow because they may short-circuit, thus executing code conditionally
* Desugaring is the process of taking a statement and transforming it into another set of statements with identical semantics the language already supports

### Chapter 7: Functions

A convinient feature that most programming languages support is a way of reusing the same code across multiple places in the program. This is done with functions, subroutines, or procedures, but the latter terms are used less frequently.

To handle functions, we create a new data type that describes a function object, as well as an interface that is implemented by all callable objects. In case of functions, that interface simply invokes the function's body, which itself is a block of statements. Thus, calling an object that isn't callable results in a runtime error. An error may also be thrown if the *arity* of a function - the number of parameters it expects - does not match the actual number of arguments. When parsing a function declaration, we produce a function object and store it in the current environment.

A function call consists of two parts - the *callee* and the actual call. The callee is any expression that may be evaluted to a function. The call mandates that we are interested in executing the body of the function, and is also responsible for passing in *arguments* to it. Conversely, each function may also specify a list of *parameters* that it expects. The difference between parameters and arguments is subtle but important - parameters are part of the declaration and don't have any specific values, while argument are the actual values passed into the function.

To let users interact with the system, we add *native* functions. These functions are implemented in Java, but are accessible from the user's program. As such, we can interact with the user's system from Java, but the effect is seen in Lox. Effectively this is a way for us to implement a standard library. To create native functions, we make Java classes that implement the callable interface and manually register them in the global environment.

To return values from functions, the `return` statements are used. Each function has an implicit `return` at the end of its body. Whenever a `return` statement is encountered, execution of that function terminates and an optional value may be produced. The difficult is that we may want to return from a nested call, in which case only the current function must be terminated. To implement this in JVM, we throw a special sentinel exception, which carries the return value of the function. That causes the call stack to be cleared up until the point where the exception is first caught. That exception is not treated as an error, but rather instantly caught by the code responsible for calling the function and simply returns the payload of the exception as the product of function call.

As most dynamic languages, Lox supports *closures* - a mechanism that allos users to define local functions inside other functions. By default, the inner function is only accessible to itself and the outer function, but the outer function may return the inner function, thus letting the outside world use it. In that case the inner function will outlive invocation to the outer function, but all references it holds to variables declared in the outer function must still be valid. As such, each function needs to store a reference to the enclosing environment to make sure it doesn't get destroyed.

**Key Points:**

* Functions let users reuse same piece of code
* Function bodies are treated as ordinary block statements
* Parameters of a function are part of the declaration, and arguments are the actual values passed in
* The arity of a function is the number of parameters it expects
* `return` statements terminate function execution and allow it to produce a result
* Each function has an implicit `return` statement at the end of its body
* Closures are a way for a function to capture the enclosing environment to make sure it doesn't get destroyed

### Chapter 8: Resolving and Binding

Our current implementation of closures has an error snuck in. Because scopes are part of the *static* side of the program, they can be fully resolved at compile time just by looking at the source code. Runtime cannot affect scoping mechanism, and thus variable resolution, in any way. Alas, it happens now. When we define a function, it gets a reference to the surrounding environment. That lets it refer to variables that may otherwise not have existed, but we've no guarantee what happens between the point when the function is defined and where it is used. For example, the closure may refer to a global variable at the point of function declaration, but the function may be invoked from a different scope with another variable of the same name. The "imposter" variable will shadow the one that was originally captured by the closure, and the behavior of the program will be incorrect.

Because scopes are static, we can resolve all bindings lexically when reading the source string in a separate *resolving* pass right before executing the code. While many languages perform multiple compilation passes to perform advanced optimizations, resolvings, checks, and error reports, our needs are not as sophisticated. To do resolving, we simply add a new traversal strategy using the Visitor pattern.

To resolve a variable, we simply need to determine the environment where it can be found. Because we only create environments at runtime, we can represent this information during resolving with an offset from the current environment into the environment tree. By saving this information in the interpreter, we can map each distinct access of a variable to an offset that lets us know exactly how far we need to traverse the environment hierarchy to find the variable. In case the offset isn't found, we assume the variable is global and look for it in the global scope instead.

**Key Points:**

* Static information about a program can be determine during compilation and cannot be affected during runtime
* Scopes are static because they can be determined lexically, and as such all variable bindings can be resolved during before the code is executed
* Resolving variables upfront lets us make sure we're not mixing up memory locations when the program is running

### Chapter 9: Classes

A class in Lox is described by three things: *constructor* that makes sure objects are always initialized in a correct state, *fields* that let class instances store data, and *methods* to let classes execute various tasks. The class itself is just another type of object. Each class needs to know its name, as well as hold a hash map of methods that are attached to it. We store class objects in the current environment like we do with all other variables.

To invoke a class' constructor, the class itself must be called as if it was a function. Thus, classes implement the callable interface by producing a new instance of itself. If the class contains a method called `init`, we invoke that method instead, which will both produce and set the instance up. An instance is yet another object that stores a pointer to its class, as well as a hash map of fields attached to it.

To access fields and methods of an object, we add two new types of expressions: get and set. These are similar to ordinary variables, but use the `.` syntax, which allow them to nest arbitrarily deeply. When executing a get or a set expression, the interpreter needs to know both the object that it operates on and the name of the field or method.

A feature that distinguishes methods from functions is that they have access to the object they operate on via `this` keyword. Since it is possible to define closures inside methods, as well as obtain references to methods, we want to make sure that `this` is always bound to the receiver that the reference to a method was first retrieved from. Each method gets a hidden environment that serves as a closure and only contains a single variable named `this` that referes to the receiver. Each time a method is called, the environment first needs to be set up with the appropriate receiver.

**Key Points:**

* Classes contain methods, which may include an initializer
* Instances are created from classes and contain fields
* When calling a class, a new instance is returned either directly or via the initializer if it's present
* Fields are methods are accessed via `.` syntax on objects
* Whenever a method is called, it creates a hidden closure which contains a variable named `this` that refers to the receiver

### Chapter 10: Inheritance

To inherit from a superclass, each class stores a reference to it. In Lox, classes are only allowed to inherit methods. Thus, when accessing an object's method, if it's not found directly on the class, then we want to check each superclass' method table until we either find it or assume that the method is undefined.

In case it is desirable to call the superclass' method specifically, Lox provides `super` keyword. Unlike `this`, `super` does not refer to anything on its own. It is simply a way for the user to express that they want to call a method on the current instance, but start looking it up at a certain point in the hierarchy. Because of this, calls to `super` are inseperable from the `.` syntax.

Other than that, `super` works in a very similar way to `this`. Due to closures and method references, `super` must always refer to the same class in a method reference, regardless of where the method was called from. Just as with `this`, we create a hidden environment right above the environment containing `this` when compiling a class and let each method capture it.

**Key Points:**

* Classes are only allowed to inherit methods, thus if a method is not found directly on the class, it must be looked up on the superclass if it exists
* In cases where it is desirable to call the superclass' method specifically, a call to `super` must be used
* The way `super` works is completely analogous to `this` - it is stored in a hidden environment just above the one where `this` exists, which is created during class compilation and captured by each method

## Part 2: clox

Our first implementation, jlox, is simple but it's not very fast. Tree-traversal interpreter is a fundamentally slow model. The AST is something of a linked-list, where each node owns references to other nodes. However, there is no guarantee as to how these nodes are structured in memory - two related nodes in the program may be located at the opposite ends of memory. Garbage collector adds even more uncertainty because it is allowed to reallocate data around memory. This, in turn, means that the implementation is less cache-friendly.

On the other hand, we've got the host language - Java. Admittedly, it's not the fastest language available to us. Although garbage collection is useful to simplify memory management, it does add an overhead onto the entire interpreter, even though we only need garbage collector for the Lox program itself. In addition, we've relied on the JVM to handle most of our language's mechanisms at runtime, which is bad for learning how things really work under the hood.

Thus, the second part is dedicated to clox - a Virtual Machine that interprets bytecode, written in C. With all of the fundamental knowledge we've gained the first part, we are now able to understand how to implement a more efficient implementaiton.

### Chapter 1: Chunks of Bytecode

The basis of the Virtual Machine we're building is *bytecode*, which can be thought of as imaginary machine code. Because it doesn't run natively on any real hardware, we need a Virtual Machine that would emulate hardware that could run it. This gives us a much more thorough control over the execution of the user's program. Another huge advantage of bytecode is in its structure - it's effectively a sequenced blob of bytes, hence the name. The program is executed by going through it byte by byte. This sequence of bytes is much easier to cache, which provides a significant boost for performance.

To represent a chunk of bytecode, we need a data structure that can grow dynamically, depending on how large the user's program is - a dynamic array. Each element of that array will contain a single byte. To implement a dynamic array in C, we need *capacity*, *count*, and an array *pointer*. Capacity represents the total allocated size of the array. Count represents how many elements are used. Array pointer is the actual region in memory where elements are stored. Whenever count overflows the capacity, the capacity is increased and the array is reallocated.

To keep track of what bit patterns match to which instruction, we need an *instruction set*. It can be thought of as the repertoire of a machine - be it virtual or physical. Each instruction in the set is a pattern of bits, and the machine knows how to decode these patterns and execute the approprite operation based on the instruction. The bit pattern of each operation is called *opcode*, and opcodes are usually given human-readable mnemonics for convenience. In clox, we use an enum to represent the instruction set and give each opcode a human-readable name.

Apart from instructions themselves, we also want to encode *operands* to these instructions as part of the bytecode. The problem is, however, that operands could be values of different types and sizes, and in most cases they won't fit into a single byte of data. To address this, a separate *constant pool* is created. It is effectively a dynamic array where we store all constants in the user's program. To read a constant, we simply pass in its index into the array via bytecode. If we exceed 256 indices available in a single byte, we may extend the instruction set with so-called *long* instructions that may read in multiple bytes at a time and combine them into singular indices.

For debugging purposes, we also need a *disassembler*. It is a system capable of reading a chunk of bytecode and printing out its contents in textual form. In most cases this means taking in the opcode and printing out its mnemonic, alongside information like the line where the instruction was encountered and what its operands were. Because of operands, different instructions may take up different sizes, so the disassembler works similarly to a state machine - it transitions to a state where it disassembles a particular instruction, consumes some number of bytes, and then transitions back to the original state.

**Key Points:**

* Bytecode is imaginary, ideal machine code that a programming language targets
* Due to its sequenced nature and blob-like structure, bytecode is cache-friendly
* Bytecode may be represented with a dynamic array of bytes
* Any machine, be it virtual or physical, has an instruction set it is capable of executing
* Each instruction has an opcode - a pattern of bits. Opcodes usually come with human-readable mnemonics
* Each byte of bytecode is either an opcode or an operand to an instruction
* Constants cannot fit into a byte, so they are stored in a separate array and are accessed by index
* To handle cases when a single byte is not enough, many instruction sets feature so-called *long* instructions
* A disassembler reads in a chunnk of bytecode and prints it in textual form

### Chapter 2: A Virtual Machine

To have a better understanding of how a Virtual Machine executes bytecode, we start at the back-end of the language implementation. To execute code, the VM holds a reference to a bytecode chunk, as well as an *instruction pointer* (IP for short), also known as *program counter* (PC for short). The IP is a pointer into the bytecode chunk that points to the next instruction to be executed. Initially, the IP points to the beginning of bytecode. The VM then enters a loop where it advances the IP on each iteration, *decodes* the instruction just read, and then *dispatches* it. Thus, this process is called *dispatch loop*. It is one of the most performance-crritical parts of the VM since this is where the user's program is executed. There are various techniques to keep things optimized, such as "direct threaded code", "jump table", and "computed goto".

At this point we're mostly interested in loading constants from the constant pool by index and performing arithmetic. The former is trivial. For the latter, we need a way of encoding the operands to arithmetic instructions. You may be familiar with the notion of *Reverse Polish Notation*, also known as *postfix* notation. In RPN, you write operators after their operands, as opposed to *infix* that we use everyday, where operators are placed between their operands. To evaluate an expression written in RPN, you simply read it from left to right, pushing operands onto a stack, then popping them off whenever an operations is encountered and pushing back the result. Thus, the Virtual Machine needs a stack to evaluate expressions.

This makes our VM *stack-based*. Whenever it encounters a constant, it is loaded and pushed onto the stack. Whenever it encounters an operation that takes in arguments, it pops a certain number of values from the stack depending on the operation's arity. It then performs that operation with popped values and pushes the result back onto the stack. Another type of VMs is called *register-based Virtual Machines*. They also rely on a stack, but their instructions are allowed to specify exact slots into the stack instead of always assuming that these are going to be on the top. It makes them more efficient, but also more complex.

**Key Points:**

* A Virtual Machine loops over each instruction in bytecode, decodes it, and then dispatches it
* To keep track of where in the execution the Virtual Machine is, it stores an instruction pointer which points to the next instruction to be executed
* Virtual Machines commonly use stacks to store temporary values that they operate on
* Whenever a constant is encountered, it is loaded and then pushed onto the stack
* Whenever an operationg is encountered, it pops some values from the stack, computes the result, and then pushes it back on

### Chapter 3: Scanning on Demand

Scanning the program is mostly identical to jlox, but some parts are optimized and adapted to C. In particular, we're being more throughtful about the way we pass around strings. In Java, we could simply pass in a string object containing the entire source code of the program. In C, we have to pass in a pointer to that string instead.

Whereas in Java we used an ordinary ArrayList to return the entire sequence of tokens from the scanner, we don't have that luxury in C. We could implement a dynamic array again, but it is not necessary - the compiler will never need to know the entire sequence of tokens at once, as we know it requires at most one token of *lookahead*, practically making it a LL(1) parser. Thus, we can avoid the overhead of constantly growing a dynamic array by simply scanning a single token whenever the compiler requests it. This means that the scanner now must guarantee that it always return a valid token on request. To make sure it doesn't trip on whitespaces and comments, we simply ignore all non-important characters before doing a scan.

To handle keywords, we're also going to use a different approach this time. In Java, we used a hash map where each key was a keyword and its value was the token type. We would then check if a token is a keyword by simply looking it up in the hash map. Again, we don't have that luxury in C, and it is not necessary - looking up values in hash maps is actually pretty slow because we need to compute the hash and then probe the map until an appropriate bucket is found, which may involve comparing strings. In reality, we can check if a token is a keyword by first looking up its first character. If no keyword starts with this character, then it is not a keyword. Then we can check the second character, and keep repeating this process until we have a good estimate that this token may be a keyword. If there is a single keyword that starts with the exact same sequence of character, we do memory comparison. This allows us to make sure we only compare strings when it is really needed. The data structure that describes this approach is called a *trie*, but we implement it using nested switch statements.

**Key Points:**

* The compiler doesn't need to know the entire sequence of tokens at once, which allows us to scan the code on demand
* To recognize if a given token is a keyword we can check the first few characters and see if there are keywords that also start with the same characters before doing any memory comparisons

### Chapter 4: Compiling Expressions

Before moving on to compiling, the compiler itself first needs to be set up. Now that the code is scanned on demand, it is the compiler's responsibility to store the tokens it needs to keep track of. Because we have a LL(1) compiler, we only need to keep track of two tokens - the current and the next. Another major difference from jlox is in the product of the compilation - this time we're emitting bytecode instead of producing an AST, so the compiler must append bytes to a chunk of bytecode and then return it.

In jlox, we parsed the program using a recursive descent parser. This time, we're using a different technique - namely, a Pratt Parser. Whereas a recursive descent parser defines a single function for each production rule and handles precedence and associativity with a rule hierarchy, Pratt Parser takes a much simpler approach. It is recognized that each token may start an expression or be used in the middle of an expression, and the process of parsing each expression type is defined with a function. Each token may also have some precedence associated with it, which defines what sub-expressions it may contain. This information could be packed into a *parse rule*, because it defines how we want to parse expressions based on tokens they contain.

Thus, a Pratt Parser defines a table of parse rules, where each entry maps to a single token type in the language's grammar. Each entry contains function pointers to *prefix* and *infix* rules, as well as the precedence of the token type. The prefix rule gets invoked if an expression starts with this token type, and the infix rule gets invoked if the token type is found in the middle of an expression. The precedence is an ordinary numeric value. Because all prefix rules have same precedence in Lox, each entry only contains the infix precedence.

When parsing an expression of a certain precedence, the Pratt Parser looks up the current token type in the table. If the parse rule contains a prefix function, then that function is invoked. Otherwise no expression can start with this token type, so an error is thrown. After the prefix rule is handled, the parser then checks the next token at the point where the prefix rule left off. If the next token's precedence is higher or equal to the precedence of the currently parsed expression, its infix rule is invoked, otherwise the parsing terminates. This parsing process happens recursively, similarly to the recursive descent parser.

**Key Points:**

* Because scanning is done on demand, the compiler needs to keep track of the current and next tokens
* When compiling expressions, compiler emits bytes into a bytecode chunk
* A simpler approach to parsing is the Pratt Parser technique
* Each token type has a parse rule associated with it, which describes what type of expression it may begin or be used within, as well as its precedence
* The compiler defines a table of parse rules with an entry for each token type
* Parsing expressions is done by looking up the parse rule table and invoking the appropriate function
* Precedence and associativity is handled by giving each parse rule a numeric value and only allowing sub-expressions with equal or higher precedence value

### Chapter 5: Types of Values

Because Lox is a dynamic programming language, the language implementation needs a way to represent a value of arbitrary type, such as Boolean, nil, number, string, or some other object. In Java, we could use the Object class to treat all values identically and `instanceof` operator to determine types of values. C is not as lenient when it comes to types. However, the only reason C cares about them is to know how much memory it needs to allocate to store data. If we know the maximum size of any Lox value, the C compiler will be happy to let us interpret that data however we want. This lets us use unions to represent Lox values.

Plain union isn't enough, however, as we also need to determine the type of a value at runtime. This means we need a *tagged union*. Each Lox value will then consist of two parts: the *type tag*, represented by an enum, and the *payload*, represented by a union. For convenience, we also define a set of macros to check a value's type, convert Lox values to their C counterparts, and create new Lox values from C values. The list of macros will grow as the implementation evolves.

On the VM side, we create new instructions for pushing Boolean constants and `nil` onto the stack, since there are only 3 such constants. In fact, many VMs feature special instructions for quickly loading commonly used constant values, such as `0` or `1`. This is a simple optimization trick, but we're not making use of it for simplicity. When executing instructions, we now also need to assert the types of values we are operating on. To make sure we're not mutating the stack itself while performing validation, we add a new function to *peek* into the stack without popping the values. If operands are of invalid types, we throw a runtime error.

**Key Points:**

* In dynamic languages like Lox, variables may hold values of any type
* C disallows dynamic variables because the compiler must know how much bytes need to be allocated for each variable
* We may use tagged unions to emulate dynamic variables in C
* Instruction sets usually feature instructions for loading constants, such as Boolean `true` and `false`, `nil`, and common values like `0` or `1`

### Chapter 6: Strings

One last built-in data type in Lox is string. Unlike other types we've encountered so far, strings are dynamic - we don't know upfront how many characters a string will contain. This means we cannot store the string directly as a payload in a Lox value. A better approach would be to allocate an array of characters on the heap and then store a *pointer* to that array as the payload. Pointers are cheap, so we can freely store them on the stack, makes copies of them, and pass them arround in different ways. Reading a string is then as straightforward as dereferencing the pointer.

Strings, however, aren't the only type of objects in Lox. In the future, we will also be interested in functions, classes, instances, and the like. All of these will eventually live on the heap, and thus be accessed by pointers. To avoid some redundancy, we generally want to treat any object as a pointer. Because C doesn't support inheritance, there is seemingly no way for us to treat pointers to different types of objects in the same way. Luckily, there is a work-around in the form of *type punning*.

In C, each struct has an invariance that we can take advantage of - its fields are laid out in memory in the order of their declaration. C standard also guarantees that a struct will never have padding at the beginning of its memory. These two facts combine mean that, given a pointer to a struct, the pointer actually points to the struct's first field. To emulate inheritance, we could say that each *substruct* has, as its first field, its *superstruct*. Then we can access each substruct with a pointer of superstruct's type. To keep track of each object's type, we yet again store a type tag in the superstruct. That way, given a pointer to any Lox object, we can determine its type and dereference it appropriately. Yet again for convenience, we define a set of macros to check types, cast and dereference, and create objects.

The string object itself consists of a pointer to the character array and the length of the string. Creating a literal string is as straightforward as performing a direct copy of characters from the source string into a heap-allocated character array. Concatenating strings is slightly more complex because it involves taking in two strings, allocating a new string of their combined size, and then copying their contents over into the newly allocated string. In both scenarios we also make sure to append a null-terminator to make sure we can treat Lox strings as C strings for printing. One quick optimization we may also do is to use C's *flexible array member* feature, which allows allocating a struct of arbitrary size if its last member is an array of unknown size and treating the "trailing" bytes as part of the array. This lets us effectively allocate the entire string in one go, avoid unnecessary indirection, and increase chances of data locality.

Eventually we will add garbage collection to reclaim unused objects from the heap. To do that, we will need to keep track of every allocated object in the user's program. We're going to achieve this with an *intrusive linked-list*, meaning each heap-allocated object will store a pointer to the next object. We define a few macros to handle memory allocations and appending newly allocated objects to the linked-list. Once the program terminates, we traverse the list and deallocate each object.

**Key Points:**

* Strings are the first type of dynamic object that needs to be allocated on the heap and accessed via pointers
* In C, we can emulate inheritance by using type punning
* Strings literals are directly copied from the source string, whereas concatenated strings are copied from other strings
* To let ourselves traverse all allocated objects in the future, each allocated object stores a reference to the next allocated object

### Chapter 7: Hash Tables

The most crucial data structure for any dynamic language is a hash table. We've made extensive use of hash tables in jlox to look up variables, functions, classes, instances and the likes by name. In fact, even the scoping mechanisms were implemented with environments, which under the hood were simple hash tables. We're getting close to adding all of these, so we need hash tables available. However, now that we're in the C territory, we need to implement them on our own.

Hash tables are similar to arrays. They provide random access by index. The difference is in hash table's *definition* of index. Whereas in plain arrays elements are accessed by the numeric index that represents their offset from the beginning of the array, a hash table consists of a set of key-value pairs, where each element is a value that has a key attached to it. That key may be of any value, but what's more important is that, given a key, we can retrieve the value from the hash table practically instantly. For clox we're only interested in string keys, but the principle stays the same.

Under the hood, a hash table is simply a dynamic array. Each element of the array is called a *bucket*, since it may either be full or empty. When a key is passed into the hash table, it uses that key to calculate an index within the array. Because array access is by itself a constant time operation, we can presumably retrieve elements from the table in constant time.

To compute the index from a key, the hash table uses a *hash* function. A hash function takes in some arbitrary value and produces some numeric value. Good hash functions are capable of producing uniform values, meaning numeric values it produces will be spread along the entire range of available numbers. This, in turn, implies that we can effectively map any string to a roughly unique number. This number will be used as the index into the bucket array. Hash functions are also deterministic, meaning each input will always map to a single output.

However, we have no guarantee of the size of the produced number. A hash function will likely produce an integer whose value may be in the magnitude of billions, which is unlikely to be anywhere near the real size of the array. To solve the problem, we modulate the hash number by the size of the array to make sure it's always within valid range. This does create a trade-off - some strings will be pointing to the same index, since we're packing billions of hashes into a handful of array buckets. This is called a *hash collision*, and it's unavoidable due to the *pigeonhole principle*.

Thus, we want to make sure collisions happen rarely. The first step to that is a good hash function. The second step is making sure the array is growing as it gets full, and to make sure hashing remains somewhat optimal, we want to grow the table whenever it reaches a certain *load factor*, which is a metric that is equal to the total number of entries divided by the number of buckets. For example, a table with 5 entries and 10 buckets has a load factor of 0.5, or 50%.

Due to the *birthday paradox* we know that, as the capacity of a table increases, the chances of collision would increase rapidly as well. As such, there is no reliable way to avoid collisions entirely, which means they need to be handled. Various papers have been dedicated to the different solutions of resolving collisions, and numerious techniques have been invented, such as "double hashing", "cuckoo hashing", and "Robin Hood hashing". For clox we're using a simple one.

Instead of treating each bucket as a single value, we could treat it as a linked-list. If a collision occurs during writing, append an element to the end of that linked-list. Any time an element is accessed, walk through the list and compare the key of each element until a match is found, then return the found entry's value. This technique is called *separate chaining*. It's simple, but it converges into an ordinary array traversal under bad circumstances. In the *catastrophic* case every element will be placed into a single bucket. This is the reason hash tables practically don't guarantee constant time for every lookup.

Another technique is called *open addressing* or *closed hashing*. It is completely analogous to separate chaining, except the linked-list is integrated into the table itself. When writing to the table, check if the *preferred* bucket is empty. If it isn't, check other buckets until an empty one is found. The exact strategy for choosing other buckets is called *probing*. For our implementation we're going to use *linear probing*, meaning we will simply check subsequent buckets, and wrap around the table if we reach its end. Although this technique clusters the hash map, it is cache-friendly.

To implement the hash table, we're going to use a hash function called "FNV-1a". Because hash functions are deterministic and we are only using strings as keys, we're going to compute each string's hash during creation and cache it. This allows us to implement functions for writing, reading, and removing elements from the table. Reading is done by modulating the hash of the string and probing the table. When writing, we see if the new size will overflow the load factor and grow the table if needed, as well as rebuilding it to make sure each entry is hashed properly. A more difficult operation is removal of elements, because we need to make sure removing an element in the middle of some cluster won't terminate probing prematurely. It's like removing a node from a linked-list without re-connecting the broken parts.

To address this problem we use *tombstones*. A tombstone is a special sentinel value that is assigned to a bucket to mark it as half-dead, half-alive. During probing we treat a tombstone as if it was a living bucket, meaning we don't terminate if we encounter one. If we don't find an entry we were looking for, we simply return the first tombstone we encountered as the entry, thus recycling them for subsequent writings. Because we treat tombstones as full buckets, we don't decrease the running count of the table whenever we remove an element. However, when we resize our table, we fully reconstruct it from scratch, so it's completely safe to get rid of all the tombstones.

When comparing keys to resolve collisions, we could simply perform memory comparison between strings. This, however, is slow. A much more efficient approach would be to compare string pointers. This is only possible if we can guarantee that each string in memory is unique, and thus has a unique location. To achieve this, we effectively filter strings when creating them. This process is called *interning*, and it is done by creating a running hash set of strings, which allows us to quickly check by hash if a given string already exists. If we get a collision, we do a single memory comparison. From this moment onwards, we know that each string is unique, and as such we can compare them by simply comparing pointers.

**Key Points:**

* Hash tables are dynamic arrays with a different definition of an index
* Each element of the table is called a bucket
* When looking up a value by key in a hash table, we compute the hash of the key and modulate it to fit within the table's size
* Scenario where multiple keys refer to the same index in the table is called a hash collision
* Hash collisions are resolved with either separate chaining or open addressing
* Separate chaining lets each bucket store a linked-list of elements
* Open addressing integrates a linked-list into the table itself
* The simplest probing strategy for open addressin is linear probing, in which the hash table tries to read or write subsequent buckets
* When a hash table is resized, it needs to be rebuilt to make sure each entry is properly hashed
* When removing elements from a hash table, a special tombstone value is inserted to make sure probing is not terminated prematurely
* String interning is the process of de-duplicating strings at their creation, which guarantees that each unique string has a unique memory location

### Chapter 8: Global Variables

Global variables in clox are implemented identically to jlox - with a hash table. Whenever a variable declaration is encountered, an new entry is added into the globals table with the variable's name and value. Whenever a variable is encountered within an expression, we emit instructions for the VM to read or write that variable by name in the table. Because the VM needs to know the variable's name, which cannot fit into bytecode directly, we treat it as a constant and write it into the constant pool.

To recognize is a variable needs to be read or written, the parser looks for a `=` after variable name. However, assignment has precedence, and as such the parser needs to recognize cases when the assignment is not part of the expression. This lets the compiler determine invalid use cases of assignment and report them as errors. To do this, we pass in a boolean flag that specifies whether an expression allows assignment or not.

Now that variables are declared with statements, there is an important observation we need to make regarding stack semantics. Expressions produce values and statements consume these values to produce side effects. Because the program is comprised of statements, each expression is always within a statement, and as such, its value will always be consumed. This is called *stack effect*. A stack effect is a factor that defines how the stack height was modified after an operation was executed. In programming languages like Lox, all statements have total stack effect of zero, meaning the stack height doesn't change.

**Key Points:**

* Global variables are stored in a hash table
* Because variable names are strings, the compiler treats them as constants by writing them to the constant pool and emitting an appropriate index in bytecode
* When parsing assignment expressions, the parser needs to make sure the assignment is allowed due to precedence
* Stack effect is the delta in stack height caused by execution of some operation

### Chapter 9: Local Variables

Local variables could be stored in a hash table just like global ones, and then composed into a cactus-stack to account for scoping. However, clox already provides one stack that may be reused - the VM stack. As it turns out, local variables and scopes have stack semantics. When entering a new scope, we keep track of where it starts on the stack. Whenever a local variable is declared, it is "pushed" onto the stack. When exitting the scope, we pop every local variable that was declared there by moving back to where it started.

Due to the stack effect discussed earlier, we know that the stack won't contain any interfering temporaries between statements. This allows us to keep track of every local variable's exact location on the stack during compilation phase. Each local variable then only needs to know its identifier and the scope depth where it was declared. Whenever a local variable is declared, we add it into an array of local variables. To read a variable, we first resolve it by traversing that array backwards until we find the first match, thus accounting for shadowing, and then emitting its stack index, which the VM can then use directly. If a match is not found, we assume the variable is global and emit a different instruction. Whenever a scope is exitted, we count the number of variables declared in that scope and emit a pop instruction for each one.

**Key Points:**

* Local variables have stack semantics, which allows us to implement them with the VM stack
* Whenever a local variable is declared, its value is simply pushed onto the stack
* Due to stack effect, only variables are persistent, allowing the compiler to know their exact runtime locations within the stack
* When exitting a scope, the compiler counts the total number of local variables declared within it and emits a single pop instruction for each

### Chapter 10: Jumping Back and Forth

With the Virtual Machine of our own, we have enough control over program execution to implement control flow without relying on the JVM. The key to control flow is in the instruction pointer of our VM. If we modify the instruction pointer, execution effectively *jumps* to a different instruction. If we make these jumps conditional, we get control flow. For example, when dealing with an if-else statement, the VM simply needs to choose which branch to jump to. With loops, the VM jumps back to the beginning of the loop after each iteration, and then jumps past the entire body to exit when the loop condition isn't met. Logical expressions may skip some of their terms by jumping past them in case of a short-circuit.

We implement jumping relatively to the current instruction - as an offset. However, the compiler cannot know how far an instruction needs to jump before the code is fully compiled. Thus, we use a technique called *patching*, in which we first write a placeholder jump offset when the jump instruction is first emitted, and then once we know the exact offset we go back to that instruction and patch it with the correct value.

**Key Points:**

* Control flow is implemented by jumping between instructions in the program
* VM can modify the instruction pointer to jump between instructions, conditionally or unconditionally
* If-else statements are executed by jumping to the appropriate branch based on a condition
* Loops are executed by jumping back to the beginning of the loop after each iteration and jumping past the whole loop body if the condition isn't met
* Logical expressions are executed by jumping past terms on short-circuit
* Patching is the process of going back in bytecode and rewriting an operation or operands, which is particularly useful for jumping across unknown distances

### Chapter 11: Calls and Functions

We now need to figure out a mechanism for calling functions without relying on the JVM's callframe stack. In clox, each function is represented as a chunk of bytecode, name, and arity. Because the compiler has been implemented in a way that each compilation results in a single chunk, we can effectively chain compilers to compile individual functions, with the program itself being a top-level function. Each compiler is now also allowed to reserve the first slot of the function's array of locals for internal use. To differentiate between different types of function, we also create an enum which we assign to the compiler itself. To make sure we don't lose any of the outer compilers, we maintain an intrusive linked-list where each compiler has a pointer to its enclosing compiler.

When we finish compiling a function, we generate the function object and save it either in the globals table or locally on the stack, depending on the current scope. To reference a function, we simply emit an appropriate read instruction.

When calling a function, which may either be read like a variable or be produced by an expression, we need to let it allocate a block of memory where it can store all of its data, such as arguments and local variables. It turns out that functions have stack semantics, which means we can let functions put all of their data onto the stack. Similarly to how we handled scopes, we can simply pop all of its local variables after the call to a function is finished. However, so far the VM has relied on the fact that the compiler knows exact location of all local variables on the stack. This is not the case anymore, as functions may be called in different orders from different places and end up in totally different locations on the stack. In other words, function calls are not static.

All local variables are, however, always in the same spot *relative* to the function itself. This means that in the VM we now must define local variables relatively to the function that it belongs to. This is easy to achieve, as each function is compiled by a separate compiler, which gets its own empty array of locals. Each function call, however, needs to know where it starts on the stack. Hence, we introduce the idea of a *callframe*, which is a block of stack slots that is claimed by a function call. Each callframe has a pointer to its location on the stack, pointer to the function it executes, and instruction pointer into its function's bytecode. The VM then maintains a list of framecalls and always executes the current call's instructions. Calling a function is equivalent to pushing a new callframe onto the stack.

Arguments to functions are evaluated after the call to the function itself, meaning they always end up being the first values in the function's callframe. This matches well with the order in which the compiler compiles local variables and assigns their indices. To return a value from the function, the VM pops it off the stack, removes the callframe, and then pushes it back on. Thus each function call effectively resolves to a single value, be it implicit `nil` or something defined by user.

**Key Points:**

* Each function owns a chunk of bytecode which represents its body
* Function declarations produce function objects, which are then stored either in the globals table or on the stack like any other variable
* Retrieving a reference to a function is done by an appropriate read instruction on the VM
* Calling a function is equivalent to pushing a new callframe onto the VM stack
* Each callframe keeps track of where it begins on the stack, what function it executes, and where in that function's bytecode it is
* The VM now treats all variables as being relative to callframes, since even the program itself is treated as a top-level function

### Chapter 12: Closures

The current function-handling mechanism doesn't allow nested functions to refer to variables of their enclosing functions. Not only do they need to support that, functions also need to capture enclosing variables as closuers. Right now, whenever a local variable goes out of scope, it is instantly popped of the stack. For closusers, captured variables need to be persistent, and as such they should go onto the heap after being popped off the stack.

A suitable technique for this is known as *upvalues*. An upvalue can be thought of as an indirection - it's a value that holds a pointer to some location on the stack. Whenever we resolve a varible in an enclosing function, we create an upvalue with a pointer to the stack location where that variable existed at that moment. Each closure then maintains an array of upvalues, which is effectively the list of variables it captures. To make sure each call gets access to that array, we wrap each function in a closure.

Whenever we are resolving a variable within a function, we first see if it's local to that function, as usual. If it isn't, we go to the enclosing compiler and try to resolve it there. This process happens recursively, and we either resolve the variable in some enclosing function or assume that it is global. If we do find the variable, we return its local index and push it into the upvalue array. Note that the variable is pushed into each involved function's upvalue array, even if that function does not directly capture the variable. That way the current function's upvalue might not point to the value directly, but rather to an enclosing function's upvalue, hence the name. We also make sure to reuse same upvalues in case the variable has already been resolved previously.

After we're done compiling a function, we know exactly how many upvalues we need for a given function and we know their locations on the stack relative to the functions they were declared in. We may now emit instructions to let the VM know of the upvalues at runtime. Because each function may have an arbitrary amount of upvalues, the VM needs to be able to handle that well, so we make sure to emit the total number of upvalues found, and then emit the index of each upvalue. The VM also needs to know whether the upvalue points to the actual value or an enclosing function's upvalue, which we represent with a "local" flag in bytecode. The VM may then easily interpret all upvalue-related instructions by taking in the closure object and reading or writing its upvalue array at the specified local index.

When reading the value contained within an upvalue, we dereference its location pointer. As such, all we need to do to make captured objects persistent is redirect that pointer to wherever the captured variable exists after being popped off the stack. The smart trick here is that because the upvalue itself lives on the stack and only references a single variable, it may simply own the variable. The location pointer then simply redirects to the upvalue's own field where the variable is stored. That process is called *closing an upvalue*.

**Key Points:**

* When resolving a variable within a function, the compiler first tries to resolve it locally, otherwise it tries the enclosing compilers
* If a variable is found within one of the enclosing functions, it is captured and being written into an upvalue array of that function
* Each upvalue of a function is an object which stores memory location of a variable. Initially it points to the slot on the stack where that variable was first found
* Whenever a captured variable is getting popped off the stack, its value gets saved inside the upvalue itself, which redirects the location pointer to the appropriate own field

### Chapter 13: Garbage Collection

In such a simple and dynamic language like Lox, we don't want the users to bother with memory management. Instead, we are going to handle it for them using *garbage collection*. Garbage collection is the process of tracing the memory of the program and freeing blocks that are not in use anymore. To recognize which blocks aren't in use, the garbage collector makes a well-estimated guess - a piece of memory must be freed if there are no references left to it in the program. We can effectively say that the garbage collector simply forbids the program from leaking resources.

Numerous algorithms and techniques exist to implement garbage collection - for example, reference counting, Cheney’s algorithm, and the Lisp 2 mark-compact algorithm - but we're going to make use of the very first and simplest garbage collection algorithm - *mark-and-sweep*. The first stage of that algorithm involves going through the user's program and marking memory blocks that are still used. The second stage involves traversing the list of allocated memory blocks and freeing those that are not marked.

The VM has access to numerous sources of memory, such as the globals table, the stack, callframes, etc. Each of these is considered a *root*. Each root may also refer to other objects. For example, closures may reference upvalues, which may close over some heap-allocated objects. There is one type of roots that is special - interned strings. If we treated them as ordinary roots, we'd end up always marking each string in the program, thus no string would be ever freed. If we didn't treat them as roots, all strings would be instantly freed and thus the VM couldn't refer to them. Thus, we want to treat interned strings as *weak references*, meaning they are not roots on their own, but if something refers to them, we don't want to sweep these. We also need to perform a separate sweeping on the interned strings table to make sure it has no dangling pointers.

Thus, the VM goes through the *tri-color* marking process. Each block of memory is assigned one of three colors: *white* if it's not touched by the garbage collection scan yet; *gray* if it has been touched but hasn't been traversed yet; and *black* if it has been fully traversed. Initially, all memory blocks are white. As we go through each root of the memory *graph*, we make it gray and add it to the *worklist*. Then we go through each element in the worklist, make it black and traverse all of its internal references, recursively. By the time our worklist is empty, all blocks of memory are either black (marked) or white (not marked). Then happens the sweeping stage, where we traverse the linked-list of memory blocks, deallocating and removing all nodes that aren't marked.

The last question to asnwer is when and how often do we want to trigger the garbage collection. The sole reason we need garbage collection is to be able to reuse memory during allocations. Thus, we can collect garbage before each allocation. To understand how often we want to do it, we first need to get acquianted with two important metrics: *throughput* and *latency*.

Throughput is the relation between time spent executing user's code and scanning the program's memory. If a program ran for 10 seconds, but 1 second was spent scanning memory, the throughput would be 90%. In a language without garbage collection, throughput would always be 100%. That is optimal, but it is only possible when we manage memory ourselves, thus a garbage collector needs to be as close as it can be to 100%, but it will *never* reach that mark. To increase throughput, we want want to perform garbage collection as in-frequently as possible.

Latency, on the other hand, is a metric that measures how much time at most the garbage collector will need to perform a single garbage collection. If a program ran for 10 seconds and performed garbage collection 3 times that took 1/5, 3/5, and 1/5 of a second respectively, then the latency of the garbage collection is 3/5 of a second. To minimize latency, we want to perform garbage collection as frequently as possible.

Thus, both metrics have conflicting interests, and what's more, it is not possible to prioritize either of them, since throughput and latency play important roles under different circumstances. Many VMs allow users to setup the garbage collector the way that is most optimal for them, but any programming language deserves a good default behavior. Thus, for Lox, we use a decent approach with self-correcting garbage collector. We maintain a threshold of bytes that may be allocated before we collect garbage. Once that threshold is passed, we sweep memory and increase the threshold by some factor.

**Key Points:**

* Garbage collection is the process of deallocating unused memory blocks
* Each block of memory that the Virtual Machine has direct access to is called a root
* Roots may have references to other objects, which are also considered as used
* Throughput is the relation between time spent executing user's code and scanning the program's memory
* Latency is the most amount of time the garbage collector needs to execute

### Chapter 14: Classes and Instances

Each class in clox is represented with a heap-allocated object that stores a pointer to its name and a method table. Each class instance is also a heap-allocated object which stores a pointer to its class and a field table. Similarly to functions, class declarations produce new class objects and save them in the globals table or the stack, depending on the scope. Classes are also callable, in which case a new class instance is produced.

We also add get and set expressions that allow us to query fields of instances. These use the special `.` syntax. To execute these in the VM, we first emit the instance that we need to operate on and the name of the field we need. The VM can then simply lookup the field table of the instance and either read or write it, or throw a runtime error if something went wrong.

**Key Points:**

* Class declarations are handled by producing a class object and saving it in the globals table or the stack
* Calling a class results in producing a new instance of that class
* To query class fields, get and set expressions are defiend that use special `.` syntax
* As part of get and set expressions, the VM receives the instance to operate on and the name of a field. It then either reads or writes the instance's field table at that field's entry

### Chapter 14: Methods and Initializers

When compiling the body of a class, we first emit an instruction that creates the class itself. Then, compilation of each method emitts a closure object onto the stack with a special `OP_METHOD` instruction. Whenever the VM encounters that instruction, it appends the closure to the class's method table. To implement `this`, we do something similar to jlox. Each method uses its slot zero to store the receiver of the method. We manually assign name `this` to it, and whenever a bound method is called, we first set its value on the stack to be the receiver of the bound method. That way, it works like an ordinary function, but it has access to `this` as an ordinary variable reference.

To support initializers, we simply need to check if a method named `init` was defined by the user when producing a new instance of a class. If so, we call that method, which is guaranteed to produce a new instance. Otherwise, we manually create an empty instance. We perform some validation to make sure the user isn't trying to return something illegal from the initializer.

There is one optimization we can do. So far we've split method invocations into two parts - we first obtain a reference to the method, then call it. We do this because users are allowed to obtain references to methods without calling them. However, in the majority of cases, we actually want to instatly call the method. A very simple optimization we can do is fuse these two operations together and execute as a single instruction when the user instantly invokes a method.

**Key Points:**

* Class objects are created before their methods are compiled. As such, the VM mutates the class object's method table every time it encounters a new method instruction
* Each method uses slot zero to store a variable named `this`
* Before a method is called, its slot zero is bound to the instance on which it operates
* When creating a new class instance, the VM looks for a method named `init` and calls it if it's defined, otherwise it creates an empty instance manually

### Chapter 15: Superclasses

Because Lox classes are only allowed to inherit methods, the simplest way to allow that is to let a subclass simly copy over all of its superclass' methods during declaration. Because it's happening before the subclass' own methods are compiled, method overriding works correctly. In case a user wants to call superclass' method specifically, Lox supports `super` calls. To allow this, we need some way of remembering a class' superclass. The way we handled it in jlox was by creating a hidden environment during class compilation and capturing it inside a closure from each method. We're doing the analogous thing here by creating a hidden scope inside the class body and declaring a variabled named `super`, which will later be captured by a closure and closed by an upvalue.

Whenever we want to invoke a `super` method, we need two things: the receiver that we will be operating on, and the superclass that this method belongs to. Thus, we emit the values of `this` and `super` onto the stack as operands to `super` access. In the VM, we read the superclass, find the method we're interested it, bind the instance and invoke it. We also perform the same optimization we did in the previous chapter, namely, fusing invocations, only in this case we fuse together `super` access and method calls.

**Key Points:**

* Inheritance may be handled by simply copying over method pointers from the superclass' method table into subclass'
* Calls to `super` are handled via a variable declared in a hidden scope during class compilation, which is then captured by a closure and closed in an upvalue

### Chapter 16: Optimization

There is an issue with our current implementation of hash tables: we're performing modulo division to make sure the hash stays within the range of the table's buckets. Modulo division is a slow operation, and because we use tables all the time, it slows down our programs significantly. There is no way to perform modulo itself more efficiently, but as it turns out, we don't need to. Because we always grow the capacity of our hash tables by a factor of two, its capacity will always be some power of two. Taking a modulo of a power of two is as simple as returning all of the bits preceeding that power. In other words, we take the capacity, subtract one from it, and then perform a bitwise-and. Even better, we don't need to subtract one every time as we can simply cache that value. This optimization is very easy and surprisingly effective.

Another clever optimization has got to do with value representation. Currently, all values are stored in a tagged union. That's generally fine, but due to boundary alignment, the compiler adds padding that ultimately makes each value twice wider than it should be. To avoid this, we can use a technique called *NaN boxing*. Because doubles are floating point numbers, they obey the IEEE 754 specification. Each double thus consists of 3 parts: a single sign bit, 11 exponent bits, and 52 mantissa bits. Additionally, each double may also encode a special sentinel value - `NaN` (Not-a-Number).

`NaN` is represented by a double which has all of its exponent bits set, regardless of the sign bit and the mantissa bits. The reason this is important is because we can check the exponent bits at runtime to determine if a given value is a number or not. If it is, we treat it as a double. If it isn't, however, we can use the sign and the mantissa to store other types, like Boolean values, nil, and object pointer.

Booleans and nil are constants, so we simply reserve three bit patterns to represent each of them. Although pointers occupy 64 bits, they do so mostly for boundary alignment, whereas in practice they use at most 48, which can easily be stored in mantissa bits. To distinguish between object pointers and constant values, we may use the sign bit. The only thing left is to rewrite all value-conversion macros to take this representation into account. To be safe, we also leave the old implementations intact and allow switching between them with a macro.

**Key Points:**

* Although some operations cannot be performed more efficiently in general, in some cases we can cheat by making use of the context in which the operation is used
* Taking modulo of a power of two is an example of such operation, which lets us make hash tables more efficient
* NaN Boxing is a technique of packing multiple types of values into a single float or double through its sentinel `NaN` value
