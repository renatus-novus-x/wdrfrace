# Licensing and publication notes

This is a practical publication checklist, not legal advice.

## Public components

- Original game source code: MIT License
- `src/wdr_se.mml`: original sound-effect data under CC0-1.0
- Public XDF built without purchased BGM: distributable with this repository

## Do not publish

- `src/bgmpriv.h` or `src/bgmstrm*.h`
- NDP DemoSongs source, archives, converted headers, audio, or derived private XDFs
- Human68k system files, proprietary ROMs, or third-party tools without redistribution permission

## Release checklist

1. Build the public XDF with plain `make`, without `PRIVATE_BGM=1`.
2. Confirm that `dist/wdrfrace.xdf` is the newly generated public build.
3. Keep private headers and `wdrpriv.xdf` outside Git history and releases.
4. Keep `LICENSE`, this file, and `THIRD_PARTY_NOTICES.md` in the repository.
5. Confirm publication terms with the upstream NDP author before announcing affiliation.
