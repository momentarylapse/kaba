# Functions

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
