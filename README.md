# OpenCReports

Eventually, this repository will contain a reporting library
based on ideas from RLIB (https://github.com/SICOM/rlib) but
having a complete reimplementation and licensed under LGPLv3
under verbal consent from the original authors:

* Bobby Doan <bdoan@sicom.com>
* Christian Betz <cbetz@sicom.com>

## Licensing

OpenCReports (the library part) is free software, with "free" as in speech.
It is licensed under LGPLv3.

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

## Run the unit tests

Create a test database in both a locally running PostgreSQL server and
a MariaDB server:

```
cat tests/mariadb/create.sql | mysql -u root mysql
psql -f tests/postgresql/create.sql -U postgres postgres
```

There are a lot of unit tests, so build them first, possibly with
as many jobs as the number of CPUs in your system.

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

This command runs all the tests:

```
make -C tests all-test
```

Author: Zoltán Böszörményi <zboszor@gmail.com>
