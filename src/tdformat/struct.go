package tdformat

/*
	Here are all structures regarding thread dump formats, such as dumps or stacktraces.
*/

// TODO: define type for states
const (
	THREAD_UNKNOWN = -1

	THREAD_RUNNING = 0
	// Object.wait()
	THREAD_WAITING = 1
	// Object.wait(long) (with timeout)
	THREAD_TIMED_WAITING = 2
	THREAD_PARKED = 3
	// Waiting at a synchronized block
	THREAD_BLOCKED = 4
)

type ThreadDump struct {
	// TODO: time
	InfoLine string
	Id string
	// TODO: index?
}

type Thread struct {
	Name string
	Nid string
	Tid string
	Daemon bool
	Priority int
	State int

	Stacktrace []CodeLine
	Locks []Lock
}

type CodeLine struct {
	MethodName string
	FileName string
	LineNumber int
	Native bool
}

type Lock struct {
	ClassName string
	Address string
	Holds bool
}