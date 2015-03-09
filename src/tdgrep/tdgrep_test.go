package main

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"tdformat"
)

func TestIsMatching_Name_Exact(t *testing.T) {
	exactMatch, _ := createFilters("foo")

	fooThread := tdformat.Thread{}
	fooThread.Name = "foo"
	barThread := tdformat.Thread{}
	barThread.Name = "bar"

	assert.True(t, isMatching(&fooThread, &exactMatch))
	assert.False(t, isMatching(&barThread, &exactMatch))
}

func TestIsMatching_Name_Wildcard(t *testing.T) {
	exactMatch, _ := createFilters("foo.*")

	fooThread := tdformat.Thread{}
	fooThread.Name = "foo"
	foo2Thread := tdformat.Thread{}
	foo2Thread.Name = "foo2"
	barThread := tdformat.Thread{}
	barThread.Name = "bar"

	assert.True(t, isMatching(&fooThread, &exactMatch))
	assert.True(t, isMatching(&foo2Thread, &exactMatch))
	assert.False(t, isMatching(&barThread, &exactMatch))
}

