# Async

Some operations (functions) might take a long time and we don't want to freeze the whole program. Some functions can immediately return and let you know at a later time, when they're finished.

🔥 So far, there is no support, to write these asynchronous functions in kaba! You can only call them.

An async function will return `future[X]`, i.e. is a value of type `X` that will only be available at a point in the future:
```kaba
func slow_operation() -> future[string]
```

You can deal with `future[X]` values, by attaching code to be called when the value is available:
```kaba
func f(x: string)
    print("finished: {{x}}")

slow_operation().then(f)
print("running...")
```

Operations might also fail:
```kaba
slow_operation().then_or_fail(f_ok, f_fail)
```

