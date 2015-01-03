package main

import (
	"fmt"
	"tdformat"
	"flag"
	"os"
	"io"
	"sort"
)

type StateStats struct {
	dump tdformat.ThreadDump
	stats map[int]int
}

type PrintPidFunc func(thread tdformat.Thread, out io.Writer)

type Conf struct {
	sortBy string
	reverseOrder bool
	printPid PrintPidFunc
}

func main() {
	fileName := flag.String("f", "", "the file to use (stdin is used per default)")
	showHelp := flag.Bool("h", false, "shows this help")
	sortBy := flag.String("s", "", "sort by any of: name, pid, state or empty (natural order)")
	reverseOrder := flag.Bool("r", false, "reverse sort order")
	printHexPids := flag.Bool("hex", false, "print PIDs in hexadecimal instead of decimal")

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
	
	conf := Conf{}
	conf.sortBy = *sortBy
	conf.reverseOrder = *reverseOrder
	if *printHexPids {
		conf.printPid = printHexPid
	} else {
		conf.printPid = printDecimalPid
	}

	listThreads(parser, conf, os.Stdout)
}

func listThreads(parser tdformat.Parser, conf Conf, out io.Writer) {
	// TODO: print header
	var threads []tdformat.Thread
	sorter := NewSortableThreads(nil, conf.sortBy, conf.reverseOrder)

	for parser.NextThread() {
		if parser.SwitchedDump() {
			printThreads(threads[0:], &sorter, conf, out)
			threads = make([]tdformat.Thread, 0)
		}

		threads = append(threads, parser.Thread())
	}

	// Flush the recorded list of threads
	//sorter = NewSortableThreads(threads, sortBy, reverseOrder)
	printThreads(threads[0:], &sorter, conf, out)
}

func printThreads(threads []tdformat.Thread, sorter *SortableThreads, conf Conf, out io.Writer) {
	if len(threads) < 1 {
		return
	}

	sorter.ChangeThreads(threads)
	sort.Sort(sorter)

	for _, thread := range threads {
		fmt.Fprintf(out, "%-35v %v ",
			thread.Name,
			convertState(thread.State))

		conf.printPid(thread, out)
		out.Write([]byte("\n"))
	}
}

func printDecimalPid(thread tdformat.Thread, out io.Writer) {
	fmt.Fprintf(out, "%6v", thread.Pid)
}
func printHexPid(thread tdformat.Thread, out io.Writer) {
	fmt.Fprintf(out, "0x%-4x", thread.Pid)
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

