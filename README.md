# Archer

Archer is a toy programming language based on Lox, the language presented in Bob Nystrom's [Crafting Intepreters](https://www.craftinginterpreters.com/). I wanted to expand on the features that were covered in the book, and my attempts led me to this little language.

Initially, I did not plan to make this code public, as it is more of an experimental playground than a proper implementation, with the code turning out to be rather messy. Nonetheless, this project contains a set of working features that I am proud of.

## The Language

The following is a simple Archer program:

```kotlin
var phrases = [
    ("Hello", "World"),
    ("Well met", "stranger"),
    ("Greetings", "Archer")
];

for (var |greeting, name| in phrases) {
    print "$greeting, $name!";
}
```

For more examples, see the `lang-tests` and `lang-benchmarks` directories. For additional information on the language syntax and grammar, see `lang-ref`.

## Features

Although this is only a toy language, it supports a decent range of features for a modern scripting language. On top of the ones inherited from Lox, such as closures and simple class system, Archer supports:

* Collections
* Lambdas
* Function expressions
* String interpolation
* Tuples and unpacking
* Simple iterators and ranges
* Null safety operators
* Static class fields
* Coroutines
* Modules

Some features are not fully complete. For instance, null safety doesn't work with compound assignments, and the iterator protocol is very basic. Quite a few syntax decisions were made hastily, as much bigger priority was given to testing out ideas - coroutines are a good example of that. Still, there was a lot of work involved in implementing these, and I hope that some people may find it interesting.

For more details, see example programs.

## Building

If you'd like to give Archer a try, building it is as straightforward as running CMake, since there are no external dependencies:

```shell
mkdir build
cd build
cmake ..
cmake --build .
```

Once the build went successfully, run the resulting executable from command prompt and type the following:

```kotlin
print "Hello, World!";
```

The interpreter should simply print the line `"Hello, World!"` back to you.

## Plans

As fun as this project was, I do not plan on continuously working on it. However, I hope to take the experience I've gained from this simple language and use it to design and build a better one, applicable to real projects.
