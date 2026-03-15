# shaker for foobar2000

A foobar2000 component that shakes desktop windows to the beat of the music.

Port of [shaker](https://mew.tv/projects/shaker.html) by [juni](https://git.sr.ht/~juni/shaker), originally a Winamp visualization plugin.

## Install

Download `foo_vis_shaker.fb2k-component` from [Releases](../../releases) and open it with foobar2000.

## Build

Requires Visual Studio 2022 with the C++ desktop workload.

```
curl -L -o fb2k-sdk.7z https://www.foobar2000.org/downloads/SDK-2025-03-07.7z
7z x -ofoobar2000-sdk fb2k-sdk.7z

curl -L -o wtl.zip "https://sourceforge.net/projects/wtl/files/WTL%2010/WTL%2010.01%20Release/WTL10_01_Release.zip/download"
7z x -owtl wtl.zip
```

Then open `shaker-foobar2000/foo_vis_shaker.sln` and build, or from the command line:

```
msbuild shaker-foobar2000/foo_vis_shaker.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143
```

The built DLL goes into `%APPDATA%/foobar2000-v2/user-components-x64/foo_vis_shaker/`.

## License

Do whatever you want with this. The original author gave [permission for derivative work](https://mastodon.social/@juni/110879255898603875).
