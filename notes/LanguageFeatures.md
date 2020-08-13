# Language Features

## Abstract

This document contains various notes and thoughts regarding the set of new features for the toy programming language. This language isn't designed to be a real tool for writing applications, and as such it is rather simplistic. It draws inspiration from dynamically-typed, object-oriented scripting languages like Python, JavaScript, and Lua. As such, many of its features aren't novel in any sense, but the intent is to experiment with possibilities, as well as draw ideas from other paradigms.

## Features

### Operators

#### Exponentiation

A common task in many programs is to raise a number to some power. Very often the goal is to square a number - for example, when dealing with Pythagoras' theorem to calculate the hypotenuse of a right triangle. In such cases the normal solution is to manually performing squaring of a number by multiplying it with itself. This works well, but is rather cumbersome when dealing with expressions - the programmer either needs to type the whole expression twice, which may not be desirable if it produces side-effects or is expensive to compute, or store it in a temporary variable. Additionally, it is not generic enough to raise a number to any arbitrary power. To address this issue, many languages feature some sort of `pow` function, which takes in a base and an exponent and computes the power. Consider this example in C:

```cpp
double x = pow(2.0, 8.0); //Equals to 256
```

This approach is totally acceptable. In fact, the language already supports this function, and there is no reason to remove it, as it may be useful when emulating functional programming style. However, for such a common operation it would be desired to have a simpler solution. Drawing inspiration from Fortran, Python, and Haskell, the proposed solution is to implement the exponentiation operator, which is simply a syntactic construct that is equivalent to the `pow` function. The syntax looks like this:

```js
var x = 2 ** 8;             //Equals to 256
var y = (1 + 1) ** (2 * 4); //Equals to 256
var z = 2 ** 4 ** 2;        //Equals to 256
```

#### Compound Assignment

It is often desirable to perform an operation on a variable and some value and store the result back into the variable, like this:

```js
var x = 10;
x = x + 5;
```

In C-like languages, this is usually achieved with compound assigment operators, which is the proposed solution. The following are the possible compound operators:

```js
x += y;  //x = x + y
x -= y;  //x = x - y
x *= y;  //x = x * y
x /= y;  //x = x / y
x %= y;  //x = x % y
x |= y;  //x = x | y
x &= y;  //x = x & y
x ^= y;  //x = x ^ y
x >>= y; //x = x >> y
x <<= y; //x = x << y
x **= y; //x = x ** y
```

While the result of such operations is mostly equivalent to their non-compound counterparts, the evaluation of compound assignments are slightly different - the expression on the left is only evaluated once. Consider:

```js
class Foo {}

var foo = Foo();
foo.field = 10;
foo.counter = 0;

fun getFoo() {
    foo.counter += 1;
    return foo;
}

getFoo().field += 20;
```

In the above example, the value of `foo.counter` will be equal to `1`, as the `getFoo` call is only evaluated once. This wouldn't be the case if the same statement was written in the non-compound form.

#### Increment and Decrement

Under certain circumstances, the program needs to either increment or decrement a numeric value exactly by one. Like the majority of C-like languages, it is desirable to add support for both `++` and `--` operators, preferably in both postfix and prefix forms. Consider:

```js
var x = 10;
print x++; //Prints 10, x is now 11
print ++x; //Prints 12, x is now 12

print x--; //Prints 12, x is now 11
print --x; //Prints 10, x is now 10
```

#### Comma Operator

Many C-like languages feature the comma operator, which lets programmers insert multiple expressions where only a single is expected, discarding the results of all but the last last expression. For example:

```js
/*
 Because the comma operator has lower precedence than assignment, the same statement without parentheses would be
 interpreted as first assigning 10 to x and then producing 20 as the result of the entire expression, which is discarded
 by the statement.
*/
var x = (10, 20); //Equals to 20

/*
 This example is more useful, as it allows the programmer to control multiple variables on each iteration
 of a for-loop. In this instance, it iterates two variables: i from one to ten, j from ten to one.
*/
var i;
var j;
for (i = 0, j = 10; i < 10; i = i + 1, j = j - 1) {
    //...
}
```

#### Ternary Operator

It is often desirable to choose one of two values based on a condition. This could be achieved with an `if-else` statement, but it is rather clumsy. Additionally, it is a statement, meaning it cannot be used if an expression is expected instead. As such, many C-like languages feature a ternary operator, also known as conditional operator. It is proposed that this language implements it as well:

```js
var x = 10;
var y = x > 5 ? "Larger" : "Smaller"; //Equals to "Larger"
var z = y != "Larger" ? 10 : 3;       //Equals to 3
```

