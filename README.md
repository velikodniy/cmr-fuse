cmr-fuse
========

A Cloud.Mail.Ru filesystem for Linux and OS X based on [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace).
You can mount your cloud disk to any folder on your computer.

At the moment cmr-fuse has no optimizations and provides read-only access. Other file operations will be added later.

How to build
------------

Currently this project supports macOS only.
For building you must install fuse-t.

You can build the command line utility with:

```sh
mkdir build
cd build
CMAKE_POLICY_VERSION_MINIMUM=3.5 cmake -G Ninja ..
ninja
```

Usage
-----

```sh
./cmr_fuse -o config=CONFIG PATH
```

`CONFIG` — path to your config file. It must be valid JSON document with login, domain and password. For example:

```json
{"user": "john.doe", "domain": "mail.ru", "password": "correct horse battery staple"}
```

`PATH` — path to mount point.

You do not need root privileges for mounting or unmounting.

Third-party components
----------------------

Project uses [c-algorithms](https://github.com/afiskon/c-algorithms) library by Aleksander Alekseev.
