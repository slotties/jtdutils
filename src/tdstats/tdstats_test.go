package tdstats

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"strings"
	"tdformat"
)

func TestSingleDump(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"TimerQueue" daemon prio=6 tid=0x000000000c0fa800 nid=0xd04 waiting on condition [0x000000000dffe000]
"Thread-1" prio=6 tid=0x00000000092e3800 nid=0x1730 in Object.wait() [0x000000000a60f000]
"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
"x" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
`)
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	stats := allStats(parser)
	assert.NotNil(t, stats)
	assert.Equal(t, 1, len(stats))
	assert.Equal(t, 2, stats[0].stats[tdformat.THREAD_RUNNING])
	assert.Equal(t, 1, stats[0].stats[tdformat.OBJECT_WAIT])
	assert.Equal(t, 1, stats[0].stats[tdformat.WAITING_ON_CONDITION])
	// TODO: test all the other states
}