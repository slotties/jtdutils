package tdstats

import (
	"fmt"
	"tdformat"
)

type StateStats struct {
	dump tdformat.ThreadDump
	stats map[int]int
}

func main() {
	fmt.Printf("yo\n")
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