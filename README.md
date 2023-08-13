# xid6info : read extended tags from SPC-files

[xid6info](https://github.com/ullenius/xid6info) is stand-alone command line app
for reading the extended tags (xid6) from SPC-files (music files for SNES/Super
Famicom).

**Not** for *PKCS#7* certificates which share the same filename extension.

## Usage

```sh
usage: xid6info <filename>
```

For example:
```sh
$ xid6info "./ff-mystic-quest/26 - Mystic Ballad.spc"
```
```
Official Soundtrack Title : Final Fantasy USA Mystic Quest Sound Collections (N25D-020)
OST disc : 1
OST track : 26
Publisher : Square
Copyright year : 1992
Intro length : 0x762a00
Fade length : 453120
No. times to loop : 1
```

## Supported tags
The program only displays tags that are present. All xid6-tags are supported:

* Song name
* Game name
* Artist's name
* Dumper's name
* Date song was dumped (ISO-8601)
* Emulator used
* Comments
* Official Soundtrack Title
* OST disc
* OST track
* Publisher's name
* Copyright year
* Introduction length
* Loop length
* End length
* Fade length
* Muted voices
* Number of times top loop
* Mixing (preamp) level

## Building
Run:
```sh
./build.sh
```

## Requirements
* C99

## Development
I wrote a Java app called `spctag` which supports all SPC tags. I wanted
something faster. `xid6info` reduces the running time by 99%.

This program was written as a complement to `espctag` (included in many Linux
distributions) that lack xid6-support.

## Licence
GPL-3.0-only
