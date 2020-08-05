# Runner

Runner is a small utility tool for running Lox test programs. To make sure the language implementation is working correctly, this script is used to run a test suite of small programs and report errors or mismatches against the expected results. It supports a few commands, which are explained in detail further on.

To use this tool, you need a Python 3 interpreter. This implementation in particular was written using Python 3.8.5. You may download an interpreter here: <https://www.python.org/downloads/>.

## Commands

### help

Pretty self-explanatory. Prints usage information.

### run [program] [test]

Runs tests located at `test` against the language implementation located at `program`.

If `program` does not refer to a valid executable, the runner will abort execution and report an error.

If `test` is path to a single file, it will be run directly. If `test` is path to a directory, the runner will walk that directory recursively, looking for files with `.lox` extension and run them one by one, accumulating results to produce a summarized report.

When running a test file, the script first reads the file and extracts the test's expected results. It searches for them in inline comments (the ones that start with `//`). An inline comment is recognized as an expected result information if it starts with `Expected:`, ignoring any whitespaces before and after. Everything after that up until the end of the line is considered a single expected result, with whitespaces trimmed.

After extracting program's expected output, the runner starts up the language implementation and passes in the file's path as argument. It captures the program's output from `stdout` and performs a line-by-line comparison with expected results. As such, each expected result must be equal to a corresponding line of the output.

If the number of expected results doesn't match with the number of actual results, the runner reports that as an error but doesn't perform any comparisons in that file. However, the total number of expected results is added to the test summary.

Whenever the runner encounters a mismatch between expected and actual results, it reports the error and continues checking further. The total number of mismatches will be reported in the test summary.

Exmple usage: `runner.py run clox ..\..\tests`
