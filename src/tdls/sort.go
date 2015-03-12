package main

import (
	"tdformat"
	"strings"
)


type LessFunc func(a *tdformat.Thread, b *tdformat.Thread) bool

type SortableThreads struct {
	threads []tdformat.Thread
	lessFn LessFunc
	descending bool
}

func NewSortableThreads(threads []tdformat.Thread, sortBy string, descending bool) SortableThreads {
	return SortableThreads{
		threads,
		resolveLessFunc(sortBy),
		descending,
	}
}

func resolveLessFunc(sortBy string) LessFunc {
	switch sortBy {
		case "name":
			return compareName
		case "pid":
			return comparePid
		case "state":
			return compareState
		default:
			return nil
	}
}

func (self *SortableThreads) ChangeThreads(threads []tdformat.Thread) {
	self.threads = threads
}

func (self SortableThreads) Len() int {
	return len(self.threads)
}

func (self SortableThreads) Less(i int, j int) bool {
	if self.descending {
		i, j = j, i
	}

	if self.lessFn != nil {
		return self.lessFn(&self.threads[i], &self.threads[j])
	} else {
		// Keep the natural order in case no less func is defined
		return i < j
	}
}

func (self SortableThreads) Swap(i int, j int) {
	self.threads[i], self.threads[j] = self.threads[j], self.threads[i]
}

func compareName(a *tdformat.Thread, b *tdformat.Thread) bool {
	return strings.ToLower(a.Name) < strings.ToLower(b.Name)
}

func comparePid(a *tdformat.Thread, b *tdformat.Thread) bool {
	return a.Pid < b.Pid
}

func compareState(a *tdformat.Thread, b *tdformat.Thread) bool {
	return a.State < b.State
}