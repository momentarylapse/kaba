# Dictionaries

Type `X{}` maps [string](strings.md) keys to values of `X`.

```kaba
let dict = {"a": 13, "b": 20}
# type: i32{}
# maps "a" to 13 and "b" to 20
```

🔥 Access might fail, if a key is not present. That's why [optionals](optionals.md) `X?` are returned and need to be unwrapped before usage (see [error handling](error.md)):
```kaba
for value in dict["a"]
    print(value)
```


