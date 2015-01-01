package tdformat

/*
	Here are all structures regarding thread dump formats, such as dumps or stacktraces.
*/

const (
	THREAD_RUNNING = 0
	// TODO: all the other states
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