#### Safe Navigation Operator

One of the most annoying issues to deal with in any programming language is the `Null Pointer Exception`, which occurs whenever the program attempts to somehow use an object which has `null` value. The best (as in the worst) example of when this becomes a problem is whenever the program needs to chain field access on an object:

```js
var x = foo.bar.baz.qux.getValue();
```

However, consider what happens if one of these fields, for example `baz`, is `nil`. The program would try to access field `qux` of `nil`, which will fail. In a dynamic language like this, there are many dangerous spots like these. For example, the field may not be an object, or it may not exist at all. These cases are related, and the language needs to ensure programs may run safely even if an error like this occurs. Consider what we would have to do to make sure that none of these fields are `nil` when dereferncing them:

```js
/*
 You may argue that this example is slightly contrived. Indeed, the same result could be achieved using the short-circuiting
 mechanism of boolean operators, i.e.

 if (foo != nil and foo.bar != nil and foo.bar.baz != nil and foo.bar.baz.qux != nil) {
     x = foo.bar.baz.qux.getValue();
 } else {
     x = nil;
 }

 However, this may not always be desirable as we evaluate the same expressions multiple times. Consider if instead of
 accessing `bar` directly we called a function which returned it. If that function had a side-effect, the behavior
 of the program would end up being very different.
*/
var x = foo;
if (x != nil) {
    x = x.bar;
    if (x != nil) {
        x = x.baz;
        if (x != nil) {
            x = x.qux;
            if (x != nil) {
                x = x.getValue();
            }
        }
    }
}
```

Notice the pattern: we go step at a time and assign a single value to `x` on each step, then check if the newly assigned value is `nil`. If it is,we stop going, leaving the x with value `nil`. Otherwise we eventually assign the desired value from the method `getValue` to it. It's really clumsy to type all of this out, however. In a functional language like Haskell, this problem would be solved with a Maybe Monad. This language doesn't support monads (at least not yet), but there is a simpler solution, inspired by Kotlin. Consider this:

```js
var x = foo?.bar?.baz?.qux?.getValue();
```

Functionally, this is equivalent to the Maybe Monad. The Safe Navigation operator `?.` returns the accessed field on the object if it is not `nil`, otherwise it returns `nil`. As such, if `bar` is `nil`, that value will get propagated to `baz`, then to `qux`, and the whole expression will evaluate to `nil`. The same operator could potentially be applied to any case where the field is not accessed on a valid object, thus preventing the program from crashing in case the programmer does not know upfront which values exist and which don't.

It may rightfully be stated that this behavior could be the default of any field access. That assessment is fair, but it would move the language closer towards the world of JavaScript, where nothing can really crash but the behavior of the program is unpredictable. In many cases accessing a non-existent field is, in fact, a mistake in the logic and must be reported to let the programmer find the issue. In some cases like the one above, however, it may be a perfectly valid use-case.

### Control Flow

#### Continue and Break

Many languages support mechanisms for stopping loops or skipping certain iterations, usually via keywords `break` and `continue`, respectively. The same mechanisms are proposed for the language as well:

```js
/* This loop skips the fifth iteration and never prints 5 */
for (var i = 0; i < 10; i = i + 1) {
    if (i == 5) {
        continue;
    }

    print i;
}

/* Breaking allows the program to exit infinite loops based on some internal conditions */
var x = 10;
while (true) {
    if (x > 20) {
        break;
    }

    x = x + 1;
}
```

#### Switch Statement

To avoid scenarios where the program needs to choose a single condition out of many using a sequence of `if-else` statements, many programming languages feature `switch` statements. The language could support simple switch cases with the following syntax:

```js
/* Prints "Work day" */
var x = "Monday";
switch (x) {
    case "Monday", "Tuesday", "Wednesday", "Thursday", "Friday" -> {
        print "Work day";
    }
    case "Saturday", "Sunday" -> {
        print "Off day";
    }
    default -> print "Invalid day of the week";
}
```

### Data Structures

#### Array

The most basic and fundamental data structure, a fixed-size array. Static programming languages normally provide only this data structure as built-in because it is trivially simple and the information about its size can be used during compilation to simplify memory allocations. However, scripting languages are usually interpreted and as such often don't benefit from these factors, as it is much easier to implement more advanced data structures straight away. On the other, arrays are fairly fast as they don't add overhead of dynamic reallocations of memory. This language is aimed more towards ease of use than performance, but it could be beneficial to provide a way of using a fixed-size array to make programs faster. The proposed solution is to add an Array class with indexing syntax, like the following:

