package diskv

import "net"
import "fmt"
import "net/rpc"
import "log"
import "time"
import "paxos"
import "sync"
import "sync/atomic"
import "os"
import "syscall"
import "encoding/gob"
import "encoding/base32"
import "math/rand"
import "shardmaster"
import "io/ioutil"
import "strconv"

import "bytes"

const (
	Debug     = 1
	BaseSleep = 20 * time.Millisecond
	MaxSleep  = 2 * time.Second
)

func DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		fmt.Printf(format, a...)
	}
	return
}

type Op struct {
	// Your definitions here.
	Operation string // Join/Leave/Move/Query
	// For Get/Put/Append
	Key   string
	Value string
	ReqID string // request ID given by Clerk: "uid.Operation.reqNum"
	// For Reconfig
	Config    shardmaster.Config // information about the new config
	Database  map[string]string  // contains only k/v need to be transfered
	MaxReqIDs map[string]int64   // contains only clerk states need to be transfered
	// Private Usage
	OpID string // For comparing if two Ops are the same, format: "DisKV_ID.opNum"
}

type DisKV struct {
	mu         sync.Mutex
	l          net.Listener
	me         int
	dead       int32 // for testing
	unreliable int32 // for testing
	sm         *shardmaster.Clerk
	px         *paxos.Paxos
	dir        string // each replica has its own data directory

	gid int64 // my replica group ID

	// Your definitions here.
	database    map[string]string  // k/v database
	maxReqIDs   map[string]int64   // maps clerk.uid to max ReqID seen by this uid
	lastExecSeq int                // last executed Paxos sequence(instance) number
	opCounter   int64              // counter of how many Ops have been started by this kvserver
	config      shardmaster.Config // current config used by this kvserver
}

func (kv *DisKV) putShardIndices() error {
	w := new(bytes.Buffer)
	e := gob.NewEncoder(w)
	shards := []int{}
	for shard, gid := range kv.config.Shards {
		if gid == kv.gid {
			shards = append(shards, shard)
		}
	}
	DPrintf("Put shard indices: %v\n", shards)
	//e.Encode(len(shards))
	e.Encode(shards)
	content:= string(w.Bytes())

	tempname := kv.dir + "temp-shards"
	fullname := kv.dir + "full-shards"

	if err := ioutil.WriteFile(tempname, []byte(content), 0666); err != nil {
		return err
	}
	if err := os.Rename(tempname, fullname); err != nil {
		return err
	}
	return nil
}

func (kv *DisKV) getShardIndices() ([]int, error) {
	fullname := kv.dir + "full-shards"
	content, err := ioutil.ReadFile(fullname)
	buf, result := string(content), []int{}

	if err != nil {
		return result, err
	}

	r := bytes.NewBuffer([]byte(buf))
	d := gob.NewDecoder(r)
	d.Decode(&result)

	DPrintf("Get shard indices: %v\n", result)
	return result, err
}

//
// these are handy functions that might be useful
// for reading and writing key/value files, and
// for reading and writing entire shards.
// puts the key files for each shard in a separate
// directory.
//

func (kv *DisKV) shardDir(shard int) string {
	d := kv.dir + "/shard-" + strconv.Itoa(shard) + "/"
	// create directory if needed.
	_, err := os.Stat(d)
	if err != nil {
		if err := os.Mkdir(d, 0777); err != nil {
			log.Fatalf("Mkdir(%v): %v", d, err)
		}
	}
	return d
}

// cannot use keys in file names directly, since
// they might contain troublesome characters like /.
// base32-encode the key to get a file name.
// base32 rather than base64 b/c Mac has case-insensitive
// file names.
func (kv *DisKV) encodeKey(key string) string {
	return base32.StdEncoding.EncodeToString([]byte(key))
}

func (kv *DisKV) decodeKey(filename string) (string, error) {
	key, err := base32.StdEncoding.DecodeString(filename)
	return string(key), err
}

