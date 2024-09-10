# Kaba

Kaba is a general purpose programming language. The core is a simplified version of c++, with some python-style syntax sugar.

Features:
* it __compiles down to machine code__ - x86 64/32 bit or aarch64 - binary compatible with c++ (at least g++, clang and msvc)
* __JIT compiler__ - can be used for plugin systems, interacting with a c++ host program
* fairly __clean syntax__ - lists, optionals etc. directly integrated into the language
* some __safety features__ - value semantics - less mutable data by default - guards against usage of null values

This is the stand-alone compiler/execution environment for running scripts/applications.

## Syntax

Uses indentation for blocks:
```kaba
func main()
    print("hi")
```

Strong typing system, with easy built-in containers:
```kaba
func f(i: i32) -> i32[]
    return [i,1,2,3]
```

Some functional ideas:
```kaba
let x = [1.0, 2.0, 3.0] |> sin
                        |> filter(x => x>0)
                        |> sort
```



* [basics](doc/language/basics.md)
* [arrays](doc/language/arrays.md)
* [strings](doc/language/strings.md)
* [optionals](doc/language/optionals.md)
* [dictionaries](doc/language/dicts.md)
* [functions](doc/language/functions.md)
* [classes](doc/language/classes.md)
* [pointers and ownership](doc/language/pointers.md)
* [data pipelines](doc/language/pipes.md)
* [error handling](doc/language/error.md)
* [async](doc/language/async.md)
* [generics](doc/language/generics.md)
* [modules](doc/language/modules.md)

## Libraries

* user interface (gtk backend)
* OpenGL
* vulkan
* threads
* math (3d linear algebra, fft)
* networking

For a full reference, see: https://wiki.michi.is-a-geek.org/kaba.reference/

## Building

* [how to build and run](doc/how-to-build.md)


