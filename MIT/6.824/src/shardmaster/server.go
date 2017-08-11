package shardmaster

import "net"
import "fmt"
import "net/rpc"
import "log"

import "paxos"
import "sync"
import "sync/atomic"
import "os"
import "syscall"
import "encoding/gob"
import "math/rand"
import "sort"
import "time"

const (
	OP_JOIN = "Join"
	OP_LEAVE = "Leave"
	OP_MOVE = "Move"
	OP_QUERY = "Query"

	BaseSleep = 5 * time.Millisecond
)

type ShardMaster struct {
	mu         sync.Mutex
	l          net.Listener
	me         int
	dead       int32 // for testing
	unreliable int32 // for testing
	px         *paxos.Paxos

	configs []Config // indexed by config num
	// Your definitions here.
	lastExecSeq int
	idleGIDsPool []int64 // idle servers GID pool, stored by time order
	reqCount int
}

type Op struct {
	// Your data here.
	Operation string // Join/Leave/Move/Query
	GID int64 // Any args that contains GID should use this filed
	Servers []string // Any args that contains Servers should use this filed
	ShardNum int // Any args that contains Shard should use this filed
	ConfigNum int // Any args that contains Num for config should use this filed
	ReqID string // Used for checking if two Ops are same.
}
// Sleep for a while to give Paxos some time to reach an agreement
func WaitForPending(sleep time.Duration) time.Duration {
	time.Sleep(sleep)
	limit := 2 * time.Second
	if sleep < limit {
		sleep *= 2
	}
	return sleep
}

func (sm *ShardMaster) GenerateReqID() string {
	newReqID := fmt.Sprintf("%s.%d", sm.me, sm.reqCount)
	sm.reqCount ++
	return newReqID
}
// Wraps up the Shards array into a map that 
// maps GID to a set of Shard Indices.
func WrapShards2Replicas(Shards [NShards]int64) map[int64][]int {
	replicas := make(map[int64][]int) // gid -> a set of {shards}, one to many
	for i, _ := range Shards {
		replicas[Shards[i]] = append(replicas[Shards[i]], i)
	}
	for gid, _ := range replicas {
		sort.Ints(replicas[gid]) // sort shard indices in the same GID
	}
	return replicas
}
// Unwraps the mapping between replica and its shards back
// into an array of Shards, each of which's component contains a GID
func UnwrapReplicas2Shards(replicas map[int64][]int) [NShards]int64 {
	shards := [NShards]int64{} // index of shard -> gid, many to one
	for gid, _ := range replicas {
		for _, shardIdx := range replicas[gid] {
			shards[shardIdx] = gid
		}
	}
	return shards
}
// Check if all the shards have not been assigned to any replica
func ShardsNotAssigned(Shards [NShards]int64) bool {
	for _, gid := range Shards {
		if gid != 0 {
			return false
		}
	}
	return true
}