// read the content of a key's file.
func (kv *DisKV) fileGet(shard int, key string) (string, error) {
	fullname := kv.shardDir(shard) + "/key-" + kv.encodeKey(key)
	content, err := ioutil.ReadFile(fullname)
	return string(content), err
}

// replace the content of a key's file.
// uses rename() to make the replacement atomic with
// respect to crashes.
func (kv *DisKV) filePut(shard int, key string, content string) error {
	fullname := kv.shardDir(shard) + "/key-" + kv.encodeKey(key)
	tempname := kv.shardDir(shard) + "/temp-" + kv.encodeKey(key)
	if err := ioutil.WriteFile(tempname, []byte(content), 0666); err != nil {
		return err
	}
	if err := os.Rename(tempname, fullname); err != nil {
		return err
	}
	return nil
}

// return content of every key file in a given shard.
func (kv *DisKV) fileReadShard(shard int) map[string]string {
	m := map[string]string{}
	d := kv.shardDir(shard)
	files, err := ioutil.ReadDir(d)
	if err != nil {
		log.Fatalf("fileReadShard could not read %v: %v", d, err)
	}
	for _, fi := range files {
		n1 := fi.Name()
		if n1[0:4] == "key-" {
			key, err := kv.decodeKey(n1[4:])
			if err != nil {
				log.Fatalf("fileReadShard bad file name %v: %v", n1, err)
			}
			content, err := kv.fileGet(shard, key)
			if err != nil {
				log.Fatalf("fileReadShard fileGet failed for %v: %v", key, err)
			}
			m[key] = content
		}
	}
	return m
}

// replace an entire shard directory.
func (kv *DisKV) fileReplaceShard(shard int, m map[string]string) {
	d := kv.shardDir(shard)
	os.RemoveAll(d) // remove all existing files from shard.
	for k, v := range m {
		kv.filePut(shard, k, v)
	}
}

/** Helper Functions **/

// Sleep for a while to give Paxos some time to reach an agreement
func WaitForPending(sleep time.Duration) time.Duration {
	time.Sleep(sleep)
	if sleep < MaxSleep {
		sleep *= 2
	}
	return sleep
}

// Generate a unique Op ID for comparing two Ops
func (kv *DisKV) GenerateOpID() string {
	newOpID := fmt.Sprintf("%d.%d", kv.me, kv.opCounter)
	kv.opCounter++
	return newOpID
}

// Update the clerk's ReqID to the maximum in the given dict
func UpdateReqIDInDict(reqID string, dict *map[string]int64) {
	uid, _, reqNum := ParseReqID(reqID)
	val, ok := (*dict)[uid]
	if !ok || val < reqNum {
		(*dict)[uid] = reqNum
	}
}

// Update the seen reqID for certain clerk's uid to prevent re-execution, if it is the max.
func (kv *DisKV) UpdateReqID(reqID string) {
	UpdateReqIDInDict(reqID, &kv.maxReqIDs)
}

// Check to see if a given ReqID has been seen by this kv server
func (kv *DisKV) HasSeenReqID(reqID string) bool {
	uid, _, reqNum := ParseReqID(reqID)
	val, ok := kv.maxReqIDs[uid]
	if !ok {
		return false
	} else {
		return reqNum <= val
	}
}

// Test to see if a key is in a given shard
func KeyBelongsShard(key string, shard int) bool {
	return key2shard(key) == shard
}

// Check to see if this kv is responsible for the given key under current config
func (kv *DisKV) ResponsibleForKey(key string) bool {
	shard := key2shard(key)
	return (kv.gid == kv.config.Shards[shard])
}

// Check to see if Config Op we are trying to add is still valid.
// For preventing stale config
func (kv *DisKV) EnsureReconfigOpValid(pxOp *Op) string {
	if pxOp.Config.Num < kv.config.Num {
		return ErrStaleConfig
	}
	return OK
}

// Check to see if Clerk's Op (Get/Put/Append) we are trying to add is still valid.
// For at-most-once semantics and shard-server checking.
func (kv *DisKV) EnsureClerkOpValid(pxOp *Op) string {
	reqID, key := pxOp.ReqID, pxOp.Key
	if kv.HasSeenReqID(reqID) {
		return ErrHasDone
	} else if !kv.ResponsibleForKey(key) {
		return ErrWrongGroup
	}
	return OK
}

