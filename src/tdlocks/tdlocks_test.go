package main

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"strings"
	"tdformat"
	"bytes"
)

func TestGeneral(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"holder" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- locked <0x123> (a java.lang.Object)

"w1" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- waiting on <0x123> (a java.lang.Object)

"w2" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- waiting on <0x123> (a java.lang.Object)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listLocks(parser, 0, "", &out)
	assert.Equal(t, `
Dump: 2014-12-31 11:01:49
"holder" holds 0x123 (java.lang.Object)
- w1
- w2

`, "\n" + out.String())
}

/*
	A special test for Object.wait() because the thread is then both locker and waiting.
*/
func TestObjectWait(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- waiting on <0x00000000c0092b98> (a java.lang.Object)
	- locked <0x00000000c0092b98> (a java.lang.Object)

`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listLocks(parser, 0, "", &out)
	assert.Equal(t, `
Dump: 2014-12-31 11:01:49
"D3D Screen Updater" holds 0x00000000c0092b98 (java.lang.Object)
- D3D Screen Updater

`, "\n" + out.String())
}

func TestClassFilter(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"filteredOut" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- locked <0x123> (a java.lang.Object)

"holder1" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- locked <0x456> (a a.b.C)

"holder2" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- locked <0x789> (a a.b.C)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listLocks(parser, 0, "a.b.C", &out)
	assert.Equal(t, `
Dump: 2014-12-31 11:01:49
"holder1" holds 0x456 (a.b.C)

"holder2" holds 0x789 (a.b.C)

`, "\n" + out.String())
}

func TestMinWaiting(t *testing.T) {
	reader := strings.NewReader(`
2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):

"holder" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- locked <0x456> (a java.lang.Object)

"w1" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- waiting on <0x456> (a java.lang.Object)

"w2" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- waiting on <0x456> (a java.lang.Object)

"holder2" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- locked <0x123> (a java.lang.Object)

"w3" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	- waiting on <0x123> (a java.lang.Object)
`)
	
	parser := tdformat.NewParser(reader)
	assert.NotNil(t, parser)

	var out bytes.Buffer

	listLocks(parser, 2, "", &out)
	assert.Equal(t, `
Dump: 2014-12-31 11:01:49
"holder" holds 0x456 (java.lang.Object)
- w1
- w2

`, "\n" + out.String())
}