// Check if each shard is been assigned to a different replica, already
func ShardsAllDifferent(Shards [NShards]int64) bool {
	replicas := WrapShards2Replicas(Shards)
	return len(replicas) == len(Shards)
}
// Given the new config of which GIDs are being used, generate the corresponding newGroups
// Prerequisite: for any GID in GIDs[] such that GID > 0, curGroups[GID] exists
func UpdateGroupsByGIDs(Shards [NShards]int64, idleGIDsPool[]int64, curGroups map[int64][]string) map[int64][]string {
	newGroups := make(map[int64][]string)
	for _, gid := range Shards {
		if _, exists := newGroups[gid]; !exists && gid > 0 {
			newGroups[gid] = curGroups[gid]
		}
	}
	for _, gid := range idleGIDsPool {
		if _, exists := newGroups[gid]; !exists && gid > 0 {
			newGroups[gid] = curGroups[gid]
		}
	}
	return newGroups
}
// Advance to a new configuration
func (sm *ShardMaster) AdvanceToNewConfig(Num int, Shards [NShards]int64, Groups map[int64][]string) {
	newConfig := Config{Num:Num, Shards:Shards, Groups:Groups}
	sm.configs = append(sm.configs, newConfig)
}
// The last config in shardmaster
func (sm *ShardMaster) LatestConfig() Config {
	return sm.configs[len(sm.configs)-1]
}
// Check if there is any idle replicas stored
func (sm *ShardMaster) IdleGroupsEmpty() bool {
	return len(sm.idleGIDsPool) == 0
}
// Add new replica to idle groups pool
func (sm *ShardMaster) AddIntoIdlePool(GID int64) {
	gidIdx := IndexOfGID(GID, sm.idleGIDsPool)
	if gidIdx == -1 { // GID has not appeared previously
		sm.idleGIDsPool = append(sm.idleGIDsPool, GID) // record new replica into idle servers
	}
}
// Remove a replica group from idle pool
func (sm *ShardMaster) RemoveFromIdlePool(GID int64) {
	gidIdx := IndexOfGID(GID, sm.idleGIDsPool)
	if gidIdx > -1 {
		sm.idleGIDsPool = append(sm.idleGIDsPool[:gidIdx], sm.idleGIDsPool[(gidIdx+1):]...) // remove replica from pool
	}
}
// Given the current configuration, calculate its optimal load balance
// replicas: a map that maps gid to a set of shards. must be valid, i.e., shards cannot be unassigned
func BalanceLoad(replicas map[int64][]int) map[int64][]int {
	if len(replicas) == 0 || len(replicas) > NShards {
		DPrintf("Configuration size incorrect, BalanceLoad not working!\n") // how to throw an error o_0?
	}
	// I don't even want to describe how this function works, heartbroken...
	configSize := len(replicas) // how many replicas are in current configuration
	numTotalShards := 0 // just for checking
	num2GidMap := make(map[int][]int64) // maps num of shards -> a set of {gid}, SET NEED TO BE ORDERED BY NUM!!!
	sortedShardNums := []int{} // descending order of num of shards each replic has in current config
	resultReplicas := make(map[int64][]int) // the result configuration of how to assign shards in replicas

	for gid, _ := range replicas {
		resultReplicas[gid] = replicas[gid] // copy the original config of replicas
		numInGID := len(replicas[gid]) // number of shards for this GID
		numTotalShards += numInGID 
		num2GidMap[numInGID] = append(num2GidMap[numInGID], gid)
		sortedShardNums = append(sortedShardNums, numInGID)
	}

	for nums, _ := range num2GidMap {
		sort.Sort(Int64Slice(num2GidMap[nums])) // sort GIDs that hold the same amount of replicas
	}
	
	sort.Sort(sort.Reverse(sort.IntSlice(sortedShardNums))) // terrible syntax
	// calculate the optimal load config, that is, how many shards each replica should have ideally
	optimalLoad := make([]int, configSize)
	for i := 0; i < configSize; i++ { // init to 0
		optimalLoad[i] = 0
	}
	for i := 0; i < numTotalShards; i++ {
		optimalLoad[i % configSize] += 1
	}
	// calculate how to transfer shards from current config to optimal config
	deltaNums := make([]int, configSize)
	for i := 0; i < configSize; i++ {
		deltaNums[i] = optimalLoad[i] - sortedShardNums[i]
	}
	// aligned by index, delta2Gid[i] should remove/append deltaNums[i] shards
	delta2Gid := make([]int64, configSize)
	for i:= 0; i < configSize; {
		nums := sortedShardNums[i]
		for _, gid := range num2GidMap[nums] {
			delta2Gid[i] = gid
			i ++
		}
	}
	// make the transfer of shards among replicas
	shardsPool := []int{}
	for i:= 0; i < configSize; i++ {
		gid := delta2Gid[i]
		if deltaNums[i] < 0 {
			// delta < 0, replica should give out shards
			numToRelease := -deltaNums[i]
			shardsPool = append(shardsPool, resultReplicas[gid][:numToRelease]...)
			resultReplicas[gid] = resultReplicas[gid][numToRelease:]
		} else if deltaNums[i] > 0 {
			// delta > 0, replica should receive shards
			numToReceive := deltaNums[i]
			resultReplicas[gid] = append(resultReplicas[gid], shardsPool[:numToReceive]...)
			shardsPool = shardsPool[numToReceive:]
		}
	}
	// at this point, @shardsPool should be empty
	return resultReplicas
}
// Execute the pxVal instance to local database, then update lastExecSeq to seq
// Once the Op is executed to local, we can make done of this seq
// precondition: Paxos.log.index(op) == seq
func (sm *ShardMaster) ExecuteLog(seq int, pxVal Op) {
	if seq <= sm.lastExecSeq {
		return
	}
	switch pxVal.Operation {
	case OP_JOIN:
		sm.ExecuteJoin(pxVal.GID, pxVal.Servers)
	case OP_LEAVE:
		sm.ExecuteLeave(pxVal.GID)
	case OP_MOVE:
		sm.ExecuteMove(pxVal.GID, pxVal.ShardNum)
	case OP_QUERY:
		// no need to do anything
	}
	sm.px.Done(seq)
	sm.lastExecSeq = seq
}
// Execute Join Paxos instance
func (sm *ShardMaster) ExecuteJoin(GID int64, Servers []string) bool {
	curConfig := sm.LatestConfig()
	curNum, curShards, curGroups := curConfig.Num, curConfig.Shards, CopyGroups(curConfig.Groups)
	// Duplicate detection
	if _, gidExists := curGroups[GID]; gidExists {
		return false
	}
	newNum, newShards := curNum+1, curShards
	curGroups[GID] = Servers

	if ShardsNotAssigned(curShards) {
		// special case: previously no replica in the config
		for i, _ := range newShards {
			newShards[i] = GID
		}
	} else if ShardsAllDifferent(curShards) {
		// each shard has been assigned to a different replica
		sm.AddIntoIdlePool(GID)
		newShards = curShards
	} else {
		newReplicas := WrapShards2Replicas(curShards)
		newReplicas[GID] = []int{} // add in the joining GID into new config
		newReplicas = BalanceLoad(newReplicas)
		newShards = UnwrapReplicas2Shards(newReplicas)
	}
	newGroups := UpdateGroupsByGIDs(newShards, sm.idleGIDsPool , curGroups)

	sm.AdvanceToNewConfig(newNum, newShards, newGroups)
	DPrintf("Server: %d, New config after GID: %d, Join: %v\n", sm.me, GID, sm.LatestConfig())
	return true
}
// Execute Leave Paxos instance
func (sm *ShardMaster) ExecuteLeave(GID int64) bool {
	curConfig := sm.LatestConfig()
	curNum, curShards, curGroups := curConfig.Num, curConfig.Shards, CopyGroups(curConfig.Groups)
	// Valid detection
	if ShardsNotAssigned(curShards) {
		return false
	} else if _, gidExists := curGroups[GID]; !gidExists {
		return false
	}
	newNum, newShards, newGroups := curNum+1, curShards, CopyGroups(curGroups)
	delete(newGroups, GID)

	if len(newGroups) == 0 {
		DPrintf("leave: no replica left\n")
		// special case: previously there is only one replica in the config
		for i, _ := range newShards {
			newShards[i] = 0
		}
	} else if IndexOfGID(GID, sm.idleGIDsPool) != -1 {
		// we are removing an idle replica
		sm.RemoveFromIdlePool(GID)
		newShards = curShards
	} else {
		// removing a replica that has been assigned to shard(s)
		newReplicas := WrapShards2Replicas(curShards)
		releasedShards := newReplicas[GID]
		delete(newReplicas, GID) // remove the GID and its shards
		
		if !sm.IdleGroupsEmpty() {
			// assign the released shards to an idle server 
			idleGid := sm.idleGIDsPool[0]
			sm.RemoveFromIdlePool(idleGid)
			newReplicas[idleGid] = releasedShards
		} else {
			// append the released shards to some new GID
			// DONT ASSIGN TO THE FIRST KEY! Order of iteration in Go's MAP is SILLY RANDOMIZED!!!
			restGids := []int64{}
			for gidKey, _ := range newReplicas {
				restGids = append(restGids, gidKey)
			}
			sort.Sort(Int64Slice(restGids)) // so ugly...
			gidKey := restGids[0]
			newReplicas[gidKey] = append(newReplicas[gidKey], releasedShards...)
		}
		newReplicas = BalanceLoad(newReplicas)
		newShards = UnwrapReplicas2Shards(newReplicas)
	}
	newGroups = UpdateGroupsByGIDs(newShards, sm.idleGIDsPool , curGroups)

	sm.AdvanceToNewConfig(newNum, newShards, newGroups)
	DPrintf("Server: %d, New config after GID: %d, Leave: %v\n", sm.me, GID, sm.LatestConfig())
	return true
}
// Execute Move Paxos instance
func (sm *ShardMaster) ExecuteMove(GID int64, ShardNum int) bool {
	curConfig := sm.LatestConfig()
	curNum, curShards, curGroups := curConfig.Num, curConfig.Shards, CopyGroups(curConfig.Groups)
	// Valid detection
	if _, gidExists := curGroups[GID]; 
		!gidExists || curShards[ShardNum] == GID || 
		IndexOfGID(GID, sm.idleGIDsPool) !=-1 || ShardsNotAssigned(curShards) {
		return false
	}
	newNum, newShards := curNum+1, curShards
	newShards[ShardNum] = GID
	newGroups := UpdateGroupsByGIDs(newShards, sm.idleGIDsPool , curGroups)

	sm.AdvanceToNewConfig(newNum, newShards, newGroups)
	DPrintf("Server: %d, New config after Shard: %d -> GID: %d, Move: %v\n", sm.me, ShardNum, GID, sm.LatestConfig())
	return true
}
// Try to add a log entry (instance) to Paxos at seq slot.
// Return the instance in seq slot. 
// Why "Try"?
// The return op might not be what we want to add. This is most likely to 
// happen when our server died and restarted. Then this will then act as 
// a way for kvpaxos server to catch up all the missed instances.
func (sm *ShardMaster) TryAddLog(seq int, pxVal Op) Op {
	sm.px.Start(seq, pxVal)
	var result Op
	done, toSleep := false, BaseSleep
	
	for !done && !sm.isdead() {
		decided, _ := sm.px.Status(seq)
		switch decided {
		case paxos.Decided:
			done = true
		case paxos.Pending:
			toSleep = WaitForPending(toSleep)
		}
	}
	_, value := sm.px.Status(seq)
	if value != nil {
		result = value.(Op)
	}
	return result
}
 // Checks if two Op isntaces are the same
