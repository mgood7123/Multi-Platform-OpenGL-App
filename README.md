# Building

## https://skia.org/docs/user/build/

- (on windows, open Git Bash (install git for windows if you have not already))
- `./sync_skia.bash`

on windows, linux, and mac hosts this should result in a skia directory suitible for building

next, open `[HOST]-OS` where `[HOST]` is your OS
- `Windows-OS` opens with `Visual Studio` (`cmake project`)
- `Android-OS` opens with `Android Studio`
- other operating system hosts are yet to be supported

# Windows Only

before building, ensure `add_subdirectory(${SKIA_DIR}/cmake_generated`
contains a valid writable directory

for example
- `add_subdirectory(skia "D:/windows_skia_debug")`
- `add_subdirectory(skia "D:/android_skia_debug")`
- drive `D:` must exist and be user writable (it must not require admin permission to write to)

this is because windows limits the max path length to 260 characters

which can limit building of skia, a symptom of this can manifest itself
as a `path does not exist` even though in `file explorer` and `git bash` it would otherwise exists

the path length can be checked in `git bash` via `wc -c <<< "$A_LONG_PATH"`


# TODO

add git repository file encryption
- 1 MB encryption key
- - 128 bit AES encryption - 1 MB in bits / 128 bits = X AES encryption keys
- - - use BLAKE3
- generated on-device per user
- - must be stored by user securely
