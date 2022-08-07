# Kaba

Kaba is a general purpose programming language. The core is a simplified version of c++, with some python-style syntax sugar.

It compiles down to x86 machine code, binary compatible with c++ (at least g++ and msvc), and comes with a JIT compiler that can be used for plugin systems, interacting with a host program.

This is the stand-alone compiler/execution environment.

## Language

Uses indentation for blocks:

```python
func main()
    print("hi")
```

Strong typing system, with easy built-in containers:

```c++
func f(i: int, s: string, some_tuple: (int,float)) -> int[]
    # btw. s is automatically call-by-reference, but const
    return [i,1,2,3]
```

Function pointers & lambdas:

```c++
func g(func_pointer: int->int)

func h()
    g(func(i: int)  i^2) # squaring i
    let result = map(sqrt, [1.0, 2.0, 3.0, 4.0])
```

Funny security flags:

```c++
class C
    s: string

    # without "mut", our object is const!
    func mut f()
        s = "a"

    # we expose a reference to us
    # makes the return value mutable, if self is mutable
    func ref g() -> string
        return s[:2]
```

Much more. See: https://michi.is-a-geek.org/projects/kaba/

## Libraries

* user interface (gtk backend)
* OpenGL
* vulkan
* threads
* math (3d linear algebra, fft)

For a full reference, see: https://wiki.michi.is-a-geek.org/kaba.reference/

## Building

* [how to build and run](doc/how-to-build.md)


