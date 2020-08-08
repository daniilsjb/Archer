# Runner

Runner is a small cross-platform utility tool for running various Lox tasks. It provides a command-line interface for running tests and benchmarks, both from individual files and entire directories. It also generates summarized reports based on the run request, which can be controlled via optional flags.

To use this tool, you need a Python 3.6+ interpreter, which you may download [from the official Python website](https://www.python.org/downloads/).

## Usage

This is the general specification of runner's usage:

```shell
> runner.py [--help] {
    test      interpreter tests      [--help] [--interpreter-alias=INTERPRETER_ALIAS] |
    benchmark interpreter benchmarks [--help] [--interpreter-alias=INTERPRETER_ALIAS] [--compare-with=COMPARED] [--compared-alias=COMPARED_ALIAS][--repeat=REPEAT] [--select={avg, median, best, worst}]
}
```

Basic usage of runner is rather straightforward, as we will see in the following examples. For more information refer to the next sections on available commands and flags, or the runner's `--help` command.

To run tests, type the following into the command-line:

```shell
> runner.py test path_to_interpreter path_to_tests
```

To run benchmarks, type the following into the command-line:

```shell
> runner.py benchmark path_to_interpreter path_to_benchmarks
```

## Commands

The runner has a rather small and concise set of commands and flags that you may use. This section explains each of them in detail.

### help

```shell
> runner.py --help
```

Prints usage information. Each other command also supports this flag for detailed usage information.

### test

```shell
> runner.py test <interpreter> <tests> [--interpreter-alias]
```

Runs tests located at `tests` against the language implementation located at `interpreter` and reports all mismatches. If `interpreter` does not refer to a valid executable, the runner will abort execution and report an error. If `tests` is path to a single file, it will be run directly. If `tests` is path to a directory, the runner will walk that directory recursively, looking for files with `.lox` extension and run them one by one.

This command accepts the following optional arguments:

* `--interpreter-alias` or `-ia` - the name to be used in the reports instead of the specified interpreter path

When running a test file, the script first reads the file and extracts the test's expected results. It searches for them in inline comments, starting with `//`. An inline comment is recognized as an expected result information if it starts with `Expected:` (ignoring any whitespaces before and after). Everything after that up until the end of the line is considered a single expected result, with whitespaces trimmed.

After extracting program's expected output, the runner starts up the language implementation and passes in the file's path as argument. It captures the program's output from `stdout` and performs a line-by-line comparison with expected results. As such, each expected result must be equal to a corresponding line of the output.

If the number of expected results doesn't match with the number of actual results, the runner reports that as an error but doesn't perform any comparisons in that file. However, the total number of expected results is added to the test summary.

If the runner encounters a mismatch between expected and actual results, it reports the error and continues checking further. The total number of mismatches will be reported in the test summary.

If the runner is unable to run the file, the message produced by `stderr` will not be captured, allowing the user to see what went wrong. The runner will ignore the erroneous file but include the total number of expected results of the file in the test summary.

**Example:**

```shell
> runner.py test ..\..\out\build\Release\clox my_tests --interpreter-alias=release
```

### benchmark

```shell
> runner.py benchmark <interpreter> <benchmarks> [--interpreter-alias] [--compare-with] [--compared-alias] [--repeat] [--select]
```

Runs benchmarks located at `benchmarks` against the language implementation located at `interpreter` and provides a summary. If `interpreter` does not refer to a valid executable, the runner will abort execution and report an error. If `benchmarks` is path to a single file, it will be run directly. If `benchmarks` is path to a directory, the runner will walk that directory recursively, looking for files with `.lox` extension and run them one by one.

This command accepts the following optional arguments:

* `--interpreter-alias` or `-ia` - the name to be used in the reports instead of the specified interpreter path
* `--compare-with` - path to another interpreter against which the runner will compare performance. If specified, the runner will run the same benchmarks on both the original interpreter and the compared one
* `--compared-alias` or `-ca` - same as `--interpreter-alias`, but for the compared interpreter
* `--repeat` - the number of times each benchmark should be run, producing a separate result each. Must be within range 1-100. Default value is 1
* `--select` - the method by which the runner will select a single value out of a list of benchmark results for report and comparison. Available options are: `avg`, `median`, `best`, `worst`. Default value is `avg`

The script runs each benchmark by invoking the interpreter and passing in the file's path as argument. It captures the program's output from `stdout` and assumes that the program's elapsed time is located on the last line. It repeats this process according to the `--repeat` argument, producing a list of elapsed time results. Then it applies the function specified by the `--select` argument to produce a single value, which will be used in reporting.

If `--compare-with` interpreter is specified, the script will also perform the same process on the compared interpreted alongside the original interpreter. During reporting, it will compare each result of both interpreters and mark the one that performed best.

If the runner is unable to run the file, the message produced by `stderr` will not be captured, allowing the user to see what went wrong. The runner itself will terminate benchmark execution.

**Example:**

```shell
> runner.py benchmark ..\..\out\build\Release\clox my_benchmarks --interpreter-alias=release --compare-with=..\..\out\build\Old-Release\clox --compared-alias=old --repeat=5 --select=median
```

### Verbosity

Each of the runner's commands also supports a couple of optional mutually-exclusive flags that control the amount of output produced by the script:

* `--verbose` or `-v` - makes the runner produce more output, such as detailed information on the elapsed time of each benchmark run or the amount of successful checks in a single test file
* `--quiet` or `-q` - makes the runner produce less output, such that it only generates the report information without logging the running process

**Example:**

```shell
> runner.py --verbose test my_interpreter my_tests
```
