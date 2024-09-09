# Optionals

An optional type `X?` can either contain a value of type `X` of no value, i.e. `nil`.

## Usage

```kaba
var optional: i32?   # no value, by default
optional = 13
optional = nil
```

🔥 Code can not refer to values contained in an optional directly, since it might not be present. Instead, we need to "unwrap" it safely:
```kaba
var optional: i32?
for i in optional
    # i is the contained i32
    print(i + 20)
else
    print("no value")
```

Optionals can also be processed by [data pipes](pipes.md), i.e. get unwrapped, transformed and wrapped again:
```kaba
(2.0 as f32?) |> sin
```

## Definitely

🔥 We can also insist to use the value and risk a failure:
```kaba
print(optional! + 5)
```
This will exit the program with an error message, if the values is not set.

We can also "catch" errors, and keep the program running:
```kaba
try
    print(optional! + 5)
except
    print("...failed")
```
🔥 This is not a full exception system! It is simpler/faster, but exceptions can not pass function boundaries!

If we feel particulary sure of our code, we can even do this:
```kaba
trust_me
    print(optional! + 5)
```
🔥🔥 No error checking! Might have freaky consequences, if no value is set!