// Check to see if the Op we are trying to add is still valid.
func (kv *DisKV) EnsureOpValid(pxOp Op) string {
	op := pxOp.Operation
	switch op {
	case Reconfig:
		return kv.EnsureReconfigOpValid(&pxOp)
	case Get, Put, Append:
		return kv.EnsureClerkOpValid(&pxOp)
	}
	return OK
}

/** Helper Functions End **/

/** Core Functions **/
// execute Put/Append operation
func (kv *DisKV) ExecutePutAppend(pxOp Op) {
	op, key, value, reqID := pxOp.Operation, pxOp.Key, pxOp.Value, pxOp.ReqID
	switch op {
	case Put:
		kv.database[key] = value
	case Append:
		kv.database[key] += value
	}
	kv.filePut(key2shard(key), key, kv.database[key])
	DPrintf("%d KV execute %s for reqID: %s, opID: %s, key: %s, value: %s, database: %v\n", kv.me, reqID, pxOp.OpID, op, key, value, kv.database)
}

func (kv *DisKV) ExecuteReconfig(pxOp Op) {
	DPrintf("%d KV execute Reconfig for opID: %s, config: %v\n", kv.me, pxOp.OpID, pxOp.Config)
	// Failed here many times due to update to a stale config under Unreliable testcase,
	// although I don't have any clue why this happened as I checked to see if config
	// is stale or not before I add paxos instance into the log.
	if kv.config.Num >= pxOp.Config.Num {
		return
	}

	for key, val := range pxOp.Database {
		kv.database[key] = val
	}
	// update the maxReqIDs from other kv srvs, notice that here ID is a pure Int
	for uid, maxReqID := range pxOp.MaxReqIDs {
		reqID, exists := kv.maxReqIDs[uid]
		if !exists || reqID < maxReqID {
			kv.maxReqIDs[uid] = maxReqID
		}
	}

	kv.config = pxOp.Config

	kv.putShardIndices()
	kv.getShardIndices()
}

// Execute the pxOp instance to local database, then update lastExecSeq to seq
// Once the Op is executed to local, we can make done of this seq
// precondition: Paxos.log.index(pxOp) == seq
func (kv *DisKV) ExecuteLog(pxOp Op, seq int) {
	// we don't need to worry about re-execute and violate at-most-once
	// as this is been checked in Get()/Put()/Append()
	if seq <= kv.lastExecSeq {
		return
	}

	op, reqID := pxOp.Operation, pxOp.ReqID
	DPrintf("%d KV execute for seq: %d, opID: %s, op: %s\n", kv.me, seq, pxOp.OpID, pxOp.Operation)
	switch op {
	case Get, Nop:
		// do nothing
	case Put, Append:
		kv.ExecutePutAppend(pxOp)
	case Reconfig:
		kv.ExecuteReconfig(pxOp)
	}
	// update if Op is coming from Client
	if op == Get || op == Put || op == Append {
		kv.UpdateReqID(reqID)
	}
	kv.px.Done(seq)
	kv.lastExecSeq = seq
}

// Try to add a log entry (instance) to Paxos at seq slot.
// Return the instance in seq slot.
// Why "Try"?
// The return op might not be what we want to add. This is most likely to
// happen when our server died and restarted. Then this will then act as
// a way for kvpaxos server to catch up all the missed instances.
func (kv *DisKV) TryAddLog(pxOp Op, seq int) Op {
	kv.px.Start(seq, pxOp)
	var result Op
	done, toSleep := false, BaseSleep
	DPrintf("%d kvshard add log in seq: %d, op: %s, OpID: %s\n", kv.me, seq, pxOp.Operation, pxOp.OpID)
	for !done && !kv.isdead() {
		decided, value := kv.px.Status(seq)
		if decided == paxos.Decided {
			result = value.(Op)
			done = true
			break
		} else {
			toSleep = WaitForPending(toSleep)
		}
	}
	return result
}

