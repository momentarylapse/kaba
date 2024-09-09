# Strings

## Basics

The type of all text is the `string`. It is `u8[]`, i.e. a dynamic array of 8-bit bytes, and always assumed to be utf-8.

```kaba
var s = "hi äö"
```

## Interpolation

Variables (and more complex terms) can be inserted into strings:
```kaba
let i = 13
print("i has value {{i}}")

# => "i has value 13"
```

Some basic formatting options exists:
```kaba
"bla{{pi|8.2}}"
# right-aligned, occupying 8 characters, with 2 digits after the decimal dot 
# => "bla    3.12"
```
