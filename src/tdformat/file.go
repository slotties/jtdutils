package tdformat

import (
	"os"
	"io"
)

func OpenFile(fileName *string) (io.ReadCloser, error) {
	if fileName == nil || *fileName == "" {
		return os.Stdin, nil
	} else {
		return os.Open(*fileName)
	}
}