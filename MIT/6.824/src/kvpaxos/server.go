package kvpaxos

import "net"
import "fmt"
import "net/rpc"
import "log"
import "paxos"
import "sync"
import "os"
import "syscall"
import "encoding/gob"
import "math/rand"
import "time"

/*
Originally, when I'm recording the RequestID, I also stored the return value
for that Request if it is Get. This turned out to be consuming too much memory
which failed to pass TestDone(). The scenario is that, if we put a large amount
of Value into some Key and then read the Key multiple times with different
request, then the Value will be stored multiple times for each request.
After I found this, I just changed to use a map to store the RequestID, only.
*/
const (
	Debug = 1

	BaseSleep = 20 * time.Millisecond
	MaxSleep  = 2 * time.Second
)

func DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		log.Printf(format, a...)
	}
	return
}

type Op struct {
	// Your definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	Operation string // Get/Put/Append/Nop
	Key       string
	Value     string
	ReqID     string
}

type KVPaxos struct {
	mu         sync.Mutex
	l          net.Listener
	me         int
	dead       bool // for testing
	unreliable bool // for testing
	px         *paxos.Paxos

	// Your definitions here.
	database       map[string]string // key/value database replica
	lastExecSeq    int               // last executed sequence(instance) number
	recordedReqIDs map[string]bool   // prevent the duplicated requests from being executed
}

// Sleep for a while to give Paxos some time to reach an agreement
func WaitForPending(sleep time.Duration, limit time.Duration) time.Duration {
	time.Sleep(sleep)
	if sleep < limit {
		sleep *= 2
	}
	return sleep
}

// Record a requestID to prevent re-execution.
// This should be called for the first time to exec reqID!
// Cannot store the actual value, otherwise cannot pass TestDone! Mem not released
func (kv *KVPaxos) RecordReqID(reqID string) {
	kv.recordedReqIDs[reqID] = true
	DPrintf("%d kvpaxos record reqID: %s\n", kv.me, reqID)
}

// Check if reqID is recorded
func (kv *KVPaxos) HasSeenReqID(reqID string) bool {
	_, reqExists := kv.recordedReqIDs[reqID]
	return reqExists
}

// Execute the logOp instance to local database, then update lastExecSeq to seq
// Once the Op is executed to local, we can make done of this seq
// precondition: Paxos.log.index(op) == seq
func (kv *KVPaxos) ExecuteLog(logOp Op, seq int) {
	if seq <= kv.lastExecSeq {
		return
	}
	DPrintf("%d paxo execute seq: %d for reqID: %s, key: %s, op: %s\n", kv.me, seq, logOp.ReqID, logOp.Key, logOp.Operation)
	op, key, value, reqID := logOp.Operation, logOp.Key, logOp.Value, logOp.ReqID
	switch op {
	case Get, Nop:
		//value = kv.database[key]
	case Put:
		kv.database[key] = value
	case Append:
		kv.database[key] += value
	}

	kv.RecordReqID(reqID)
	kv.px.Done(seq)
	kv.lastExecSeq = seq
}

// Try to add a log entry (instance) to Paxos at seq slot.
// Return the instance in seq slot.
// The return op might not be what we want to add. The most likely case
// is that our server died and restarted. Then this will then act as a way
// for kvpaxos server to catch up all the missed instances
func (kv *KVPaxos) TryAddLog(logOp Op, seq int) Op {
	kv.px.Start(seq, logOp)
	var result Op
	done, toSleep := false, BaseSleep
	DPrintf("%d kvpaxos add log in seq: %d, reqID: %s\n", kv.me, seq, logOp.ReqID)
	for !done && !kv.dead {
		decided, _ := kv.px.Status(seq)
		switch decided {
		case paxos.Decided:
			done = true
		case paxos.Pending:
			toSleep = WaitForPending(toSleep, MaxSleep)
		}
	}
	_, value := kv.px.Status(seq)
	result = value.(Op)
	return result
}

// Add a new Op to paxos log as a new instance. This method will not
// terminate until it's certain the passed in Op is inserted successfully.
func (kv *KVPaxos) AddOpToPaxos(opArgs Op) {
	done := false
	for !done && !kv.dead {
		seq := kv.lastExecSeq + 1
		result := kv.TryAddLog(opArgs, seq)
		// if this seq(instance) agrees on other value, then ReqID will not be the same
		done = (result.ReqID == opArgs.ReqID)
		// execute what we get from Paxos
		kv.ExecuteLog(result, seq)
	}
}

// Get RPC Handler in kvpaxos
func (kv *KVPaxos) Get(args *GetArgs, reply *GetReply) error {
	// Your code here.
	kv.mu.Lock()
	defer kv.mu.Unlock()
	DPrintf("%d kvpaxos receives Get, key: %s, reqID: %s\n", kv.me, args.Key, args.ReqID)
	// at-most-once
	if kv.HasSeenReqID(args.ReqID) {
		reply.Value = kv.database[args.Key]
		reply.Err = OK
		return nil
	}
	// never seen before
	opArgs := Op{Operation: Get, Key: args.Key, Value: "", ReqID: args.ReqID}
	kv.AddOpToPaxos(opArgs)

	reply.Value = kv.database[args.Key]
	reply.Err = OK // just ignore the ErrNoKey case

	return nil
}

// Put/Append RPC Handler in kvpaxos
func (kv *KVPaxos) PutAppend(args *PutAppendArgs, reply *PutAppendReply) error {
	// Your code here.
	kv.mu.Lock()
	defer kv.mu.Unlock()
	DPrintf("%d kvpaxos receives %s, key: %s, reqID: %s\n", kv.me, args.Op, args.Key, args.ReqID)
	// at-most-once
	if kv.HasSeenReqID(args.ReqID) {
		reply.Err = OK
		return nil
	}

	opArgs := Op{Operation: args.Op, Key: args.Key, Value: args.Value, ReqID: args.ReqID}
	kv.AddOpToPaxos(opArgs)

	reply.Err = OK
	return nil
}

// tell the server to shut itself down.
// please do not change this function.
func (kv *KVPaxos) kill() {
	DPrintf("Kill(%d): die\n", kv.me)
	kv.dead = true
	kv.l.Close()
	kv.px.Kill()
}

//
// servers[] contains the ports of the set of
// servers that will cooperate via Paxos to
// form the fault-tolerant key/value service.
// me is the index of the current server in servers[].
//
func StartServer(servers []string, me int) *KVPaxos {
	// call gob.Register on structures you want
	// Go's RPC library to marshall/unmarshall.
	gob.Register(Op{})

	kv := new(KVPaxos)
	kv.me = me

	// Your initialization code here.
	kv.database = make(map[string]string)
	kv.lastExecSeq = -1
	kv.recordedReqIDs = make(map[string]bool)

	rpcs := rpc.NewServer()
	rpcs.Register(kv)

	kv.px = paxos.Make(servers, me, rpcs)

	os.Remove(servers[me])
	l, e := net.Listen("unix", servers[me])
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	kv.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for kv.dead == false {
			conn, err := kv.l.Accept()
			if err == nil && kv.dead == false {
				if kv.unreliable && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if kv.unreliable && (rand.Int63()%1000) < 200 {
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
			if err != nil && kv.dead == false {
				fmt.Printf("KVPaxos(%v) accept: %v\n", me, err.Error())
				kv.kill()
			}
		}
	}()

	return kv
}
