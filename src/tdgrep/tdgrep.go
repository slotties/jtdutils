package main

import (
	"tdformat"
	"flag"
	"os"
	"io"
	"tdtool"
	"regexp"
	"fmt"
)

type Filters struct {
	name *regexp.Regexp
	stacktrace *regexp.Regexp
}

func main() {
	name := flag.String("n", "", "either an exact name or a regular expression like 'http.*' to match everything that starts with 'http'")
	stacktrace := flag.String("s", "", "a regular expression that is performed on every stacktrace line")
	// TODO: native pid
	// TODO: java-pid
	// TODO: state

	parser := tdtool.InitTool()
	parser.KeepContent = true
	parser.ParseStacktrace = true

	filters, err := createFilters(*name, *stacktrace) 
	if err != nil {
		fmt.Fprintf(os.Stderr, "Bad filters: %s\n", err.Error())
		os.Exit(1)
	}

	grepThreads(parser, filters, os.Stdout)
}

func createFilters(name string, stacktrace string) (Filters, error) {
	var err error

	filters := Filters{}
	if filters.name, err = compileIfProvided(name); err != nil {
		return filters, err
	}
	if filters.stacktrace, err = compileIfProvided(stacktrace); err != nil {
		return filters, err
	}

	return filters, nil
}

func compileIfProvided(expression string) (*regexp.Regexp, error) {
	if expression != "" {
		return regexp.Compile(expression)
	} else {
		return nil, nil
	}
}

func grepThreads(parser tdformat.Parser, filters Filters, out io.Writer) {
	for parser.NextThread() {
		if parser.SwitchedDump() {
			// TODO: print dump start
		}

		thread := parser.Thread()
		if isMatching(&thread, &filters) {
			fmt.Fprintln(out, thread.TextContent)
		}
	}
}

func isMatching(thread *tdformat.Thread, filters *Filters) (bool) {
	if filters.name != nil {
		return filters.name.MatchString(thread.Name)
	}
	if filters.stacktrace != nil {
		for _, line := range thread.Stacktrace {
			if filters.stacktrace.MatchString(line.MethodName) {
				return true
			}
		}
	}

	return false
}
