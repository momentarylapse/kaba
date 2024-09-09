# Generics

Kaba has (limited) support for generic programming.

## Templated functions

A function squaring arbitrary data types would be:
```kaba
func square[T](x: T) -> T
    return x*x
```

Extracting the first element of an array (without safety):
```kaba
func first[T](array: T[]) -> T
    return array[0]
```

Calling a generic function:
```kaba
square(10)      # i32->i32
square(10.0)    # f32->f32

square[i32](10) # explicit type
```

Will also work in [data pipes](pipes.md):
```kaba
[1,2,3] |> square
```

## Templated classes

So far, you can not write your own templated classes!

But internally lists `X[]`, dicts `X{}`, pointers `X*`, `owned[X]` etc. are implemented as instances of templated classes.


