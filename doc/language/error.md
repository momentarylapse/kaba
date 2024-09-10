# Error handling

## No value

This is the most basic error.

An operation might "fail" and not produce a resulting value (like reading from a missing file). A function could indicate this by returning an [optional](optionals.md) `X?`.

Accessing a [dictionary](dicts.md) `dict["key"]` for example returns an optional because the key might not exist.

Similarly, a raw [pointer](pointers.md) `X*` can be `nil`. This could be used to indicate the failure to find a requested object.

### Unwrap

Kaba protects against usage of optionals or raw pointers without values, by forcing implicit checks and unwrapping the value:
```kaba
var optional: i32?
for i in optional
    print(i + 5)
else
    print("we failed!")
```

### Definitely

Alternatively we can simply promise that a value is present and use it:
```kaba
var optional: i32?
print(i! + 5)
```

🔥 This will secretly perform a check. And if no value is present, the program will terminate with an error message.

This is useful for keeping programs easy to read. Especially for scripts, where terminating with an error is the most reasonable error handling.

### Local exceptions

When dealing with multiple uncertain operations, when you still want to _catch_ errors, use `try`:
```kaba
try
    print(i! + 5)
    var a: any
    print(a["key1"]!["key2"]!)
except
    print("nope")
```

🔥 This is not a full exception system! Errors can not be _caught_ across function boundaries!

### No checks at all

If you are reeeeeally sure that there will be a value, you can skip the checks via:
```kaba
trust_me
    print(i! + 5)
```

🔥 What if there is no value? ...that's your problem!

### Pipelines

You can also use [pipelines](pipes.md) to operate on values, if they exist, while bypassing none-values:
```kaba
func f(x: i32) -> string
func g(x: string) -> f32

var optional: i32?
let out = optional |> f |> g   # results in f32?
```

## Exceptions

They exist, but are discouraged.

