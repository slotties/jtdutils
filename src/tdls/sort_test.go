package main

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"tdformat"
	"sort"
)

func TestSortByName(t *testing.T) {
	threads := []tdformat.Thread {		
		tdformat.Thread{ "c", 0, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "a", 0, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "z", 0, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "Z", 0, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "A", 0, 0, true, 0, 0, nil, nil, },
	}

	sort.Sort(NewSortableThreads(threads, "name", false ))
	assert.Equal(t, "a", threads[0].Name)
	assert.Equal(t, "A", threads[1].Name)
	assert.Equal(t, "c", threads[2].Name)
	assert.Equal(t, "z", threads[3].Name)
	assert.Equal(t, "Z", threads[4].Name)

	sort.Sort(NewSortableThreads(threads, "name", true ))
	assert.Equal(t, "z", threads[0].Name)
	assert.Equal(t, "Z", threads[1].Name)
	assert.Equal(t, "c", threads[2].Name)
	assert.Equal(t, "a", threads[3].Name)
	assert.Equal(t, "A", threads[4].Name)
}

func TestSortByPid(t *testing.T) {
	threads := []tdformat.Thread {		
		tdformat.Thread{ "c", 1, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "a", 3, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "z", 2, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "Z", 4, 0, true, 0, 0, nil, nil, },
		tdformat.Thread{ "A", 5, 0, true, 0, 0, nil, nil, },
	}

	sort.Sort(NewSortableThreads(threads, "pid", false ))
	assert.Equal(t, 1, threads[0].Pid)
	assert.Equal(t, 2, threads[1].Pid)
	assert.Equal(t, 3, threads[2].Pid)
	assert.Equal(t, 4, threads[3].Pid)
	assert.Equal(t, 5, threads[4].Pid)

	sort.Sort(NewSortableThreads(threads, "pid", true ))
	assert.Equal(t, 5, threads[0].Pid)
	assert.Equal(t, 4, threads[1].Pid)
	assert.Equal(t, 3, threads[2].Pid)
	assert.Equal(t, 2, threads[3].Pid)
	assert.Equal(t, 1, threads[4].Pid)
}

func TestSortByState(t *testing.T) {
	threads := []tdformat.Thread {		
		tdformat.Thread{ "c", 0, 0, true, 0, tdformat.THREAD_RUNNING, nil, nil, },
		tdformat.Thread{ "a", 0, 0, true, 0, tdformat.THREAD_TIMED_WAITING, nil, nil, },
		tdformat.Thread{ "z", 0, 0, true, 0, tdformat.THREAD_BLOCKED, nil, nil, },
		tdformat.Thread{ "Z", 0, 0, true, 0, tdformat.THREAD_WAITING, nil, nil, },
		tdformat.Thread{ "A", 0, 0, true, 0, tdformat.THREAD_PARKED, nil, nil, },
		tdformat.Thread{ "A", 0, 0, true, 0, tdformat.THREAD_UNKNOWN, nil, nil, },
	}

	sort.Sort(NewSortableThreads(threads, "state", false ))
	assert.Equal(t, tdformat.THREAD_UNKNOWN, threads[0].State)
	assert.Equal(t, tdformat.THREAD_RUNNING, threads[1].State)
	assert.Equal(t, tdformat.THREAD_WAITING, threads[2].State)
	assert.Equal(t, tdformat.THREAD_TIMED_WAITING, threads[3].State)
	assert.Equal(t, tdformat.THREAD_PARKED, threads[4].State)
	assert.Equal(t, tdformat.THREAD_BLOCKED, threads[5].State)

	sort.Sort(NewSortableThreads(threads, "state", true ))
	assert.Equal(t, tdformat.THREAD_BLOCKED, threads[0].State)
	assert.Equal(t, tdformat.THREAD_PARKED, threads[1].State)
	assert.Equal(t, tdformat.THREAD_TIMED_WAITING, threads[2].State)
	assert.Equal(t, tdformat.THREAD_WAITING, threads[3].State)
	assert.Equal(t, tdformat.THREAD_RUNNING, threads[4].State)
	assert.Equal(t, tdformat.THREAD_UNKNOWN, threads[5].State)
}