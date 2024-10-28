# OpenCReports

OpenCReports is a report generator engine based on ideas from RLIB
with a complete reimplementation and licensed under LGPLv3.

The most up-to-date original repository was at https://github.com/SICOM/rlib
which was (together with many others under https://github.com/SICOM)
recently deleted. Ancient versions of RLIB are at https://sourceforge.net/projects/rlib

## Currently missing features

* language bindings other than C and PHP
* graphing (Graph, Chart XML elements)

## Roadmap

* 0.9: Graph and Chart support in PDF (full feature parity with RLIB, with many extras)
* 1.0: GUI to create either code or XML
* 1.1: Possibly port the reporting library part to Windows and Android, GUI to Windows

## Documentation

[OpenCReports documentation](https://zboszor.github.io/OpenCReports-docs/index.html)

[OpenCReports documentation as single page HTML](https://zboszor.github.io/OpenCReports-docs/OpenCReports.html)

[OpenCReports documentation as A4 PDF](https://zboszor.github.io/OpenCReports-A4.pdf)

[OpenCReports documentation as US letter PDF](https://zboszor.github.io/OpenCReports-US.pdf)

## Licensing

OpenCReports (the library part) is free software, with "free" as in speech.
It is licensed under LGPLv3.

About commercial licensing and billing, contact me at the email address at the end of this page.

## Build and installation

The dependencies to build OpenCReports are:

* automake and autoconf
* GNU GMP and MPFR
* utf8proc
* libxml2
* libpaper
* libcsv
* yajl
* libgdk_pixbuf
* librsvg
* Cairo built with PDF support
* PangoCairo
* libpq (the PostgreSQL client library)
* mariadb-connector-c
* unixODBC (preferred) or iodbc

### Build OpenCReports from sources

```
autoreconf -vif -Wall
./configure --prefix=/usr --libdir=/usr/lib64
make
sudo make install
```

### Build OpenCReports PHP module

OpenCReports contains a PHP module that supports PHP 5.4 or newer versions.
It is possible that it suppports older PHP versions than 5.4 but it
was not tested.
The PHP module can be built after the OpenCReports engine is built and installed.

```
cd bindings/php
phpize
./configure
make
sudo make install
```

## Run the unit tests

Create a test database in both a locally running PostgreSQL server and
a MariaDB server:

```
cat tests/mariadb/create.sql | mysql -u root mysql
psql -f tests/postgresql/create.sql -U postgres postgres
```

There are a lot of unit tests for the engine itself also written in C,
so build them first, possibly with as many jobs as the number of CPUs
in your computer.

```
make -j16 -C tests
```

Run most of the tests that have stable results:

```
make -C tests test
```

There are some tests for which the results are inherently unstable,
like using random(). Their results are expected to fail:

```
make -C tests unstable-test
```

There are some slow database tests:

```
make -C tests slow-test
```

This command runs all the above in one go:

```
make -C tests all-test
```

There are also PHP binding tests:

```
make -C tests php-test
```

Author: Zoltán Böszörményi <zboszor@gmail.com>
