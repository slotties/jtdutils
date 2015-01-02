package tdformat

import (
	"testing"
	"github.com/stretchr/testify/assert"
	"os"
)

func TestOpenEmptyFileName(t *testing.T) {
	fileName := ""
	reader, err := OpenFile(&fileName)
	assert.Nil(t, err)
	assert.Equal(t, os.Stdin, reader)
}

func TestOpenNil(t *testing.T) {
	reader, err := OpenFile(nil)
	assert.Nil(t, err)
	assert.Equal(t, os.Stdin, reader)
}

func TestOpenExistingFile(t *testing.T) {
	fileName := "setenv.bat"
	reader, err := OpenFile(&fileName)
	defer reader.Close()

	assert.NotNil(t, err)
	assert.NotEqual(t, os.Stdin, reader)
}

func TestOpenMissingFile(t *testing.T) {
	fileName := "foo.txt"
	_, err := OpenFile(&fileName)
	assert.NotNil(t, err)
}
