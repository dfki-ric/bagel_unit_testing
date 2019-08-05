# bagel_unit_testing

Bagel (Biologically inspired Graph-Based Language) is a cross-platform
graph-based dataflow language developed at the
[Robotics Innovation Center of the German Research Center for Artificial Intelligence (DFKI-RIC)](http://robotik.dfki-bremen.de/en/startpage.html)
and the [University of Bremen](http://www.informatik.uni-bremen.de/robotik/index_en.php).
It runs on (Ubuntu) Linux, Mac and Windows.

This package implements a method to test Bagel graphs.

# Documentation {#documentation}

The main user documentation of Bagel can be found at:
https://github.com/dfki-ric/bagel_wiki/wiki

Once `bagel_unit_testing` is installed it can be executed in
a configuration folder. The configuration folder have to include
the Bagel graph to test, a unit test file with the same name as
the graph extended by `unit_test` and a configuration file including
a list of tests to perform.

In the test folder an example configuration is given including a
`SimpleTest.yml` Bagel graph, a `SimpleTest_unit_test.yml` file
defining the inputs to test with expected outputs, and the
`bagel_unit_testing.yml` configuration file linking the `SimpleTest`
graph.

# Test

To run the test:

    cd test
    ./test.sh

If everything is fine the test output should be:

    -- test succeeded --

## License

bagel_unit_testing is distributed under the
[3-clause BSD license](https://opensource.org/licenses/BSD-3-Clause).