func AreOpsSame(op1 Op, op2 Op) bool {
	return op1.ReqID == op2.ReqID
}
// Add a new Op to paxos log as a new instance. This method will not 
// terminate until it's certain the passed in Op is inserted successfully.
func (sm *ShardMaster) AddOpToPaxos(pxVal Op) {
	done := false
	for !done && !sm.isdead() {
		seq := sm.lastExecSeq + 1
		DPrintf("Server: %d add Op to Paxos, Op: %v\n", sm.me, pxVal)
		result := sm.TryAddLog(seq, pxVal)
		// if this seq(instance) agrees on other value, then ReqID will not be the same
		done = AreOpsSame(result, pxVal)
		DPrintf("Server: %d hear back op from Paxos, Op: %v, succeeded: %v\n", sm.me, result, done)
		// execute what we get from Paxos
		sm.ExecuteLog(seq, result)
		//pxVal.Shards = sm.LatestConfig().Shards
	}
}
// RPC Handler for Join
func (sm *ShardMaster) Join(args *JoinArgs, reply *JoinReply) error {
	// Your code here.
	sm.mu.Lock()
	defer sm.mu.Unlock()

	pxVal := Op{Operation:OP_JOIN, GID:args.GID, Servers:args.Servers, ReqID:sm.GenerateReqID()}
	sm.AddOpToPaxos(pxVal)

	return nil
}
// RPC Handler for Leave
func (sm *ShardMaster) Leave(args *LeaveArgs, reply *LeaveReply) error {
	// Your code here.
	sm.mu.Lock()
	defer sm.mu.Unlock()

	pxVal := Op{Operation:OP_LEAVE, GID:args.GID, ReqID:sm.GenerateReqID()}
	sm.AddOpToPaxos(pxVal)

	return nil
}
// RPC Handler for Move
func (sm *ShardMaster) Move(args *MoveArgs, reply *MoveReply) error {
	// Your code here.
	sm.mu.Lock()
	defer sm.mu.Unlock()

	pxVal := Op{Operation:OP_MOVE, GID:args.GID, ShardNum:args.Shard, ReqID:sm.GenerateReqID()}
	sm.AddOpToPaxos(pxVal)

	return nil
}
// RPC Handler for Query
func (sm *ShardMaster) Query(args *QueryArgs, reply *QueryReply) error {
	// Your code here.
	sm.mu.Lock()
	defer sm.mu.Unlock()

	pxVal := Op{Operation:OP_QUERY, ConfigNum:args.Num, ReqID:sm.GenerateReqID()}
	sm.AddOpToPaxos(pxVal)

	configIdx := args.Num
	if configIdx == -1 || configIdx >= len(sm.configs) {
		configIdx = len(sm.configs) - 1
	}
	reply.Config = sm.configs[configIdx]
	
	return nil
}

