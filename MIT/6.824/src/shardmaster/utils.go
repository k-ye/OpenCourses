package shardmaster

import "fmt"

const Debug = 0

func DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		fmt.Printf(format, a...)
	}
	return
}
// Sort interface of Int64 Slice
type Int64Slice []int64

func (a Int64Slice) Len() int {
	return len(a) 
}

func (a Int64Slice) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}

func (a Int64Slice) Less(i, j int) bool {
	return a[i] < a[j] 
}
// Copy a group map, b/c maps are passed by ref in Go
func CopyGroups(groups map[int64][]string) map[int64][]string {
	groupsCopy := make(map[int64][]string)
	for k, v := range groups {
		groupsCopy[k] = v // slices are copy assigned
	}
	return groupsCopy
}
// Find the index of a given GID in slice
func IndexOfGID(GID int64, GIDSlice []int64) int {
	for i, val := range GIDSlice {
		if val == GID {
			return i
		}
	}
	return -1
}