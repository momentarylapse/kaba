# Data pipelines

## Mapping lists

Lists can be mapped/transformed element-wise by a function:
```kaba
let out = [1.0, 2.0, 3.0] |> sqrt
```

This is equivalent to:
```kaba
var out: float[]
for x in [1.0, 2.0, 3.0]
    out.add(sqrt(x))
````

Multiple mapping stages can be chained. It is advisable to keep each stage in a separate line:
```kaba
["1", "2.1", "3"]
    |> string.__f32__
    |> sqrt
    |> sin
```
Functions' output types don't have to be the same as the input types, as long as consecutive stages match up.

Functions can also act on the whole list. E.g. the some of squares can be computed with `func sum(a: f32[]) -> f32`:
```kaba
[1.0, 2.0, 3.0]
    |> sqr
    |> sum
```

## Mapping optionals

Optionals can have their values be extracted, transformed and re-wrapped:
```kaba
(13.0 as f32?) |> sin
```

🔥 If the function returns an optional, then the additional wrapping will be skipped!

## Sorting

"Simple" ascending sorting of basic values (can deal with `i32`, `f32`, `string`, `Path`):
```kaba
range(10.0)
    |> sin
    |> sort
```

Data structures can be sorted by elements of basic values:
```kaba
[vec3(1,0,0), vec3(0,1,0)]
    |> sort("x")
```
(sort 3d vectors by their x-component)

Sort descending:
```kaba
list |> sort("-")
```

## Filtering

Only pass on positive numbers:
```kaba
range(10.0)
    |> sin
    |> filter(x => x>0)
```

