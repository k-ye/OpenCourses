package raft

import (
	"fmt"
	"sync"
)

//
// Thread safe logging helpers
//

var gPrintMu sync.Mutex

func TsPrintf(format string, a ...interface{}) {
	gPrintMu.Lock()
	fmt.Printf(format, a...)
	gPrintMu.Unlock()
}

func TsPrintln(line string) {
	gPrintMu.Lock()
	fmt.Println(line)
	gPrintMu.Unlock()
}

func Check(cond bool, format string, a ...interface{}) {
	if !cond {
		panic(fmt.Sprintf(format, a...))
	}
}

func Min(x, y int) int {
	if x < y {
		return x
	}
	return y
}

func Max(x, y int) int {
	if x > y {
		return x
	}
	return y
}