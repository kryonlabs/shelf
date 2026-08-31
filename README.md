# Shelf

Shelf is a Kryon/libdraw file explorer for Rill and Taiji OS.

It is a separate application repo, like ktrem. Rill hosts it through the
Kryon `AppHost` ABI, while the standalone `shelf` binary can run by itself.

```sh
make
make test
make install
```

`make install` installs the standalone binary, Rill host module, and desktop
launcher under the selected `PREFIX` (`~/.local` by default).
