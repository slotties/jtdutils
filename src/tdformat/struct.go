package tdformat

/*
	Here are all structures regarding thread dump formats, such as dumps or stacktraces.
*/

type ThreadDump struct {
	// TODO: time
	infoLine string
	id string
	// TODO: index?
}

type Thread struct {
	name string
	nid string
	tid string
	daemon bool
	priority int

	stacktrace []CodeLine
	locks []Lock
}

type CodeLine struct {
	methodName string
	fileName string
	lineNumber int
	native bool
}

type Lock struct {
	className string
	address string
	holds bool
}