// Add a new Op to paxos log as a new instance. This method will not
// terminate until it's certain the passed in Op is inserted successfully.
func (kv *DisKV) AddOpToPaxos(pxOp Op) string {
	done := false
	for !done && !kv.isdead() {
		seq := kv.lastExecSeq + 1
		err := kv.EnsureOpValid(pxOp)
		if err != OK {
			return err
		}
		result := kv.TryAddLog(pxOp, seq)
		// if this seq(instance) agrees on other value, then OpID will not be the same
		done = (result.OpID == pxOp.OpID)
		// execute what we get from Paxos
		kv.ExecuteLog(result, seq)
	}
	return OK
}

// Function for CollectShardsForConfig RPC handler, the srvr being called will hand out
// the shard after all the paxos ops, currently inside the log which has effect on this shard,
// have been executed.
func (kv *DisKV) RequestShard(args *RequestShardArgs, reply *RequestShardReply) error {
	// The GID_G(ive) being requested shard will not request shard from other servers, ensured by minimum transfer.
	// This process will execute only if the current config (with Num_G) in kvsrv GID_G is geq the config (with Num_Req) being requested.
	// We can then reverse thinking and see that the kvsrv GID_R(eq) sending out this request will have a config with at most Num_Req - 1,
	// otherwise it should not request for a config (Num_Req) that is leq its own config.Num.
	// Since Num_G >= Num_Req > Num_Req - 1, we know that if this is exectuing, then the config on GID_G must be more updated
	// than that on GID_R. Thus the shards stored on GID_G is indeed the most updated for GID_R.
	// That is, we can safely conclude that the following case is handled correctly:
	//		Request S1 on GID_G (At this point, GID_G sends out S1)
	// -> Put (k, v), where k belongs to S1.
	// If the above case happened while GID_G was still in a config where it was responsible for S1, then GID_G
	// will have no way to know that it should not update S1. However, since GID_G is now in a config where
	// it shouldn't be responsible for S1, this Put will be rejected.
	if args.Confignum <= kv.config.Num {
		kv.mu.Lock()
		// This step is very important!
		// We are using this step to ensure everything currently in the Paxos log will be executed
		// Otherwise, such scenario will happen:
		//		GID1 receives Put(k1, v1)
		// -> GID1 receives RequestShard shard1 from GID2
		// -> GID1 replies GID2 with shard1 immediately without executing Put(k1, v1)
		// -> Since GID1 still has Put(k1, v1) in its Paxos log unexecuted, GID1 will execute it
		// -> GID2 may receive Append(k1, v2) in the new config -> Split Brain!
		kv.AddOpToPaxos(Op{Operation: Nop, OpID: kv.GenerateOpID()})
		// weird?? Why do I need to re-initialize these maps??
		reply.Database = make(map[string]string)
		reply.MaxReqIDs = make(map[string]int64)

		for key, val := range kv.database {
			if KeyBelongsShard(key, args.Shard) {
				reply.Database[key] = val
			}
		}
		for uid, reqID := range kv.maxReqIDs {
			reply.MaxReqIDs[uid] = reqID
		}
		reply.Err = OK

		kv.mu.Unlock()
	} else {
		reply.Err = ErrStaleConfig
	}
	return nil
}

/** Core Functions End **/
// RPC handler for client Get and Append requests
func (kv *DisKV) Get(args *GetArgs, reply *GetReply) error {
	// Your code here.
	// if has seen req, return directly
	// else if not responsible for Shard, ErrWrongGroup
	kv.mu.Lock()
	defer kv.mu.Unlock()

	key, reqID := args.Key, args.ReqID
	DPrintf("%d kvshard receives Get, key: %s, reqID: %s\n", kv.me, args.Key, args.ReqID)

	op := Op{Operation: Get, Key: key, ReqID: reqID, OpID: kv.GenerateOpID()}
	err := kv.EnsureClerkOpValid(&op)
	if err == ErrHasDone {
		reply.Err = OK
		reply.Value = kv.database[key]
	} else if err == ErrWrongGroup {
		reply.Err = err
		reply.Value = ""
	} else {
		err = kv.AddOpToPaxos(op)
		if err == OK || err == ErrHasDone {
			reply.Err = OK
			reply.Value = kv.database[key]
		} else if err == ErrWrongGroup {
			reply.Err = err
			reply.Value = ""
		}
	}

	return nil
}

