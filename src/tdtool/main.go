package tdtool

import (
	"flag"
	"os"
	"tdformat"
	"fmt"
)

func InitTool() tdformat.Parser {
	showHelp := flag.Bool("h", false, "shows this help")
	fileName := flag.String("f", "", "the file to use (stdin is used per default)")

	flag.Parse()

	if *showHelp {
		flag.PrintDefaults()
		os.Exit(0)
	}

	reader, err := tdformat.OpenFile(fileName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Could not open file '%v': %v\n", *fileName, err)
		os.Exit(1)
	}
	defer reader.Close()

	return tdformat.NewParser(reader)
}