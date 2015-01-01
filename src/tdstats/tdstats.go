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
	flag.Parse()

fmt.Printf("reading %v\n", *fileName)
	reader, err := newReader(*fileName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Could not open file '%v': %v\n", *fileName, err)
		os.Exit(1)
	}
	parser := tdformat.NewParser(reader)
	stats := allStats(parser)
	printStats(stats, os.Stdout)
}

func newReader(fileName string) (io.Reader, error) {
	if fileName == "" {
		return os.Stdin, nil
	} else {
		return  os.Open(fileName)
	}
}

func printStats(allStats []StateStats, out io.Writer) {
	fmt.Fprintf(out, `
                                   |  RUN | WAIT | TIMED_WAIT | PARK | BLOCK
-----------------------------------|------|------|------------|------|-------
`)

	for _, stats := range allStats {
		fmt.Fprintf(out, "%-35v %5v  %5v        %5v  %5v   %5v\n", 
			stats.dump.Id,
			stats.stats[tdformat.THREAD_RUNNING],
			stats.stats[tdformat.THREAD_WAITING],
			stats.stats[tdformat.THREAD_TIMED_WAITING],
			stats.stats[tdformat.THREAD_PARKED],
			stats.stats[tdformat.THREAD_BLOCKED])
	}
}

func allStats(parser tdformat.Parser) []StateStats {
	stats := make([]StateStats, 0)
	var currentStats StateStats
	var thread tdformat.Thread
	var switchedDump bool
	var err error

	for err == nil {
		thread, switchedDump, err = parser.NextThread()
		if switchedDump {
			currentStats = newStats(parser.Dump())
			stats = append(stats, currentStats)
		} else {
			currentStats.stats[thread.State]++
		}
	}

	return stats
}

func newStats(dump tdformat.ThreadDump) StateStats {
	stats := make(map[int]int, 1)
	stats[tdformat.THREAD_RUNNING] = 0

	return StateStats{
		dump,
		stats,
	}
}