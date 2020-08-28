import sys
import os
import subprocess
import argparse
import statistics

class TestResult:
    def __init__(self, interpreter, file, successful_count, total_count):
        self.interpreter = interpreter
        self.file = file
        self.successful_count = successful_count
        self.total_count = total_count
        
class BenchmarkResult:
    def __init__(self, interpreter, file, elapsed):
        self.interpreter = interpreter
        self.file = file
        self.elapsed = elapsed

selection_methods = {
    "avg": statistics.mean,
    "median": statistics.median,
    "best": max,
    "worst": min
}
        
def main():
    parser = argparse.ArgumentParser(description="Runner is a utility tool for running various Lox tasks.")
    parser.set_defaults(func=lambda _: parser.print_help())
    
    verbosity_group = parser.add_mutually_exclusive_group()
    verbosity_group.add_argument("-v", "--verbose", action="store_true", help="makes the program produce more information in the output")
    verbosity_group.add_argument("-q", "--quiet", action="store_true", help="makes the program produce less information in the output")
    
    subparsers = parser.add_subparsers()
    
    parser_test = subparsers.add_parser("test", help="runs tests and reports mismatches")
    parser_test.add_argument("interpreter", help="path to the interpreter against which the tests should be run")
    parser_test.add_argument("tests", help="path to the file or directory containing tests to run")
    parser_test.add_argument("-ia", "--interpreter-alias", dest="interpreter_alias", help="alias for the interpreter to be used in the program's output")
    parser_test.set_defaults(func=test)
    
    parser_benchmark = subparsers.add_parser("benchmark", help="runs benchmarks and provides various comparisons of results")
    parser_benchmark.add_argument("interpreter", help="path to the interpreter against which the benchmarks should be run")
    parser_benchmark.add_argument("benchmarks", help="path to the file or directory containing benchmarks to run")
    parser_benchmark.add_argument("-ia", "--interpreter-alias", dest="interpreter_alias", help="alias for the interpreter to be used in the program's output")
    parser_benchmark.add_argument("--compare-with", dest="compared", help="path to another interpreter to be run for performance comparison")
    parser_benchmark.add_argument("-ca", "--compared-alias",dest="compared_alias", help="alias for the compared interpreter to be used in the program's output")
    parser_benchmark.add_argument("--repeat", type=int, default=1, help="the number of times each benchmark should be run")
    parser_benchmark.add_argument("--select", choices=["avg", "median", "best", "worst"], default="avg", help="specifies the selection method of benchmark results")
    parser_benchmark.set_defaults(func=benchmark)
    
    args = parser.parse_args()
    args.func(args)

def test(args):
    path = args.tests
    
    if os.path.isfile(path):
        test_file(args)
    elif os.path.isdir(path):
        test_directory(args)
    else:
        exit(f"Specified path is not recognized as an existing file or directory: '{path}'")
        
def test_file(args):
    file = args.tests
    
    interpreter_name = args.interpreter_alias or args.interpreter
    
    if not is_valid_file(file):
        exit("Can only run files with '.lox' extension.")
    
    print("Starting testing process. This may take a while...")
    
    result = run_test(args.interpreter, file, interpreter_name, args)
    print(f"Successfully completed {result.successful_count} out of {result.total_count} checks in total.")

def test_directory(args):
    directory = args.tests
    
    interpreter_name = args.interpreter_alias or args.interpreter
    
    successful_count = 0
    total_count = 0
    
    print("Starting testing process. This may take a while...")
    
    for root, _, files in os.walk(directory):
        for file in files:
            if not is_valid_file(file):
                continue
            
            complete_path = os.path.join(root, file)
            result = run_test(args.interpreter, complete_path, interpreter_name, args)
            
            successful_count += result.successful_count
            total_count += result.total_count
            
    if total_count == 0:
        print("Specified directory does not contain any valid tests.")
    else:
        print(f"Successfully completed {successful_count} out of {total_count} checks in total.")
        
def run_test(interpreter, file, alias, args):
    parsed = parse_expected(file)
    
    expected, lines = map(list, zip(*parsed)) if parsed else ([], [])
    expected_count = len(expected)
    
    if not args.quiet:
        print(f"Running test '{file}' with '{alias}'...")
    
    actual = get_interpreter_output(interpreter, file)
    if actual == None:
        print(f"File '{file}', containing {expected_count} checks, could not be run with '{alias}'.")
        return TestResult(interpreter, file, 0, expected_count)
    
    actual_count = len(actual)

    if expected_count != actual_count:
        print(f"Expected {expected_count} results but got {actual_count} in '{file}':")
        print(f"- Expected: {expected}")
        print(f"- Actual: {actual}")
        return TestResult(interpreter, file, 0, expected_count)
    
    successful_count = 0
    for expected_item, actual_item, line in zip(expected, actual, lines):
        if expected_item != actual_item:
            print(f"Result mismatch: expected '{expected_item}' but got '{actual_item}' in test '{file}' on line {line}.")
        else:
            successful_count += 1
    
    if args.verbose:
       print(f"Test '{file}' run with '{alias}' completed {successful_count} out of {actual_count} checks successfully.")
            
    return TestResult(interpreter, file, successful_count, actual_count)

