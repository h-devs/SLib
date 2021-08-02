# Dokany
[![Github All Releases](https://img.shields.io/github/downloads/dokan-dev/dokany/total.svg)](https://github.com/dokan-dev/dokany/releases)
[![Build status](https://ci.appveyor.com/api/projects/status/4tpt4v8btyahh3le/branch/master?svg=true)](https://ci.appveyor.com/project/Maxhy/dokany/branch/master)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=dokany&metric=alert_status)](https://sonarcloud.io/dashboard?id=dokany)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/43afb600ca75462e94d9484c32090135)](https://www.codacy.com/app/Liryna/dokany?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=dokan-dev/dokany&amp;utm_campaign=Badge_Grade)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1234/badge)](https://bestpractices.coreinfrastructure.org/projects/1234)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fdokan-dev%2Fdokany.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fdokan-dev%2Fdokany?ref=badge_shield)
|
[![Bounty](https://img.shields.io/bountysource/team/dokan-dev/activity.svg)](https://www.bountysource.com/teams/dokan-dev/issues)
|
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/dokan-dev/dokany.svg)](http://isitmaintained.com/project/dokan-dev/dokany "Average time to resolve an issue")
[![Percentage of issues still open](http://isitmaintained.com/badge/open/dokan-dev/dokany.svg)](http://isitmaintained.com/project/dokan-dev/dokany "Percentage of issues still open")

![Dokan Demo](http://dokan-dev.github.io/images/screencast.gif)

## What is Dokan
When you want to create a new file system on Windows, other than FAT or NTFS,
you need to develop a file system driver. Developing a device driver that works
in kernel mode on windows is extremely technical. By using Dokan, you can create
your own file systems very easily without writing device drivers. Dokan is
similar to FUSE (Linux file system in user space) but works on Windows. Moreover,
dokany includes a [FUSE wrapper](https://github.com/dokan-dev/dokany/wiki/FUSE)
that helps you to port your FUSE filesystems without changes.

## What is Dokany
*Dokany is a fork of Dokan 0.6.0 with bug fixes, clean change history and
updated to build with latest tools.*

Because the original Dokan Legacy (< 0.6.0) project is no longer maintained.

Since version 0.8.0, dokany broke compatibility with the dokan API. See
[Choose a version](https://github.com/dokan-dev/dokany/wiki/Installation#choose-a-version)
for more information.

**Signed x86 / x64 / ARM / ARM64 drivers** are provided at each release.

## Licensing
Dokan contains LGPL and MIT licensed programs.

- user-mode library (dokan1.dll)   **LGPL**
- driver (dokan1.sys)              **LGPL**
- network library (dokannp1.dll)   **LGPL**
- fuse library (dokanfuse1.dll)    **LGPL**
- installer (DokanSetup.exe)       **LGPL**
- control program (dokanctl.exe)   **MIT**
- samples (mirror.exe / memfs.exe) **MIT**

For details, please check the license files.
 * **LGPL** license.lgpl.txt
 * **MIT**  license.mit.txt

You can obtain source files from https://dokan-dev.github.io

## Environment
Dokan works on
 * Windows Server 2019 / 2016 / 2012 (R2) / 2008 R2 SP1
 * Windows 10 / 8.1 / 8 / 7 SP1
 
Platform
 * x86
 * x64
 * ARM
 * ARM64

## How it works
Dokan library contains a user mode DLL (dokan1.dll) and a kernel mode file
system driver (dokan1.sys). Once the Dokan file system driver is installed, you can
create file systems which can be seen as normal file systems in Windows. The
application that creates file systems using Dokan library is called File system
application.

File operation requests from user programs (e.g., CreateFile, ReadFile,
WriteFile, ...) will be sent to the Windows I/O subsystem (runs in kernel mode)
which will subsequently forward the requests to the Dokan file system driver
(dokan1.sys). By using functions provided by the Dokan user mode library
(dokan1.dll), file system applications are able to register callback functions
to the file system driver. The file system driver will invoke these callback
routines in order to respond to the requests it received. The results of the
callback routines will be sent back to the user program.

For example, when Windows Explorer requests to open a directory, the CreateFile
with Direction option request will be sent to Dokan file system driver and the
driver will invoke the CreateFile callback provided by the file system
application. The results of this routine are sent back to Windows Explorer as
the response to the CreateFile request. Therefore, the Dokan file system driver
acts as a proxy between user programs and file system applications. The
advantage of this approach is that it allows programmers to develop file systems
in user mode which is safe and easy to debug.
 
To learn more about Dokan file system development, see the
[![API documentation](https://img.shields.io/badge/Documentation-API-green.svg)](https://dokan-dev.github.io/dokany-doc/html/) and the [samples](https://github.com/dokan-dev/dokany/tree/master/samples), especially [dokan_memfs](https://github.com/dokan-dev/dokany/tree/master/samples/dokan_memfs).

## Build
In short, download and install the
[Visual Studio 2019](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx), select [Windows 10 SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/) component during the install or from the Tools menu &
install the [WDK 10](https://msdn.microsoft.com/en-us/windows/hardware/hh852365.aspx)

For details, see the
[build page](https://github.com/dokan-dev/dokany/wiki/Build).

## Installation
For manual installation, see the
[installation page](https://github.com/dokan-dev/dokany/wiki/Installation).

## Contribute
You want Dokan to get better? Contribute!

Learn the code and suggest your changes on
[GitHub repository](https://github.com/dokan-dev).

Detect defects and report them on
[GitHub issue tracker](https://github.com/dokan-dev/dokany/issues).

Ask and answer questions on
[Github Discussions](https://github.com/dokan-dev/dokany/discussions) or 
[Google discussion group](https://groups.google.com/forum/#!forum/dokan).