```js
var x = Array(5); //x is [nil, nil, nil, nil, nil]
x[0] = 10;        //x is [10, nil, nil, nil, nil]
x.set(1, 11);     //x is [10, 11, nil, nil, nil]

x[-1] = 20;       //x is [10, 11, nil, nil, 20]
x.set(-2, 19);    //x is [10, 11, nil, 19, 20]

x[0];             //Equals to 10
x.get(0);         //Equals to 10
x.length();       //Equals to 5
```

Accessing an element out of array's range results in a runtime error.

#### List

List is a very popular data structure, which can be used to produce behavior of other data structures as well. A list is a dynamic array, meaning it is a contigious block of memory that may be accessed randomly, but unlike ordinary array, it can grow and shrink in size. A list can be used as a stack or a queue by only adding and removing elements from the ends of the list.

Technically, a list could be implemented from an array without built-in support from the language. However, the implementation would likely be less efficient than a foreign one, as the language does not provide any mechanisms for reallocating memory in a fast manner. Additionally, the standard library is not developed enough yet to support native implementations of data structures.

As such, the inspiration is drawn from dynamic scripting languages like JavaScript, Python, and Lua. The following is the proposed syntax for lists:

```js
var x = [1, 2, 3, 4, 5]; //x is [1, 2, 3, 4, 5]
x.append(6);             //x is [1, 2, 3, 4, 5, 6]
x.remove(0);             //x is [2, 3, 4, 5, 6]

x[0] = 6;                //x is [6, 3, 4, 5]
x.set(0, 7);             //x is [7, 3, 4, 5]

x.length();              //Equals to 5

var y = x[0];            //Equals to 2
x.get(0);                //Equals to 2

var z = x[-1];           //Equals to 6
x.get(-1);               //Equals to 6
```

Various other methods should be added to this to make the data structure more flexible, such as insertion of elements into arbitrary positions and copying elements from other lists.

Accessing an index out of the list's range results in a runtime error.

#### Hash Map

Another really common data structure is an associative array. It has many names, but the proposed name is "hash map", taken from Java. This may arguably be a poor decision because it relies on the implementation details unlike "dictionary" or "table", but it describes the idea fairly well. Drawing inspiration from languages like JavaScript and Python, the following is the proposed syntax:

```js
var x = {
    "A": 1,
    "B": 2,
    "C": 3
}

x["A"];             //Equals to 1
x.get("A");         //Equals to 1

x["D"] = 4;         //Equals to 4, x is now { "A": 1, "B": 2, "C": 3, "D": 4 }
x.set("D", 5);      //Equals to 5, x is now { "A": 1, "B": 2, "C": 3, "D": 5 }

x.containsKey("B"); //Equals true
x.containsKey(5);   //Equals false

x.remove("B");      //x is now { "A": 1, "C": 3, "D": 4 }
```

Non-homogeneous keys are allowed, as well as nested maps. Accessing a non-existen key results in a runtime error.

### Strings

#### Char

Currently, the language only supports numbers, Booleans, strings, and nil as built-in data types, apart from more advanced objects like functions and class instances. Strings allow users to group together character, but there is no way to refer to individual characters. The only way to create a single character is through a string, which is also non-efficient. To avoid this, the proposed solution is to add support for `char` data type with the following syntax:

```js
var x = 'a';    //This is a single character
var y = "abc";  //This is a string

x = x + "bc";   //Equals to "abc", as characters are allowed to be concatenated to strings
y = y + "de";   //Equals to "abcde"

y[0];           //Equals to 'a'
y.get(0);       //Equals to 'a'

y[-1];          //Equals to 'e'
y.get(-1);      //Equals to 'e'

y.[0] = 'w';    //y is now "wbcde"
y.set(0, 'q');  //y is now "qbcde"

y.length();     //Equals to 5
```

The string can now be seen as an array of characters with appropriate syntax. Various functions may also be added that operate on individual characters, such as `toUpperCase` and `toLowerCase`, `isAlpha`, etc.

#### Escape Characters

Certain characters should have special meaning in string literals. For example, the newline character is denoted as `\n`, tabulation is `\t`, etc. As such, the proposed syntax is fairly standard:

```js
/*
 Prints:
 Hello
 World
*/
print "Hello\nWorld!";

/*
 Prints:
 Hello    World!
*/
print "Hello\tWorld!";

/*
 Prints:
 Hello\\nWorld!
*/
print "Hello\\nWorld!";

/*
 Invalid escape character
*/
print "Hello\World!";
```

#### To String

