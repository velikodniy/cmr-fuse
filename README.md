cmr-fuse
========

A Cloud.Mail.Ru filesystem for Linux and OS X based on [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace).
You can mount your cloud disk to any folder on your computer.

At the moment cmr-fuse has no optimizations and provides read-only access. Other file operations will be added later.

How to build
------------

For building you must install development packages for this libraries:
- libfuse (interface to export filesystem to the kernel),
- libjansson (JSON parser),
- libcurl (HTTP queries)

You can build the command line utility with:
```bash
cmake .
make
```

(Note the dot after `cmake`.)

Usage
-----

Just run a command like
```
cmr_fuse -o config=CONFIG PATH
```

`CONFIG` — path to your config file. It must be valid JSON document with login, domain and password. For example:

```json
{"user": "john.doe", "domain": "mail.ru", "password": "correct horse battery staple"}
```

`PATH` — path to mount point.

You can unmount filesystem with
```bash
fusermount -u PATH
```

You do not need root privileges for mounting or unmounting.

Third-party components
----------------------

Project uses [c-algorithms](https://github.com/afiskon/c-algorithms) library by Aleksander Alekseev.
