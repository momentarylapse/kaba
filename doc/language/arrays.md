# Arrays

Kaba supports 2 types of arrays:
* fixed sized arrays
* dynamically sized arrays, or "lists"

## Lists

The more common type.

### Basics

A list of integers has type `i32[]`.

```kaba
var list: i32[]

list = [1,2,3]

list.add(4)
# -> [1,2,3,4]

len(list)
# -> 4
```

### Access

Element access:
```kaba
var list = [1,2,3]
print(list[0])      # -> 1 ... since [0] is the first element...

list[1] = 99
# -> [1,99,3]
```

Slices/sub lists can be accessed via:
```
var list = [1,2,3,4,5,6,7,8,9]
list[2:5]   # (first index):(first after) -> [3,4,5]
```

Slices can be used to alter elements:
```kaba
list[2:5] = [-1,-2,-3]
# -> list = [1,2,-1,-2,-3,6,7,8,9]
```

🔥 Mutability of a list extends to its elements!

### Iteration

Iterating over elements in a list:
```kaba
for i in [1,2,3]
    print(i)
```

Here, `i` is immutable! To change the elements, we need `for mut`:
```kaba
var list = [1,2,3]

for mut i in list
    i *= 10
# -> [10,20,30]
```

We can also iterate including the indices and values:
```
for i=>value in [100,200,300]
    # ...

# i=0, value=100
# i=1, value=200
# i=2, value=300
```

### Pipelines

[Data pilelines](pipes.md) can be used to transform, filter and sort arrays:
```kaba
[1.0, 2.0, 3.0] |> sin |> sort |> filter(x => x>0) |> sum
```

## Fixed sized arrays

An array with 100 integer elements is `i32[100]`.

