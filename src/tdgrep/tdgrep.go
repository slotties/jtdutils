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
}

func main() {
	name := flag.String("n", "", "either an exact name or a regular expression like 'http.*' to match everything that starts with 'http'")
	// TODO: stacktrace
	// TODO: native pid
	// TODO: java-pid
	// TODO: state

	parser := tdtool.InitTool()
	parser.KeepContent = true

	filters, err := createFilters(*name) 
	if err != nil {
		fmt.Fprintf(os.Stderr, "Bad filters: %s\n", err.Error())
		os.Exit(1)
	}

	grepThreads(parser, filters, os.Stdout)
}

func createFilters(name string) (Filters, error) {
	var err error

	filters := Filters{}
	filters.name, err = regexp.Compile(name)
	if err != nil {
		return filters, err
	}

	return filters, nil
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

	return false
}
