package main

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"strings"
	"tdformat"
	"bytes"
)

func TestNaturalOrder(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: BLOCKED (on object monitor)
"AWT-Shutdown" prio=6 tid=0x0000000007e31800 nid=0xb34 in Object.wait() [0x0000000009aef000]
   java.lang.Thread.State: WAITING (on object monitor)
"a" prio=1 tid=0x2 nid=0x3 waiting on condition [0x4]
   java.lang.Thread.State: WAITING (parking)
"zZ" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"ZZ" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listThreads(parser, &out)
	assert.Equal(t, `
AWT-Windows                         B 0x17a4 0x0000000007e88800
AWT-Shutdown                        W 0xb34  0x0000000007e31800
a                                   P 0x3    0x2               
zZ                                  R 0x17a4 0x0000000007e88800
ZZ                                  T 0x7fc  0x00000000092e4000
`, "\n" + out.String())
}