# TF2 patcher for full-colored decals

## Summary

This tool allows you to remove various restrictions when applying a decal to a decalable item in TF2.

In short, now you can apply fully-colored images to your Objectors, Flairs, Photo Badges and Clan Prides.

This is a complete rewrite of [tf2-decal-patcher](https://github.com/unnamed10/tf2-decal-patcher).


## Is it safe?

It is completely safe to use. However:

- It may be a good idea to take a look at current active issues, in an unlikely event that this tool gets detected by VAC (which is highly improbable anyway);

- After running this tool, be sure to close TF2. You can then safely open it again. This is not strictly necessary, but it's best to clear all memory modifications before connecting to a secure server.


## How to use

1. Download the latest release [here](https://github.com/default-username-was-already-taken/tf2patcher/releases/latest);

2. Run tf2patcher.exe and launch TF2 (or do it in reverse, the order does not matter);

3. Now try applying a decal to your item - the preview pane should show a fully-colored image;

4. After you've applied your decal, close TF2 in order to remove any modifications caused by the patcher.


## Building from source

Run `make TARGET=32` or `make TARGET=64`, depending on which architecture you want to build for.

C11-aware compiler is required.


## How does it work
Due to a recent TF2 source code leak it became incredibly easy to determine the most optimal patching strategy for decal stuff.

Old patcher used a complicated and inefficient method that required you to enter an image path manually.

This version only patches two instructions, instead of, like, 20? And as an added benefit, you don't need to enter an image path anymore.

What exactly gets patched:

- m_pFilterCombo->GetActiveItem() call gets changed to "mov eax, 1" (reg/imm32 form) - this forces an "Identity" filter no matter what the schema says;

- bDoBlendLayers conditional jump gets changed to an inconditional jump (this skips all blending code altogether).


## Linux support?
Currently this project can only be built for Windows (both x86 and x86-64).

If you really need Linux binaries, create an issue and I'll take it from there.


## License

See UNLICENSE file.


## Contact
[Steam](https://steamcommunity.com/id/unnamed10) | Discord: default_username#6209
