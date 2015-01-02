package main

import (
	"fmt"
	"tdformat"
	"flag"
	"os"
	"io"
)

type StateStats struct {
	dump tdformat.ThreadDump
	stats map[int]int
}

func main() {
	fileName := flag.String("f", "", "the file to use (stdin is used per default)")
	showHelp := flag.Bool("h", false, "shows this help")
	/* TODO:
		support for --hex (print hex tids/nids instead of decimal ones)
		support for -r (reverse sort order)
		support for -P (sort by native PID)
		support for -J (sort by java PID)
		support for -N (sort by name)
		support for -S (sort by state)
		support for -1 (just thread names, one per line)
	 */
	flag.Parse()

	if *showHelp {
		flag.PrintDefaults()
		os.Exit(0)
	}

	reader, err := tdformat.OpenFile(fileName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Could not open file '%v': %v\n", *fileName, err)
		os.Exit(1)
	}
	defer reader.Close()

	parser := tdformat.NewParser(reader)
	parser.ParseStacktrace = false
	
	listThreads(parser, os.Stdout)
}

func listThreads(parser tdformat.Parser, out io.Writer) {
	// TODO: print header
	var threads []tdformat.Thread

	for parser.NextThread() {
		if parser.SwitchedDump() && len(threads) > 0 {
			// TODO: sort threads
			printThreads(threads, out)

			threads = make([]tdformat.Thread, 0)
		}

		threads = append(threads, parser.Thread())
	}

	// Flush the recorded list of threads
	if len(threads) > 0 {
		// TODO: sort threads
		printThreads(threads, out)
	}
}

func printThreads(threads []tdformat.Thread, out io.Writer) {
	for _, thread := range threads {
		fmt.Fprintf(out, "%-35v %v %6v\n",
			thread.Name,
			convertState(thread.State),
			thread.Pid)
	}
}

func convertState(state int) (string) {
	switch state {
		case tdformat.THREAD_RUNNING:
			return "R"
		case tdformat.THREAD_WAITING:
			return "W"
		case tdformat.THREAD_TIMED_WAITING:
			return "T"
		case tdformat.THREAD_PARKED:
			return "P"
		case tdformat.THREAD_BLOCKED:
			return "B"
		default:
			return "U"
	}
}