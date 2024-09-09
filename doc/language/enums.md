# Enums

## Values

Enums are a set of discrete values:
```kaba
enum PlaybackMode
    PLAYING
    PAUSED
    NO_DATA
    ERROR

var mode = PlaybackMode.PLAYING
```
Values have type `PlaybackMode`.

These are integer constants, starting from 0 and ascending. The values can also be user-defined:
```kaba
enum PlaybackMode
    PLAYING = 13
    PAUSED = 99
```

Can be cast to integers and back:
```kaba
i32(PlaybackMode.PAUSED)

let mode = PlaybackMode.from_int(0)
```

The list of all values:
```kaba
PlaybackMode.all()
```

## Labels

Conversion to strings will try to use the value's name/label:
```kaba
print(PlaybackMode.PLAYING)
# -> "PLAYING"
```

Values can also be found by their label:
```kaba
let mode = PlaybackMode.parse("PLAYING")
```

🔥 This might change to handle errors soon... 

User-defined text labels:
```kaba
enum PlaybackMode
    PLAYING as "play"
    PAUSED as "pause"
```

## Matching

```kaba
match mode
    PlaybackMode.PLAYING => print("playing")
    PlaybackMode.PAUSED  => print("paused")
    *                    => print("...")

```

