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
	assert.True(t, parser.NextThread())
	thread := parser.Thread()
	assert.True(t, parser.SwitchedDump())
	assert.Nil(t, parser.Err())
	assert.Equal(t, "D3D Screen Updater", thread.Name)
	assert.Equal(t, "0", parser.Dump().Id)
	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.False(t, parser.SwitchedDump())
	assert.Equal(t, "main1", thread.Name)
	assert.Equal(t, "0", parser.Dump().Id)

	// Second dump
	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.True(t, parser.SwitchedDump())
	assert.Nil(t, parser.Err())
	assert.Equal(t, "main2", thread.Name)
	assert.Equal(t, "2014-12-31 11:01:49", parser.Dump().Id)
	assert.Equal(t, "Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode)", parser.Dump().InfoLine)

	// Third dump
	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.True(t, parser.SwitchedDump())
	assert.Nil(t, parser.Err())
	assert.Equal(t, "foo", thread.Name)
	assert.Equal(t, "2015-06-24 10:12:45", parser.Dump().Id)
	assert.Equal(t, "foo bar", parser.Dump().InfoLine)
}

func TestNextThread_Single(t *testing.T) {
	reader := strings.NewReader("\"D3D Screen Updater\" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]")
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	assert.True(t, parser.NextThread())
	thread := parser.Thread()
	assert.NotNil(t, thread)
	assert.Nil(t, parser.Err())

	assert.Equal(t, "D3D Screen Updater", thread.Name)
	assert.Equal(t, "0xe2c", thread.Nid)
	assert.Equal(t, "0x00000000094ce800", thread.Tid)
	assert.Equal(t, true, thread.Daemon)
	assert.Equal(t, 8, thread.Priority)

	assert.False(t, parser.NextThread())
	assert.NotNil(t, parser.Thread())
	assert.Equal(t, io.EOF, parser.Err())
}

func TestNextThread_Multiple(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
"main" prio=1 tid=0x10000000094ce800 nid=0xccc in Object.wait() [0x000000000ce3e000]
`)
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	assert.True(t, parser.NextThread())
	thread := parser.Thread()
	assert.NotNil(t, thread)
	assert.Nil(t, parser.Err())
	assert.Equal(t, "D3D Screen Updater", thread.Name)
	assert.Equal(t, "0xe2c", thread.Nid)
	assert.Equal(t, "0x00000000094ce800", thread.Tid)
	assert.Equal(t, true, thread.Daemon)
	assert.Equal(t, 8, thread.Priority)

	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, "main", thread.Name)
	assert.Equal(t, "0xccc", thread.Nid)
	assert.Equal(t, "0x10000000094ce800", thread.Tid)
	assert.Equal(t, false, thread.Daemon)
	assert.Equal(t, 1, thread.Priority)

	assert.False(t, parser.NextThread())
	assert.NotNil(t, thread)
	assert.Equal(t, io.EOF, parser.Err())
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

	assert.True(t, parser.NextThread())
	thread := parser.Thread()
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

	assert.True(t, parser.NextThread())
	thread := parser.Thread()
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

func TestThreadStates(t *testing.T) {
	reader := strings.NewReader(`
"AWT-Windows" daemon prio=6 tid=0x0000000007e88800 nid=0x17a4 runnable [0x0000000009bef000]
   java.lang.Thread.State: RUNNABLE
"AWT-Shutdown" prio=6 tid=0x0000000007e31800 nid=0xb34 in Object.wait() [0x0000000009aef000]
   java.lang.Thread.State: WAITING (on object monitor)
"a" prio=1 tid=0x2 nid=0x3 waiting on condition [0x4]
   java.lang.Thread.State: WAITING (parking)
"Thread-2" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
"My thread" prio=10 tid=0x00007fffec015800 nid=0x1775 waiting for monitor entry [0x00007ffff15e5000]
   java.lang.Thread.State: BLOCKED (on object monitor)   
`)
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	assert.True(t, parser.NextThread())
	assert.Equal(t, THREAD_RUNNING, parser.Thread().State)

	assert.True(t, parser.NextThread())
	assert.Equal(t, THREAD_WAITING, parser.Thread().State)

	assert.True(t, parser.NextThread())
	assert.Equal(t, THREAD_PARKED, parser.Thread().State)

	assert.True(t, parser.NextThread())
	assert.Equal(t, THREAD_TIMED_WAITING, parser.Thread().State)

	assert.True(t, parser.NextThread())
	assert.Equal(t, THREAD_BLOCKED, parser.Thread().State)
}

func TestSkipStacktraces(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	- waiting on <0x00000000c0092b98> (a foo.bar)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	at java.lang.Thread.run(Thread.java:745)

"Thread-2" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	- locked <0x00000000c0092b98> (a java.lang.Object)
	at java.lang.Thread.run(Thread.java:745)

"My thread" prio=10 tid=0x00007fffec015800 nid=0x1775 waiting for monitor entry [0x00007ffff15e5000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	at java.lang.Thread.run(Thread.java:745)
`)
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	parser.ParseStacktrace = false

	assert.True(t, parser.NextThread())
	thread := parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, 0, len(thread.Stacktrace))
	assert.Equal(t, 1, len(thread.Locks))
	assert.Equal(t, "foo.bar", thread.Locks[0].ClassName)
	assert.Equal(t, "D3D Screen Updater", thread.Name)

	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, 0, len(thread.Stacktrace))
	assert.Equal(t, 1, len(thread.Locks))
	assert.Equal(t, "java.lang.Object", thread.Locks[0].ClassName)
	assert.Equal(t, "Thread-2", thread.Name)

	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, 0, len(thread.Stacktrace))
	assert.Equal(t, 0, len(thread.Locks))
	assert.Equal(t, "My thread", thread.Name)
}

func TestSkipLocks(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	- waiting on <0x00000000c0092b98> (a foo.bar)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	at java.lang.Thread.run(Thread.java:745)

"Thread-2" prio=6 tid=0x00000000092e4000 nid=0x7fc in Object.wait() [0x000000000a70e000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	- locked <0x00000000c0092b98> (a java.lang.Object)
	at java.lang.Thread.run(Thread.java:745)

"My thread" prio=10 tid=0x00007fffec015800 nid=0x1775 waiting for monitor entry [0x00007ffff15e5000]
   java.lang.Thread.State: TIMED_WAITING (on object monitor)
	at java.lang.Object.wait(Native Method)
	at sun.java2d.d3d.D3DScreenUpdateManager.run(D3DScreenUpdateManager.java:432)
	at java.lang.Thread.run(Thread.java:745)
`)
	parser := NewParser(reader)
	assert.NotNil(t, parser)

	parser.ParseLocks = false

	assert.True(t, parser.NextThread())
	thread := parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, 3, len(thread.Stacktrace))
	assert.Equal(t, 0, len(thread.Locks))
	assert.Equal(t, "D3D Screen Updater", thread.Name)

	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, 3, len(thread.Stacktrace))
	assert.Equal(t, 0, len(thread.Locks))
	assert.Equal(t, "Thread-2", thread.Name)

	assert.True(t, parser.NextThread())
	thread = parser.Thread()
	assert.NotNil(t, thread)
	assert.Equal(t, 3, len(thread.Stacktrace))
	assert.Equal(t, 0, len(thread.Locks))
	assert.Equal(t, "My thread", thread.Name)
}