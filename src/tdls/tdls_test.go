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

	listThreads(parser, Conf{ "", false, printDecimalPid, }, &out)
	assert.Equal(t, `
AWT-Windows                         B   6052
AWT-Shutdown                        W   2868
a                                   P      3
zZ                                  R   6052
ZZ                                  T   2044
`, "\n" + out.String())
}

func TestOrderByName(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: BLOCKED (on object monitor)
"zZ" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"a" prio=1 tid=0x2 nid=0x3 waiting on condition [0x4]
   java.lang.Thread.State: WAITING (parking)
"AWT-Shutdown" prio=6 tid=0x0000000007e31800 nid=0xb34 in Object.wait() [0x0000000009aef000]
   java.lang.Thread.State: WAITING (on object monitor)
"ZZ" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listThreads(parser, Conf{ "name", false, printDecimalPid, }, &out)
	assert.Equal(t, `
a                                   P      3
AWT-Shutdown                        W   2868
AWT-Windows                         B   6052
zZ                                  R   6052
ZZ                                  T   2044
`, "\n" + out.String())
}

func TestOrderByPid(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: BLOCKED (on object monitor)
"zZ" daemon prio=6 tid=0x0000000007e88800 nid=0x17a5 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"a" prio=1 tid=0x2 nid=0x3 waiting on condition [0x4]
   java.lang.Thread.State: WAITING (parking)
"AWT-Shutdown" prio=6 tid=0x0000000007e31800 nid=0xb34 in Object.wait() [0x0000000009aef000]
   java.lang.Thread.State: WAITING (on object monitor)
"ZZ" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listThreads(parser, Conf{ "pid", false, printDecimalPid, }, &out)
	assert.Equal(t, `
a                                   P      3
ZZ                                  T   2044
AWT-Shutdown                        W   2868
AWT-Windows                         B   6052
zZ                                  R   6053
`, "\n" + out.String())
}

func TestOrderByState(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: BLOCKED (on object monitor)
"zZ" daemon prio=6 tid=0x0000000007e88800 nid=0x17a5 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"a" prio=1 tid=0x2 nid=0x3 waiting on condition [0x4]
   java.lang.Thread.State: WAITING (parking)
"AWT-Shutdown" prio=6 tid=0x0000000007e31800 nid=0xb34 in Object.wait() [0x0000000009aef000]
   java.lang.Thread.State: WAITING (on object monitor)
"ZZ" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listThreads(parser, Conf{ "state", false, printDecimalPid, }, &out)
	assert.Equal(t, `
zZ                                  R   6053
AWT-Shutdown                        W   2868
ZZ                                  T   2044
a                                   P      3
AWT-Windows                         B   6052
`, "\n" + out.String())
}

func TestPrintHexPids(t *testing.T) {
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

	listThreads(parser, Conf{ "", false, printHexPid, }, &out)
	assert.Equal(t, `
AWT-Windows                         B 0x17a4
AWT-Shutdown                        W 0xb34 
a                                   P 0x3   
zZ                                  R 0x17a4
ZZ                                  T 0x7fc 
`, "\n" + out.String())
}