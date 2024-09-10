# Functions

## General

Basic parameters:
```kaba
func f(a: i32, b: f32, c: string)
    # parameters: a, b, c
    print("hi...")

f(1, 2.0, "something")
```

The return type (if present), uses an arrow `->`:
```kaba
func square(i: i32) -> i32
    return i^2

let x = square(5)
```

🔥 By default, all parameters are immutable!

🔥 The compiler automatically passes larger objects by reference. We don't lose performance!

🔥 This is "mostly safe" thanks to the immutability. ...TODO describe plans for memory safety...

If you need, you can add "output" parameters:
```kaba
func f(out i: i32)
    i = 13

var i = 0
f(i)
# now, i is 13
```

## Function pointers

The type of function pointers for functions `func ...(a: X) -> R` is `X->R`. We have `(X,Y,Z)->R` for functions with 3 parameters and `void->void` for functions without parameters or return value.

Mostly useful for callbacks:
```kaba
func do_something(f: i32->void)
    f(13)

func f1(a: i32)
    print(a)
func f2(a: i32)
    print("nope")

do_something(f1) # -> 13
do_something(f2) # -> "nope"
```

## Lambdas

Functions can be defined within the context of other functions without an explicit name. To sound more sophisticated, they are called "lambda" functions.

```kaba
func f()
    var x = 23
    do_something(func(a: i32) print(a + x))
```

Or more verbosely:
```kaba
func f()
    var x = 23
    let l = func(a: i32)
        print(a + x)
    do_something(l)
```

In this one-line syntax, there is an automatic return statement and type. E.g. `func(x: f32) x^2` returns the square.

🔥 The tricky part: they can access the surrounding function's local variables!

🔥 Sounds simple enough, but it can be very messy! What if `do_something()` stores the lambda and it will only be called later, when `f()` is already finished and `x` is gone?!?

🔥 Small/simple variables will be copied (so, no problem, but maybe not what you wanted). Complex variables (like `self`) will only be references. Yes, it is bad!

As a side effect, member callbacks are very easy to write:
```kaba
class X
    func __init__()
        do_something(f)  # will actually encapsulate the whole call to self.f()
    func f(a: i32)
        ...
```
