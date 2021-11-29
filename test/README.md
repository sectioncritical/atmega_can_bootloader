Automated Testing
=================

There is some limited automated testing. There are some unit tests to verify
some features of the boot loader. The unit tests are implement using the
[Unity unit test framework](http://www.throwtheswitch.org/unity). This is not
the Unity graphics engine.

The unity files are brought in as a submodule.

The unit tests are built and run on a development host, not the target.

Simple things you can do:

* `make` - build the unit tests
* `make run` - run the unit test
* `make clean` - clean all the build products
* `make tidy` - clean nuisance coverage files but leave build products

**Notes:**

- the unit tests mainly test the message processing logic
- code coverage intermediate files (.gcda, .gcno) files will appear in the
  test directory. These are meant to be used for generating a code coverage
  report that is not implemented yet. These can be ignored or removed with
  `make tidy`.
