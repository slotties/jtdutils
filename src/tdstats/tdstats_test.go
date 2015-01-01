package main

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

"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"AWT-Shutdown" prio=6 tid=0x0000000007e31800 nid=0xb34 in Object.wait() [0x0000000009aef000]
   java.lang.Thread.State: WAITING (on object monitor)
"a" prio=1 tid=0x2 nid=0x3 waiting on condition [0x4]
   java.lang.Thread.State: WAITING (parking)
"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"Thread-2" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
"My thread" prio=10 tid=0x00007fffec015800 nid=0x1775 waiting for monitor entry [0x00007ffff15e5000]
   java.lang.Thread.State: BLOCKED (on object monitor)
"Thread-2" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
`)
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	stats := allStats(parser)
	assert.NotNil(t, stats)
	assert.Equal(t, 1, len(stats))
	assert.Equal(t, 5, len(stats[0].stats))
	assert.Equal(t, 2, stats[0].stats[tdformat.THREAD_RUNNING])
	assert.Equal(t, 1, stats[0].stats[tdformat.THREAD_WAITING])
	assert.Equal(t, 2, stats[0].stats[tdformat.THREAD_TIMED_WAITING])
	assert.Equal(t, 1, stats[0].stats[tdformat.THREAD_BLOCKED])
	assert.Equal(t, 1, stats[0].stats[tdformat.THREAD_PARKED])
}