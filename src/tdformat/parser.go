package tdformat

import (
	"io"
	"bufio"
	"strings"
	"strconv"
)

type Parser struct {
	scanner *bufio.Scanner
	lastLine string

	currentDump ThreadDump
	nextDump ThreadDump
	switchedDump bool

	currentThread Thread
	lastError error

	ParseStacktrace bool
	ParseLocks bool
}

func NewParser(r io.Reader) Parser {
	scanner := bufio.NewScanner(r)
	return Parser{ 
		scanner,
		"",
		ThreadDump{},
		ThreadDump{},
		false,
		Thread{},
		nil,
		true,
		true,
	}
}

func (self *Parser) Dump() (ThreadDump) {
	return self.currentDump
}

func (self *Parser) Thread() (Thread) {
	return self.currentThread
}

func (self *Parser) SwitchedDump() bool {
	return self.switchedDump
}

func (self *Parser) Err() (error) {
	return self.lastError
}

// FIXME: simplify this whole bunch of code
func (self *Parser) NextThread() (bool) {
	self.currentThread = Thread{}
	// Check if a header line was cached on the last Next() call and parse it now.
	if self.lastLine != "" {
		parseThreadHeader(&self.currentThread, self.lastLine)
		self.lastLine = ""
	}
	// Adopt an already reached next dump.
	if self.nextDump.Id != "" {
		self.currentDump = self.nextDump
		self.nextDump = ThreadDump{}
	}

	prevLine := ""
	self.switchedDump = false

	for self.scanner.Scan() {
		line := self.scanner.Text()
		if isThreadHeader(line) {
			// We found a thread but have not reached a dump yet. Start an anonymous dump then.
			if self.currentDump.Id == "" {
				self.currentDump = generateDump(self.currentDump)
				self.switchedDump = true
			}
			// A new dump started without forewarning (no new line between the dumps). We have to remember/cache
			// this line in case Next() is called again. Otherwise we would forget this new dump.
			if self.currentThread.Name != "" {
				self.lastLine = line
				return true
			} else {
				parseThreadHeader(&self.currentThread, line)
			}
		} else if self.ParseStacktrace && isCodeLine(line) {
			parseCodeLine(&self.currentThread, line)
		} else if self.ParseLocks && isLock(line) {
			parseLockLine(&self.currentThread, line)
		} else if isState(line) {
			parseStateLine(&self.currentThread, line)
		} else if isThreadEnd(line) && self.currentThread.Name != "" {
			return true
		} else if isDumpStart(line) {
			dump := parseDumpHeader(line, prevLine)
			// It may happen that we are already parsing a thread and happen to reach a new dump. In this case we
			// remember the dump and set it as current dump in the next call of Next().
			if self.currentThread.Name != "" {
				self.nextDump = dump
				return true
			} else {
				self.currentDump = dump
				self.switchedDump = true
			}
		} else {
			prevLine = line
		}
	}

	if self.currentThread.Name == "" {
		self.lastError = io.EOF
		return false
	} else {
		return true
	}
}

func isThreadHeader(line string) bool {
	return strings.HasPrefix(line, "\"")
}

func isCodeLine(line string) bool {
	return strings.HasPrefix(line, "\tat")
}

func isThreadEnd(line string) bool {
	return line == ""
}

func isLock(line string) bool {
	return strings.HasPrefix(line, "\t-")
}

func isDumpStart(line string) bool {
	return strings.HasPrefix(line, "Full thread dump")
}

func isState(line string) bool {
	return strings.HasPrefix(line, "   java.lang.Thread.State:")
}

func generateDump(lastDump ThreadDump) ThreadDump {
	return ThreadDump {
		"",
		"0",
	}
}

func parseThreadHeader(thread *Thread, line string) {
	// Example: "D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]

	thread.Name = stringBetween(line, "\"", "\"")
	thread.Nid = stringBetween(line, "nid=", " ")
	thread.Tid = stringBetween(line, "tid=", " ")
	thread.Daemon = strings.Contains(line, "daemon")
	thread.Priority, _ = strconv.Atoi(stringBetween(line, "prio=", " "))
	thread.Stacktrace = make([]CodeLine, 0)
	thread.Locks = make([]Lock, 0)
	thread.State = THREAD_UNKNOWN
}

func parseCodeLine(thread *Thread, line string) {
	/* Examples:
	at java.util.concurrent.locks.LockSupport.park(LockSupport.java:186)
	at java.lang.Object.wait(Native Method)
	*/
	codeLine := CodeLine{}

	codeLine.MethodName = stringBetween(line, "at ", "(")
	codeLine.Native = strings.Contains(line, "Native Method")
	if !codeLine.Native {
		codeLine.FileName = stringBetween(line, "(", ":")
		codeLine.LineNumber, _ = strconv.Atoi(stringBetween(line, ":", ")"))
	}

	thread.Stacktrace = append(thread.Stacktrace, codeLine)
}

func parseLockLine(thread *Thread, line string) {
	/* Examples:
	- waiting on <0x00000000c0092b98> (a java.lang.Object)
	- locked <0x00000000c0092b98> (a java.lang.Object)
	*/
	lock := Lock{}

	lock.ClassName = stringBetween(line, "a ", ")")
	lock.Address = stringBetween(line, "<", ">")
	lock.Holds = strings.Contains(line, "locked <")
	
	thread.Locks = append(thread.Locks, lock)
}

func parseStateLine(thread *Thread, line string) {
	if strings.Contains(line, "RUNNABLE") {
		thread.State = THREAD_RUNNING
	} else if strings.Contains(line, "TIMED_WAITING (on object monitor)") {
		thread.State = THREAD_TIMED_WAITING
	} else if strings.Contains(line, "WAITING (on object monitor)") {
		thread.State = THREAD_WAITING
	} else if strings.Contains(line, "WAITING (parking)") {
		thread.State = THREAD_PARKED
	} else if strings.Contains(line, "BLOCKED (on object monitor)") {
		thread.State = THREAD_BLOCKED
	}
}

func parseDumpHeader(headerLine string, prevLine string) ThreadDump {
	/*
	headerLine example: Full thread dump Java HotSpot(TM) 64-Bit Server VM (24.65-b04 mixed mode):
	prevLine example: 2014-12-31 11:01:49
	*/

	infoLine := stringBetween(headerLine, "dump ", ":")
	id := prevLine

	return ThreadDump{ infoLine, id }
}