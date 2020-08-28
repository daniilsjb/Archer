# Language Features

## Abstract

This document contains various notes and thoughts regarding the set of new features for the toy programming language. This language isn't designed to be a real tool for writing applications, and as such it is rather simplistic. It draws inspiration from dynamically-typed, object-oriented scripting languages like Python, JavaScript, and Lua. As such, many of its features aren't novel in any sense, but the intent is to experiment with possibilities, as well as draw ideas from other paradigms.

## Features

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
