package tdformat

import (
	"io"
	"bufio"
	"strings"
	"strconv"
)

type Parser struct {
	reader *bufio.Reader
	lastLine string
}

func NewParser(r io.Reader) Parser {
	bufferedReader := bufio.NewReaderSize(r, 8 * 1024)
	return Parser{ bufferedReader, "", }
}

func (self *Parser) Next() (Thread, bool, error) {
	var lineBytes []byte
	var err error
	err = nil

	thread := Thread{}
	// Check if a header line was cached on the last Next() call and parse it now.
	if self.lastLine != "" {
		parseThreadHeader(&thread, self.lastLine)
		self.lastLine = ""
	}

	for err == nil {
		lineBytes, _, err = self.reader.ReadLine()
		line := string(lineBytes)
		if isThreadHeader(line) {
			// A new dump started without forewarning (no new line between the dumps). We have to remember/cache
			// this line in case Next() is called again. Otherwise we would forget this new dump.
			if thread.name != "" {
				self.lastLine = line
				return thread, true, nil
			} else {
				parseThreadHeader(&thread, line)
			}
		} else if isCodeLine(line) {
			parseCodeLine(&thread, line)
		} else if isLock(line) {
			parseLockLine(&thread, line)
		} else if isThreadEnd(line) {
			return thread, thread.name != "", nil
		}
	}

	return thread, thread.name != "", io.EOF
}

func isThreadHeader(line string) bool {
	return strings.HasPrefix(line, "\"")
}

func isCodeLine(line string) bool {
	return strings.HasPrefix(line, "\tat")
}

func isThreadEnd(line string) bool {
	return line == "\n"
}

func isLock(line string) bool {
	return strings.HasPrefix(line, "\t-")
}

func parseThreadHeader(thread *Thread, line string) {
	// Example: "D3D Screen Updater" daemon prio=8 tid=0x00000000094ce800 nid=0xe2c in Object.wait() [0x000000000ce3e000]

	thread.name = stringBetween(line, "\"", "\"")
	thread.nid = stringBetween(line, "nid=", " ")
	thread.tid = stringBetween(line, "tid=", " ")
	thread.daemon = strings.Contains(line, "daemon")
	thread.priority, _ = strconv.Atoi(stringBetween(line, "prio=", " "))
	thread.stacktrace = make([]CodeLine, 0)
	thread.locks = make([]Lock, 0)
	// TODO: state
}

func parseCodeLine(thread *Thread, line string) {
	/* Examples:
	at java.util.concurrent.locks.LockSupport.park(LockSupport.java:186)
	at java.lang.Object.wait(Native Method)
	*/
	codeLine := CodeLine{}

	codeLine.methodName = stringBetween(line, "at ", "(")
	codeLine.native = strings.Contains(line, "Native Method")
	if !codeLine.native {
		codeLine.fileName = stringBetween(line, "(", ":")
		codeLine.lineNumber, _ = strconv.Atoi(stringBetween(line, ":", ")"))
	}

	thread.stacktrace = append(thread.stacktrace, codeLine)
}

func parseLockLine(thread *Thread, line string) {
	/* Examples:
	- waiting on <0x00000000c0092b98> (a java.lang.Object)
	- locked <0x00000000c0092b98> (a java.lang.Object)
	*/
	lock := Lock{}

	lock.className = stringBetween(line, "a ", ")")
	lock.address = stringBetween(line, "<", ">")
	lock.holds = strings.Contains(line, "locked <")
	
	thread.locks = append(thread.locks, lock)
}