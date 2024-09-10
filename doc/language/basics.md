# Basics

Some python inspiration:
* Comments start with `#`
* Indentation with tabs defines blocks

Some c++ inspiration:
* Strongly typed
* Compiled and executed as machine code

Some other inspirations:
* Things are mostly immutable by default

## Main

The program start by executing a `main()` function:
```kaba
func main()
    print("hi")
```

This can optionally take command-line arguments:
```kaba
func main(args: string[])
    ...
```

## Variables

The common basic types are:

| Type     | Meaning                                            |
|----------|----------------------------------------------------|
| `i32`    | (32bit) signed integers                            |
| `f32`    | (32bit) floating point numbers                     |
| `string` | [strings](strings.md) ("text") of arbitrary length |
| `bool`   | boolean truth values, can be `true` or `false`     |

Less common are other signed integers (`i64`, `i16`, `i8`), unsigned 8bit integers (`u8`), 64 bit "doubles" (`f64`).

Defining a variable:
```kaba
# Common usage
var i = 13

# Or, more verbose
var i: i32 = 13

# i is mutable
i = 20
```

🔥 Variables can change values, but they can never change their type!

If the variable does not need to be mutable, use `let`:
```kaba
let i = 13
let f = 1.25
let s = "hi"
let b = true
```

## Basic control flow

Conditions:
```kaba
if i > 10
    print("big!")
else
    print("small")
    print("...")
```

Basic loop:
```kaba
for i in 0:5
    if i == 3
        continue
    print(i)
    if i == 1
        print("...")
# output:
# 0
# 1
# ...
# 2
# 4
```
Start at 0, end BEFORE 5. Respects `continue` and `break`.

## Boolean logic

Boolean operators are words:
```kaba
let a = true
let b = false
let c = a and b
let d = not (b or c)
if d
    ...
``` 

## Funky stuff

🔥 Blocks can have a value!

Might not sound impressive, but you can write
```kaba
let i = if condition
    13
else
    99
```
(both branches of `if` need to be of the same type)