There should be a support for converting various data types to strings. This functionality is already handled for built-in types, but classes should be able to define a `toString` method which will be called whenever an object has to be converted to a string, either directly or indirectly. Each class also should have a default `toString` implementation which simply prints out its contents in the form `{fieldA = value, fieldB = value, ...}`.

```js

class Foo {

    init(bar, baz) {
        this.bar = bar;
        this.baz = baz;
    }

    toString() {
        return "This Foo object has bar of '" + this.bar + "' and baz of '" + this.baz + "'";
    }

}

print Foo(1, true); //Prints "This Foo object has bar of '1' and baz of 'true'"
```

#### String Interpolation

The proposed way of concatenating strings with values of various types is through string interpolation, which allows for any arbitrary expressions, even nested interpolations. The following is the fairly standard proposed syntax:

```js
var x = 10;

/* Prints "The value of x is 10, which is larger than 9 by one. */
print "The value of x is ${x}, which is larger than ${x - 1} by one.";

/* Prints "Hello World! */
print "Hello ${"World"}!";

/* The dollar sign is now considered a special character, meaning it has to be escaped in other cases */
print "\$50";
```

### General

#### Function Expressions

As a language with functions as first-class citizens, it is obligatory that the language has to support anonymous functions, or lambdas. The following is the proposed syntax for them:

```js
var func = fun(a, b) {
    return a + b;
};

print func(2, 5); //Prints 7

fun apply(f, x) {
    return f(x);
}

print apply(fun(x) { return x ** 2; }, 5); //Prints 25
```

#### Constant Values

Programs often deal with scenarios where a certain value needs to be constant accross the entire execution of the program. This is becoming increasingly popular as of late, such that languages like Kotlin assume that idiomatic code always makes variables constant unless it is explicitly needed to be otherwise. As such, it would be beneficial to add constants into the language. Following Kotlin's footsteps, this is the proposed syntax:

```js
val pi = 3.1415926535;
pi = 3; //Illegal operation, this should preferably be a compile-time error
```

#### Static Fields and Methods

A very useful feature of classes in many object-oriented languages is the ability to provide functionality without relying on any particular instance. This serves mainly as a namespacing mechanism, but it has an advantage of being able to access the class' static fields as well. As such, it would be beneficial to implement statics into the language. The following is an example:

```js

class Calculator {

    //Called once after class declaration is finished executing, does not interfere with ordinary `init` constructor
    static init() {
        Calculator.pi = 3.1415926535; //Static field
    }

    static circleArea(r) {
        return Calculator.pi * r ** 2;
    }

}

print Calculator.circleArea(1); //Prints 3.1415926535
```

#### Error Handling

It is very important for the language to be able to handle runtime errors well, as often it is really difficult or purely impossible to recognize if something will go wrong upfront. Languages like C fail silently and delegate the responsibility of checking validity of operation outputs to the programmer. This is a possible approach, but in certain cases it may be difficult to use as it is difficult to come up with a suitable "identity" value representing invalid state of the output. In addition, the language already throws various runtime errors under certain circumstances. As such, it would be desirable to have some notion of a `try-catch` block, like this:

```js
//Prints "Something went wrong!"
try {
    10 / 0;
} catch {
    print "Something went wrong!";
}
```

The specifics are not yet recognized, but this approach would require a formal definition of an `Exception` and an ability to handle specific exceptions.

#### Modules

Modularity is a fairly important concept for any programming language. It is useful both for breaking down a program into various files that are responsible for providing different capabilities, functions, and data types, as well as to let the standard library provide native implementations that can be imported from elsewhere. The proposed solution is to go with Python's modularity model, which is well-suited for dynamic scripting languages. Whenever an `import` statement is encountered, it executes a specified path. To avoid circular dependency loops, the program is only allowed to import each file once. Consider the following setup:

```js
/* File TestA.lox */
print "Hello from A";
```

```js
/* TestB.lox */
import TestA

print "Hello from B";

/* Output:
   Hello from A
   Hello from B
*/
```

## Applications

### Interactive

The language implementation should support interactive mode in which it executes statements one by one, as they are provided via the command-line. In this mode, the implementation should work in a slightly different way from the normal execution mode - for example, an ordinary expression statement should be evaluated and printed to the command-line by default, whereas all other statements should be executed properly. In addition, the mode should also support special commands for quitting, printing help, etc.

### Command-Line

It should be possible to launch a script via command line and pass in arguments that the script can then access. This would allow the language to be used for more realistic scripting tasks.

### Embedded

The main goal of this scripting language is to be embedded within a different program, likely written in C or C++. Specifically, this language is aimed to be used to write scripts for a toy game engine later on. As such, it is desirable that the implementation can be used as a library.
