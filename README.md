# KOMCast

[![ESEmu Project](https://avatars0.githubusercontent.com/u/8460236?s=230)][esemuweb]

KOMCast is a tool to extract and repack KOM archives from ElSword's clients up to 2014.

  - Unpack KOM archives for Algorithm 0 & 2.
  - Repack KOM archives, both Algorithms supported.
  - Can use external tools (unluac) to decompile LUA and recompile it on packing, allowing fast editing.
  - Tested in action.

This tool has been written by [d3vil401][d3vsite] from the team, you can find us on [ESEmu Project website][esemuweb]. 

### Usage

These are the arguments:

```sh
-u / -unpack : Unpack the target KOM.
-o / -out : Targeted output path.
-p / -pack :  Pack targeted directory/file.
-r / -recompile : Uses luac.exe to recompile files before repacking.
-d / -decompile : Uses unloac.jar to decompile files after extraction.
```

To pack to a KOM from a folder, and recompile its LUA files:
```
./KOMCast.exe -o data036_test2.kom -p ./out -r
```

To unpack a KOM archive to a folder, and decompile its LUA files:
```
./KOMCast.exe -u data036_test2.kom -o ./out -d
```

   [d3vsite]: <https://d3vsite.org>
   [esemuweb]: <https://esemuproject.org/>
