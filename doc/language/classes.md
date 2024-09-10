# Classes

Classes define collections of variables with associated "member" functions.

🔥 In Kaba we use the term "class" and "type" interchangeably. My apologies to type theorists.

## Basics

A simple class with 2 member variables and a member function:
```kaba
class Vehicle
    var number_of_wheels: i32
    var id: string
    
    func drive()
        print("driving")
```

The class can be used like any other type. Instancing and accessing members:
```kaba
var v: Vehicle
v.drive()
v.number_of_wheels = 13
```

You can also heap-allocate objects:
```kaba
var v = new Vehicle()
v.drive()
```

🔥 Here `v` is a pointer with ownership. See [pointer](pointers.md) for details.

Inside member functions, the instance can be accessed through `self`:
```kaba
class X
    var a: f32
    func f()
        ...
    func g()
        print(self.a)
        self.f()
        
        # or shorter:
        print(a)
        f()
```

🔥 By default, member functions treat `self` as immutable. To allow changes, we need `func mut`:
```kaba
class X
    var a: f32
    func mut f()
        a = 13
```

🔥 `func mut` members can only be called, if the object is also mutable!

## Constructors

A constructor is a special member function called `__init__` it is called when creating an instance:
```kaba
class X
    var a: f32
    
    # "default" constructor:
    func __init__()
        a = 1
    
    # another constructor
    func __init__(_a: f32)
        a = _a

var x1: X            # use default constructor
var x2 = new X()     # use default constructor
var x3 = new X(2.0)  # use other constructor
```

🔥 When no user-defined constructor exists, an automatic default constructor is secretly added.

Special functions:

| Name                                  | Meaning                                       |
|:--------------------------------------|-----------------------------------------------|
| `__init__()`                          | constructor - called when creating the object |
| `__delete__()`                        | destructor - called when destroying the object |
| `__assign__(o: X)`                    | assignment operator `self = o`                |
| `__eq__(o: X) -> bool`                | equality operator `self == o`                 |
| `__lt__,__gt__,__le__,__ge__,__neq__` | `<,>,>=,<=,!=` operators                      |
| `__get__(key: ...) -> ...`            | `self[key]`                                   |
| ...                                   | ....                                          |


## Inheritance

We can inherit members from base classes, by extending them:
```kaba
class Base
    var a: i32
    func f()

class Derived extends Base
    # now automatically contains a + f
    
    func override f()
        # we really want our own version
```

🔥 We only allow single inheritance!

🔥 When overriding member functions, the compiler decides which one to call in the most "idiotic" way:
```kaba
var d: Derived
something(d)

func something(b: Base)
    b.f()
    # Base.f() is always called here, even though we know the object is Derived!
```  

## Virtual

To tell the compiler to call the correct member depending on the actually created object, use `virtual`:
```kaba
class Base
    func virtual f()

class Derived extends Base
    func override f()
```

🔥 Classes with virtual functions can not be derived from classes without. 

## Structs

Often we need "pure" data. Use `struct` then:
```kaba
struct Vector3d
    var x, y, z: f32
```

🔥 This automatically defines a default constructor, and constructor to set all member variables. Also assignment and comparison operators.



