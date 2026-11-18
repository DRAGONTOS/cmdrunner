# cmdrunner

Run commands. *(Scaffold — the feature set is not pinned down yet; this README gets
filled in once the design is settled with the owner.)*

# Requirements

None beyond a compiler — no external dependencies yet.

# Installing

From source:
```
make all
sudo make install
```

Using rpm/deb:
```
make all
cd build
cpack -G RPM   # or -G deb
```

Using PKGBUILD:
```
makepkg -si PKGBUILD
```