// RPC handler for client Put and Append requests
func (kv *DisKV) PutAppend(args *PutAppendArgs, reply *PutAppendReply) error {
	// Your code here.
	// What if the following scenario happens?
	// Srv: config 3, S1 			||  			Ck: config 5, Put (k, v), where k belongs to S1
	// Three cases:
	// 1. Both (Reconfig 4) and (Reconfig 5) are in its Paxos log: OK, this Put will wait util the previous two Reconfigs have
	//		been applied
	// 2. (Reconfig 4) is in Paxos log, not (Reconfig 5): OK, we will reject Ck's request until a moment where this replica group
	//		learns about config 5
	// 3. Neither (Reconfig 4) nor (Reconfig 5) is in Paxos log: OK, that means shard S1 has never been transfered actually, we can just
	//		operate on it, although I really doubt if this case will happen. Because a majority of kvsrvs in this group will always be
	//		alive, and they will surely advance to (Config 4) as they do not request any shards in config 4. I guess this will happen
	// 		only when we choose a silly large time interval between two tick().
	// if has seen req, return directly
	// else if not responsible for Shard, ErrWrongGroup
	kv.mu.Lock()
	defer kv.mu.Unlock()

	paOp, key, value, reqID := args.Op, args.Key, args.Value, args.ReqID
	DPrintf("%d kvshard receives %s, key: %s, reqID: %s\n", kv.me, args.Op, args.Key, args.ReqID)

	op := Op{Operation: paOp, Key: key, Value: value, ReqID: reqID, OpID: kv.GenerateOpID()}
	err := kv.EnsureClerkOpValid(&op)
	if err == ErrHasDone {
		reply.Err = OK
	} else if err == ErrWrongGroup {
		reply.Err = err
	} else {
		err = kv.AddOpToPaxos(op)
		if err == OK || err == ErrHasDone {
			reply.Err = OK
		} else if err == ErrWrongGroup {
			reply.Err = err
		}
	}
	return nil
}

// During the changing of configuration, we need to collect all the shards.
// A srvr that Requests shard will NEVER Hand Out any shard, and vice versa.
func (kv *DisKV) CollectShardsForConfig(newConfig *shardmaster.Config) (string, map[string]string, map[string]int64) {
	opDatabase, opMaxReqIDs := make(map[string]string), make(map[string]int64)
	if kv.config.Num >= newConfig.Num {
		return ErrStaleConfig, opDatabase, opMaxReqIDs
	}

	DPrintf("%d kvshard collecting shards for config: %d\n", kv.me, newConfig.Num)
	for shard, gid := range newConfig.Shards {
		oldShardGID := kv.config.Shards[shard]
		if gid == kv.gid && oldShardGID != kv.gid {
			// we need to request Shard from replica group: oldShardGID
			args := RequestShardArgs{Shard: shard, Confignum: newConfig.Num}
			var reply RequestShardReply
			done, srvPt := len(kv.config.Groups[oldShardGID]) == 0, 0
			for !done {
				kvsrv := kv.config.Groups[oldShardGID][srvPt]
				reply = RequestShardReply{Database: make(map[string]string), MaxReqIDs: make(map[string]int64)}
				done = call(kvsrv, "DisKV.RequestShard", &args, &reply)

				if done {
					if reply.Err == OK {
						break
					} else {
						return reply.Err, opDatabase, opMaxReqIDs
					}
				}
				srvPt = (srvPt + 1) % len(kv.config.Groups[oldShardGID])
			}

			for key, val := range reply.Database {
				opDatabase[key] = val
			}
			for uid, reqID := range reply.MaxReqIDs {
				fullReqID := fmt.Sprintf("%s.%s.%d", uid, "Dummy", reqID)
				UpdateReqIDInDict(fullReqID, &opMaxReqIDs)
			}
		}
	}

	DPrintf("%d kvshard done collecting\n", kv.me)
	return OK, opDatabase, opMaxReqIDs
}

