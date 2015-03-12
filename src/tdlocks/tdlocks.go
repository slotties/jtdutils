package main

import (
	"fmt"
	"tdformat"
	"flag"
	"os"
	"io"
	"tdtool"
)

type Locks struct {
	dump *tdformat.ThreadDump
	lockedThreads map[string]LockedThreads
}
type LockedThreads struct {
	holder tdformat.Thread
	holdedLock tdformat.Lock
	waitingThreads []tdformat.Thread
}

type PrintPidFunc func(thread tdformat.Thread, out io.Writer)

type Conf struct {
	sortBy string
	reverseOrder bool
	printPid PrintPidFunc
}

func main() {
	minWaitingThreads := flag.Int("w", 0, "the minimum amount of waiting threads to be listed")
	lockObjectClass := flag.String("c", "", "a filter on the lock object class")

	parser := tdtool.InitTool()
	parser.ParseStacktrace = false
	
	listLocks(parser, *minWaitingThreads, *lockObjectClass, os.Stdout)
}

func listLocks(parser tdformat.Parser, minWaitingThreads int, lockObjectClass string, out io.Writer) {
	locks := Locks{}

	for parser.NextThread() {
		if parser.SwitchedDump() {
			printLocks(locks, minWaitingThreads, out)

			dump := parser.Dump()
			locks = Locks{
				&dump,
				make(map[string]LockedThreads, 0),
			}
		}

		thread := parser.Thread()
		if len(thread.Locks) > 0 {
			for _, lock := range thread.Locks {
				if lockObjectClass == "" || lockObjectClass == lock.ClassName {
					rememberLock(&locks, &lock, &thread)
				}
			}
		}
	}

	printLocks(locks, minWaitingThreads, out)
}

func rememberLock(locks *Locks, lock *tdformat.Lock, thread *tdformat.Thread) {
	lockedThreads, _ := locks.lockedThreads[lock.Address]

	if lock.Holds {
		lockedThreads.holder = *thread
		lockedThreads.holdedLock = *lock
	} else {
		lockedThreads.waitingThreads = append(lockedThreads.waitingThreads, *thread)
	}

	locks.lockedThreads[lock.Address] = lockedThreads
}

func printLocks(locks Locks, minWaitingThreads int, out io.Writer) {
	if len(locks.lockedThreads) < 1 {
		return
	}

	fmt.Fprintf(out, "Dump: %v\n", locks.dump.Id)
	for _, lock := range locks.lockedThreads {
		if len(lock.waitingThreads) >= minWaitingThreads {
			fmt.Fprintf(out, "\"%v\" holds %v (%v)\n", lock.holder.Name, lock.holdedLock.Address, lock.holdedLock.ClassName)
			for _, thread := range lock.waitingThreads {
				fmt.Fprintf(out, "- %v\n", thread.Name)
			}
			fmt.Fprint(out, "\n")
		}
	}
}
