# Language Features

## Abstract

This document contains various notes and thoughts regarding the set of new features for the toy programming language. This language isn't designed to be a real tool for writing applications, and as such it is rather simplistic. It draws inspiration from dynamically-typed, object-oriented scripting languages like Python, JavaScript, and Lua. As such, many of its features aren't novel in any sense, but the intent is to experiment with possibilities, as well as draw ideas from other paradigms.

## Features

### General

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

#### Constant Values

Programs often deal with scenarios where a certain value needs to be constant accross the entire execution of the program. This is becoming increasingly popular as of late, such that languages like Kotlin assume that idiomatic code always makes variables constant unless it is explicitly needed to be otherwise. As such, it would be beneficial to add constants into the language. Following Kotlin's footsteps, this is the proposed syntax:

```js
val pi = 3.1415926535;
pi = 3; //Illegal operation, this should preferably be a compile-time error
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

## Applications

### Interactive

The language implementation should support interactive mode in which it executes statements one by one, as they are provided via the command-line. In this mode, the implementation should work in a slightly different way from the normal execution mode - for example, an ordinary expression statement should be evaluated and printed to the command-line by default, whereas all other statements should be executed properly. In addition, the mode should also support special commands for quitting, printing help, etc.

### Command-Line

It should be possible to launch a script via command line and pass in arguments that the script can then access. This would allow the language to be used for more realistic scripting tasks.

### Embedded

The main goal of this scripting language is to be embedded within a different program, likely written in C or C++. Specifically, this language is aimed to be used to write scripts for a toy game engine later on. As such, it is desirable that the implementation can be used as a library.