// please don't change these two functions.
func (sm *ShardMaster) Kill() {
	atomic.StoreInt32(&sm.dead, 1)
	sm.l.Close()
	sm.px.Kill()
}

func (sm *ShardMaster) isdead() bool {
	return atomic.LoadInt32(&sm.dead) != 0
}

// please do not change these two functions.
func (sm *ShardMaster) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&sm.unreliable, 1)
	} else {
		atomic.StoreInt32(&sm.unreliable, 0)
	}
}

func (sm *ShardMaster) isunreliable() bool {
	return atomic.LoadInt32(&sm.unreliable) != 0
}

//
// servers[] contains the ports of the set of
// servers that will cooperate via Paxos to
// form the fault-tolerant shardmaster service.
// me is the index of the current server in servers[].
//
func StartServer(servers []string, me int) *ShardMaster {
	sm := new(ShardMaster)
	sm.me = me

	sm.configs = make([]Config, 1)
	sm.configs[0].Groups = make(map[int64][]string)
	for i, _ := range sm.configs[0].Shards {
		// init to Gid 0 (invalid) for all Shards
		sm.configs[0].Shards[i] = 0
	}
	sm.idleGIDsPool = []int64{}
	sm.reqCount = 0

	rpcs := rpc.NewServer()

	gob.Register(Op{})
	rpcs.Register(sm)
	sm.px = paxos.Make(servers, me, rpcs)

	os.Remove(servers[me])
	l, e := net.Listen("unix", servers[me])
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	sm.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for sm.isdead() == false {
			conn, err := sm.l.Accept()
			if err == nil && sm.isdead() == false {
				if sm.isunreliable() && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if sm.isunreliable() && (rand.Int63()%1000) < 200 {
					// process the request but force discard of reply.
					c1 := conn.(*net.UnixConn)
					f, _ := c1.File()
					err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
					if err != nil {
						fmt.Printf("shutdown: %v\n", err)
					}
					go rpcs.ServeConn(conn)
				} else {
					go rpcs.ServeConn(conn)
				}
			} else if err == nil {
				conn.Close()
			}
			if err != nil && sm.isdead() == false {
				fmt.Printf("ShardMaster(%v) accept: %v\n", me, err.Error())
				sm.Kill()
			}
		}
	}()

	return sm
}
