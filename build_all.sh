#!/bin/bash

TOOLS="tdstats tdls tdlocks tdgrep"

echo "Running tests ..."
go test tdformat $TOOLS
if [ "$?" != "0" ]; then
	echo "Tests failed."
	exit 1
fi

for tool in $TOOLS; do
	echo "Building $tool ..."
	go build -o bin/$tool $tool
done