def parse_expected(file):
    expected = []
    current_line = 1
    
    reader = open(file, "r")
    for line in reader:
        _, _, comment = line.partition("//")
        
        comment = comment.strip()        
        if comment.startswith("Expected:"):
            _, _, value = comment.partition(':')
            expected.append((value.strip(), current_line))
            
        current_line += 1
            
    return expected

def benchmark(args):
    if (not (1 <= args.repeat <= 100)):
        exit("'repeat' must be in range 1-100")
    
    path = args.benchmarks
    
    if os.path.isfile(path):
        benchmark_file(args)
    elif os.path.isdir(path):
        benchmark_directory(args)
    else:
        exit(f"Specified path is not recognized as an existing file or directory: '{path}'")
        
def benchmark_file(args):
    file = args.benchmarks
    
    interpreter_alias = args.interpreter_alias or args.interpreter
    compared_alias = args.compared_alias or args.compared
    
    if not is_valid_file(file):
        exit("Can only benchmark files with '.lox' extension.")
    
    print("Starting benchmarking process. This may take a while...")
    
    if args.compared:
        interpreter_result = run_benchmark(args.interpreter, file, interpreter_alias, args)
        compared_result = run_benchmark(args.compared, file, compared_alias, args)
        
        interpreter_is_best = interpreter_result.elapsed < compared_result.elapsed
        
        print(f"The {args.select} elapsed time across file '{file}' run {args.repeat} times:")
        
        print(f"Interpreter '{interpreter_alias}' - {interpreter_result.elapsed} seconds{' [Best]' if     interpreter_is_best else ''}")
        print(f"Interpreter '{   compared_alias}' - {   compared_result.elapsed} seconds{' [Best]' if not interpreter_is_best else ''}")
    else:
        result = run_benchmark(args.interpreter, file, interpreter_alias, args)
        print(f"The {args.select} elapsed time across file '{file}' run against '{interpreter_alias}' {args.repeat} times is {result.elapsed} seconds.")

def benchmark_directory(args):
    directory = args.benchmarks
    
    interpreter_results = []
    compared_results = []
    
    interpreter_alias = args.interpreter_alias or args.interpreter
    compared_alias = args.compared_alias or args.compared
    
    print("Starting benchmarking process. This may take a while...")
    
    for root, _, files in os.walk(directory):
        for file in files:
            if not is_valid_file(file):
                continue
            
            complete_path = os.path.join(root, file)
            
            interpreter_results.append(run_benchmark(args.interpreter, complete_path, interpreter_alias, args))
            if args.compared:
                compared_results.append(run_benchmark(args.compared, complete_path, compared_alias, args))
                
    if args.compared:
        total_count = len(interpreter_results)
        
        outperform_count = 0
        outperformed_results = []
        for left, right in zip(interpreter_results, compared_results):
            if left.elapsed < right.elapsed:
                outperformed_results.append(True)
                outperform_count += 1
            else:
                outperformed_results.append(False)
        
        print(f"The {args.select} elapsed time across directory '{directory}' run {args.repeat} times:")
        
        print(f"Interpreter '{interpreter_alias}':")
        for result, outeperformed in zip(interpreter_results, outperformed_results):
            print(f"    > '{result.file}' - {result.elapsed} seconds{' [Best]' if     outeperformed else ''}")
            
        print(f"Interpreter '{compared_alias}':")
        for result, outeperformed in zip(compared_results, outperformed_results):
            print(f"    > '{result.file}' - {result.elapsed} seconds{' [Best]' if not outeperformed else ''}")
  
        print(f"Interpreter '{interpreter_alias}' performed better in {outperform_count} out of {total_count} benchmarks.")
    else:
        print(f"The {args.select} elapsed time across directory '{directory}' run against '{interpreter_alias}' {args.repeat} times:")
        for result in interpreter_results:
            print(f"    > '{result.file}' - {result.elapsed} seconds")

def run_benchmark(interpreter, file, alias, args):
    results = []
    for i in range(args.repeat):
        if not args.quiet:
            print(f"Running benchmark '{file}' with '{alias}'{f'(iteration #{i + 1})' if args.repeat > 1 else ''}")
        
        elapsed = get_benchmark_elapsed(interpreter, file)
        if args.verbose:
            print(f"Benchmark '{file}' run with '{alias}' took {elapsed} seconds to complete.")
        
        results.append(elapsed)
    
    selected_value = selection_methods[args.select](results)
    return BenchmarkResult(interpreter, file, selected_value)

def get_benchmark_elapsed(interpreter, file):
    output = get_interpreter_output(interpreter, file)
    if output == None:
        exit(f"Benchmark '{file}' failed to run with '{interpreter}', terminating benchmarking process.")
    
    return float(output[-1])

def get_interpreter_output(interpreter, file):
    output = None
    try:
        output = subprocess.check_output([interpreter, file])
    except OSError:
        exit(f"Couldn't run interpreter at '{interpreter}'.")
    except subprocess.CalledProcessError:
        return
        
    return [x.decode() for x in output.splitlines()]

def is_valid_file(file):
    return file.rpartition('.')[-1].lower() == "lox"

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        exit("Stopping gracefully...")