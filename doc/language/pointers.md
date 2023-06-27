# Pointers

There are several pointer types:

* `X*` - raw pointers
* `X&` - references
* `owned[X]` - single owner
* `owned![X]` - single owned, not null
* `shared[X]` - shared ownership
* `shared![X]` - shared ownership, not null
* `xfer[X]` - ownership transfer

## Pointers as containers

Pointers that can be null (`X*`, `owned[X]`, `shared[X]`) need additional unwrapping when using the "contained" value (i.e. the object pointed to by the pointer):
```kaba
var pointer: shared[X]
for x in pointer
    x.do_something()
else
    print("null pointer...")
```

This ensures, that no null pointers are dereferenced.

The extracted value is of reference type `X&`.

## References

References (as well as not-null-pointers `owned![X]`, `shared![X]`) will be dereferenced automativally:
```kaba
var pointer: owned![X] = new X()
pointer.do_something()

var x: X
var r: X& = &!x
r.do_something()
```

References should be seen as a temporary view, always pointing to a value.

## Single ownership

Only owned and shared pointers take ownership.

For `owned` that means, destroying or altering the pointer will delete the pointed to object:
```kaba
func main()
    let x = new X()
    # by default, x has type owned![X]
    ...
    x = new X() # deletes the old object
    ...
    # here `x` goes out of scope, also deleting the new object
```

When there is an `owned[X]`, no other pointer with ownership can exist simultaneously. Ownership has to be transferred:
```kaba
var a: owned[X] = new X()
var b: owned[X]
b = give(a)
# now, a is null
```

Here `give(a)` clears the pointer `a` and returns a transfer pointer of type `xfer[X]`. `b` then takes the ownership.

Likewise, ownership has to be transferred in and out of functions:
```kaba
func ownership_taker(x: xfer[X])
    var p: owned[X] = x

func ownership_giver() -> xfer[X]
    var p = new X[]
    return give(p)

ownership_taker(ownership_giver())
```

Btw. `new X()` returns a type `xfer[X]`. But since no variables of type `xfer[X]` are allowed, new variables default to `owned![X]`:
```kaba
var p = new X()
# -> owned![X]
```

## Shared ownership

Similar to `owned[X]`, `shared[X]` also owns an object. Although multiple `shared[X]` pointers can point to the same object. The object is deleted, when no shared pointer points to it anymore:
```kaba
var a: shared[X] = new X()
var b = a
# now, both a and b point to the same object
a = nil
# object still alive
b = nil
# object is deleted
```

`owned` and `shared` should not be mixed for the same object! Future plans for the language include more safe guards.

## Recommendations

When storing data, use `owned(!)[X]` and `shared(!)[X]`:
```kaba
struct MyData
    a: owned[X]
    b: shared[X][]
```

Functions should indicate ownership. If ownership is fully taken/given, indicate with `xfer[X]`:
``` kaba
func taker(p: xfer[X])
    some_owner = p
    # this is now the only owner

func giver() -> xfer[X]
    return new X()
    # caller is the only owner
```

Shared ownership via `shared[X]`:
```kaba
func taker(p: shared[X])
    some_shared = p
    # caller might sill own, too

func giver() -> shared[X]
    return x
    # caller might not be the only owner
```

