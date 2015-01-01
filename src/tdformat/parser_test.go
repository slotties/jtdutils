package tdformat

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"strings"
	"io"
)

func TestNewParser(t *testing.T) {
	reader := strings.NewReader("")
	parser := NewParser(reader)

	assert.NotNil(t, parser)
}

func TestDumpSwitching(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
"main1" prio=1 tid=0x10000000094ce800 nid=0xccc in Object.wait() [0x000000000ce3e000]

2014-12-31 11:01:49
Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):
"main2" prio=1 tid=0x10000000094ce800 nid=0xccc in Object.wait() [0x000000000ce3e000]

2015-06-24 10:12:45
Full thread dump foo bar:
"foo" daemon prio=1 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
`)
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	// First dump ("anonymous" => without a header line)
	thread, reachedNewDump, err := parser.NextThread()
	assert.True(t, reachedNewDump)
	assert.Nil(t, err)
	assert.Equal(t, "D3D Screen Updater", thread.Name)
	assert.Equal(t, "0", parser.Dump().Id)
	thread, reachedNewDump, err = parser.NextThread()
	assert.False(t, reachedNewDump)
	assert.Equal(t, "main1", thread.Name)
	assert.Equal(t, "0", parser.Dump().Id)

	// Second dump
	thread, reachedNewDump, err = parser.NextThread()
	assert.True(t, reachedNewDump)
	assert.Nil(t, err)
	assert.Equal(t, "main2", thread.Name)
	assert.Equal(t, "2014-12-31 11:01:49", parser.Dump().Id)
	assert.Equal(t, "Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode)", parser.Dump().InfoLine)

	// Third dump
	thread, reachedNewDump, err = parser.NextThread()
	assert.True(t, reachedNewDump)
	assert.Nil(t, err)
	assert.Equal(t, "foo", thread.Name)
	assert.Equal(t, "2015-06-24 10:12:45", parser.Dump().Id)
	assert.Equal(t, "foo bar", parser.Dump().InfoLine)
}

func TestNextThread_Single(t *testing.T) {
	reader := strings.NewReader("\"D3D Screen Updater\" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]")
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	thread, _, err := parser.NextThread()
	assert.NotNil(t, thread)
	assert.Nil(t, err)

	assert.Equal(t, "D3D Screen Updater", thread.Name)
	assert.Equal(t, "0xe2c", thread.Nid)
	assert.Equal(t, "0x00000000094ce800", thread.Tid)
	assert.Equal(t, true, thread.Daemon)
	assert.Equal(t, 8, thread.Priority)

	thread, _, err = parser.NextThread()
	assert.NotNil(t, thread)
	assert.Equal(t, io.EOF, err)
}

func TestNextThread_Multiple(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
"main" prio=1 tid=0x10000000094ce800 nid=0xccc in Object.wait() [0x000000000ce3e000]
`)
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	thread, _, err := parser.NextThread()
	assert.NotNil(t, thread)
	assert.Nil(t, err)
	assert.Equal(t, "D3D Screen Updater", thread.Name)
	assert.Equal(t, "0xe2c", thread.Nid)
	assert.Equal(t, "0x00000000094ce800", thread.Tid)
	assert.Equal(t, true, thread.Daemon)
	assert.Equal(t, 8, thread.Priority)

	thread, _, err = parser.NextThread()
	assert.NotNil(t, thread)
	assert.Equal(t, "main", thread.Name)
	assert.Equal(t, "0xccc", thread.Nid)
	assert.Equal(t, "0x10000000094ce800", thread.Tid)
	assert.Equal(t, false, thread.Daemon)
	assert.Equal(t, 1, thread.Priority)

	thread, _, err = parser.NextThread()
	assert.NotNil(t, thread)
	assert.Equal(t, io.EOF, err)
}

func TestStacktrace(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	- waiting on <0x00000000c0092b98> (a java.lang.Object)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	- locked <0x00000000c0092b98> (a java.lang.Object)
	at java.lang.Thread.run(Thread.java:745)
`)
	parser := NewParser(reader)

	assert.NotNil(t, parser)

	thread, _, _ := parser.NextThread()
	assert.NotNil(t, thread)
	assert.NotNil(t, thread.Stacktrace)
	assert.Equal(t, 3, len(thread.Stacktrace))

	line := thread.Stacktrace[0]
	assert.Equal(t, "java.lang.Object.wait", line.MethodName)
	assert.True(t, line.Native)
	assert.Equal(t, "", line.FileName)
	assert.Equal(t, 0, line.LineNumber)

	line = thread.Stacktrace[1]
	assert.Equal(t, "sun.java2d.d3d.D3DScreenUpdateManager.run", line.MethodName)
	assert.False(t, line.Native)
	assert.Equal(t, "D3DScreenUpdateManager.java", line.FileName)
	assert.Equal(t, 432, line.LineNumber)

	line = thread.Stacktrace[2]
	assert.Equal(t, "java.lang.Thread.run", line.MethodName)
	assert.False(t, line.Native)
	assert.Equal(t, "Thread.java", line.FileName)
	assert.Equal(t, 745, line.LineNumber)
}

func TestLocks(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	- waiting on <0x00000000c0092b98> (a java.lang.Object)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	- locked <0x00000000c0092b98> (a a.b.C)
	at java.lang.Thread.run(Thread.java:745)
`)
	parser := NewParser(reader)

	assert.NotNil(t, parser)

	thread, _, _ := parser.NextThread()
	assert.NotNil(t, thread)
	assert.NotNil(t, thread.Locks)
	assert.Equal(t, 2, len(thread.Locks))

	lock := thread.Locks[0]
	assert.Equal(t, "java.lang.Object", lock.ClassName)
	assert.False(t, lock.Holds)
	assert.Equal(t, "0x00000000c0092b98", lock.Address)

	lock = thread.Locks[1]
	assert.Equal(t, "a.b.C", lock.ClassName)
	assert.True(t, lock.Holds)
	assert.Equal(t, "0x00000000c0092b98", lock.Address)
}