//
// Ask the shardmaster if there's a new configuration;
// if so, re-configure.
//
func (kv *DisKV) tick() {
	latestConfig := kv.sm.Query(-1)
	kv.mu.Lock()
	defer kv.mu.Unlock()

	for cfgNum := kv.config.Num + 1; cfgNum <= latestConfig.Num; cfgNum++ {
		cfg := kv.sm.Query(cfgNum)
		err, opDatabase, opMaxReqIDs := kv.CollectShardsForConfig(&cfg)
		if err == OK {
			op := Op{Operation: Reconfig, Config: cfg, Database: opDatabase, MaxReqIDs: opMaxReqIDs, OpID: kv.GenerateOpID()}
			kv.AddOpToPaxos(op)
		} else {
			break
		}
	}
}

// tell the server to shut itself down.
// please don't change these two functions.
func (kv *DisKV) kill() {
	atomic.StoreInt32(&kv.dead, 1)
	kv.l.Close()
	kv.px.Kill()
}

func (kv *DisKV) isdead() bool {
	return atomic.LoadInt32(&kv.dead) != 0
}

// please do not change these two functions.
func (kv *DisKV) Setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&kv.unreliable, 1)
	} else {
		atomic.StoreInt32(&kv.unreliable, 0)
	}
}

func (kv *DisKV) isunreliable() bool {
	return atomic.LoadInt32(&kv.unreliable) != 0
}

//
// Start a DisKV server.
// gid is the ID of the server's replica group.
// shardmasters[] contains the ports of the
//   servers that implement the shardmaster.
// servers[] contains the ports of the servers
//   in this replica group.
// Me is the index of this server in servers[].
// dir is the directory name under which this
//   replica should store all its files.
//   each replica is passed a different directory.
// restart is false the very first time this server
//   is started, and true to indicate a re-start
//   after a crash or after a crash with disk loss.
//
func StartServer(gid int64, shardmasters []string,
	servers []string, me int, dir string, restart bool) *DisKV {

	kv := new(DisKV)
	kv.me = me
	kv.gid = gid
	kv.sm = shardmaster.MakeClerk(shardmasters)
	kv.dir = dir

	// Your initialization code here.
	// Don't call Join().
	kv.database = make(map[string]string)
	kv.maxReqIDs = make(map[string]int64)
	kv.opCounter = 0
	kv.lastExecSeq = -1
	if restart {
		DPrintf("OMG it's restarting! I'm gonna fail the lab.\n")
		shardIndices, _ := kv.getShardIndices()
		for _, shard := range shardIndices {
			shardData := kv.fileReadShard(shard)
			for key, val := range shardData {
				kv.database[key] = val
			}
			//DPrintf("shard data: %v\n", shardData)
		}
	}

	// log.SetOutput(ioutil.Discard)

	gob.Register(Op{})

	rpcs := rpc.NewServer()
	rpcs.Register(kv)

	//kv.px = paxos.Make(servers, me, rpcs)
	kv.px = paxos.MakeComplete(servers, me, rpcs, dir, restart)

	// log.SetOutput(os.Stdout)

	os.Remove(servers[me])
	l, e := net.Listen("unix", servers[me])
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	kv.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for kv.isdead() == false {
			conn, err := kv.l.Accept()
			if err == nil && kv.isdead() == false {
				if kv.isunreliable() && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if kv.isunreliable() && (rand.Int63()%1000) < 200 {
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
			if err != nil && kv.isdead() == false {
				fmt.Printf("DisKV(%v) accept: %v\n", me, err.Error())
				kv.kill()
			}
		}
	}()

	go func() {
		for kv.isdead() == false {
			kv.tick()
			time.Sleep(250 * time.Millisecond)
		}
	}()

	return kv
}
