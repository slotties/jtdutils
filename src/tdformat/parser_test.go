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

func TestNext_Single(t *testing.T) {
	reader := strings.NewReader("\"D3D Screen Updater\" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]")
	parser := NewParser(reader)

	assert.NotNil(t, parser)

	thread, threadFound, err := parser.Next()
	assert.NotNil(t, thread)
	assert.Equal(t, true, threadFound)
	// Expect an EOF because it was actually the end of the reader.
	assert.Equal(t, io.EOF, err)

	assert.Equal(t, "D3D Screen Updater", thread.name)
	assert.Equal(t, "0xe2c", thread.nid)
	assert.Equal(t, "0x00000000094ce800", thread.tid)
	assert.Equal(t, true, thread.daemon)
	assert.Equal(t, 8, thread.priority)
}

func TestNext_Multiple(t *testing.T) {
	reader := strings.NewReader(`
"D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]
"main" prio=1 tid=0x10000000094ce800 nid=0xccc in Object.wait() [0x000000000ce3e000]
`)
	parser := NewParser(reader)

	assert.NotNil(t, parser)

	thread, threadFound, err := parser.Next()
	assert.NotNil(t, thread)
	assert.Equal(t, true, threadFound)
	assert.Nil(t, err)
	assert.Equal(t, "D3D Screen Updater", thread.name)
	assert.Equal(t, "0xe2c", thread.nid)
	assert.Equal(t, "0x00000000094ce800", thread.tid)
	assert.Equal(t, true, thread.daemon)
	assert.Equal(t, 8, thread.priority)

	thread, threadFound, err = parser.Next()
	assert.NotNil(t, thread)
	assert.Equal(t, true, threadFound)
	assert.Equal(t, io.EOF, err)
	assert.Equal(t, "main", thread.name)
	assert.Equal(t, "0xccc", thread.nid)
	assert.Equal(t, "0x10000000094ce800", thread.tid)
	assert.Equal(t, false, thread.daemon)
	assert.Equal(t, 1, thread.priority)

	thread, threadFound, err = parser.Next()
	assert.NotNil(t, thread)
	assert.Equal(t, false, threadFound)
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

	thread, threadFound, _ := parser.Next()
	assert.NotNil(t, thread)
	assert.Equal(t, true, threadFound)
	assert.NotNil(t, thread.stacktrace)
	assert.Equal(t, 3, len(thread.stacktrace))

	line := thread.stacktrace[0]
	assert.Equal(t, "java.lang.Object.wait", line.methodName)
	assert.True(t, line.native)
	assert.Equal(t, "", line.fileName)
	assert.Equal(t, 0, line.lineNumber)

	line = thread.stacktrace[1]
	assert.Equal(t, "sun.java2d.d3d.D3DScreenUpdateManager.run", line.methodName)
	assert.False(t, line.native)
	assert.Equal(t, "D3DScreenUpdateManager.java", line.fileName)
	assert.Equal(t, 432, line.lineNumber)

	line = thread.stacktrace[2]
	assert.Equal(t, "java.lang.Thread.run", line.methodName)
	assert.False(t, line.native)
	assert.Equal(t, "Thread.java", line.fileName)
	assert.Equal(t, 745, line.lineNumber)
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

	thread, found, _ := parser.Next()
	assert.NotNil(t, thread)
	assert.Equal(t, true, found)
	assert.NotNil(t, thread.locks)
	assert.Equal(t, 2, len(thread.locks))

	lock := thread.locks[0]
	assert.Equal(t, "java.lang.Object", lock.className)
	assert.False(t, lock.holds)
	assert.Equal(t, "0x00000000c0092b98", lock.address)

	lock = thread.locks[1]
	assert.Equal(t, "a.b.C", lock.className)
	assert.True(t, lock.holds)
	assert.Equal(t, "0x00000000c0092b98", lock.address)
}