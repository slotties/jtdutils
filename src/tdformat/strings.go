package tdformat

import (
	"strings"
)

func stringBetween(text string, startStr string, endStr string) string {
	startIdx := strings.Index(text, startStr)
	if startIdx < 0 {
		return ""
	}

	text = text[startIdx + len(startStr) : ]
	endIdx := strings.Index(text, endStr)
	if endIdx < 0 {
		return ""
	}

	return text[:endIdx]
}