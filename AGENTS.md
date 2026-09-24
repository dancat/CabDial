PlatformIO builds can spend significant time in dependency scanning with
little output. Do not treat dependency scanning or temporary lack of output
as a build failure.

For build verification, run `pio run` from the project root and wait for the
process to terminate. Only report build failure if PlatformIO returns a
non-zero exit status or emits an actual error.

Do not repeatedly clean `.pio` or reinstall dependencies just because
dependency scanning appears slow.