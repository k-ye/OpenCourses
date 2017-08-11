package pbservice

import "viewservice"
import "net/rpc"
import "fmt"
import "time"

import "crypto/rand"
import "math/big"
import "strconv"
import "mydebug"


type Clerk struct {
	vs *viewservice.Clerk
	// Your declarations here
	UID string // Unique ID for clerk
	VSdead bool
	Primary string // cached Primary server
	ReqIDCounter uint64 // counter for request ID, inc by 1
}

// this may come in handy.
func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}

func MakeClerk(vshost string, me string) *Clerk {
	ck := new(Clerk)
	ck.vs = viewservice.MakeClerk(me, vshost)
	// Your ck.* initializations here
	ck.UID = strconv.FormatInt(nrand(), 10)
	ck.VSdead = false
	ck.Primary = ""
	ck.ReqIDCounter = 1
	mydebug.DPrintf("Client %s inited\n", ck.UID)

	return ck
}


//
// call() sends an RPC to the rpcname handler on server srv
// with arguments args, waits for the reply, and leaves the
// reply in reply. the reply argument should be a pointer
// to a reply structure.
//
// the return value is true if the server responded, and false
// if call() was not able to contact the server. in particular,
// the reply's contents are only valid if call() returned true.
//
// you should assume that call() will time out and return an
// error after a while if it doesn't get a reply from the server.
//
// please use call() to send all RPCs, in client.go and server.go.
// please don't change this function.
//
func call(srv string, rpcname string,
	args interface{}, reply interface{}) bool {
	c, errx := rpc.Dial("unix", srv)
	if errx != nil {
		return false
	}
	defer c.Close()

	err := c.Call(rpcname, args, reply)
	if err == nil {
		return true
	}

	fmt.Println(err)
	return false
}

// Try to init Primary if it is ""
func (ck *Clerk) TryInitPrimary() {
	if ck.Primary == ""  {
		mydebug.DPrintf("Client try to find the Primary\n")
		ck.Ping()
	}
}

// Ping the VS to get latest view info
func (ck *Clerk) Ping() {
	view, ok := ck.vs.Get()
	ck.Primary = view.Primary
	ck.VSdead = !ok
}

// Must call this to generate new request ID for ck!
func GenReqID(ck *Clerk, op string) string {
	newReqID := fmt.Sprintf("%s.%s.%d", ck.UID, op, ck.ReqIDCounter)
	ck.ReqIDCounter += 1
	return newReqID
}

// Check if the operation this time is failed
func IsOperationSucessful(ok bool, err Err) bool {
	if !ok || err == ErrWrongServer || err == ErrFailBackup || err == ErrServerBusy {
		return false
	}
	return true
}

//
// fetch a key's value from the current primary;
// if they key has never been set, return "".
// Get() must keep trying until it either the
// primary replies with the value or the primary
// says the key doesn't exist (has never been Put().
//
func (ck *Clerk) Get(key string) string {
	args := GetArgs{Key:key, IsFromClient:true, From:ck.UID, ReqID:GenReqID(ck, "Get")}
	var reply GetReply
	ck.TryInitPrimary()
	// Your code here.
	ok := false
	for !ok {
		mydebug.DPrintf("Client %s attempts to Get, Key: %s, ReqID: %s\n", ck.UID, key, args.ReqID)
		ok = call(ck.Primary, "PBServer.Get", &args, &reply)
		if opOk := IsOperationSucessful(ok, reply.Err); !opOk {
			mydebug.DPrintf("Client %s Get failed. ok: %v, Err: %s, Key: %s\n", ck.UID, ok, reply.Err, key)
			ck.Ping()
			reply = GetReply{}
			ok = false
		}
		time.Sleep(viewservice.PingInterval)
		if ck.VSdead {
			mydebug.DPrintf("Client %s Get, found View Server dead\n", ck.UID)
			return ""
		}
	}
	
	mydebug.DPrintf("Client %s Get OK, Key: %s, Value: %s\n", ck.UID, key, reply.Value)
	return reply.Value
}

//
// send a Put or Append RPC
//
func (ck *Clerk) PutAppend(key string, value string, op string) {

	// Your code here.
	args := PutAppendArgs{Key:key, Value:value, Operation:op, IsFromClient:true, From:ck.UID, ReqID:GenReqID(ck, op)}
	var reply PutAppendReply
	ck.TryInitPrimary()

	ok := false
	for !ok {
		mydebug.DPrintf("Client %s attempts to %s, Key: %s, Value: %s, ReqID: %s\n", ck.UID, op, key, value, args.ReqID)
		ok = call(ck.Primary, "PBServer.PutAppend", &args, &reply)
		if opOk:= IsOperationSucessful(ok, reply.Err); !opOk {
			mydebug.DPrintf("Client %s %s failed. ok: %v, Err: %s, Key: %s, Value: %s\n", ck.UID, op, ok, reply.Err, key, value)
			ck.Ping()
			reply = PutAppendReply{}
			ok = false
		}
		time.Sleep(viewservice.PingInterval)
		if ck.VSdead {
			mydebug.DPrintf("Client PutAppend, %s found View Server dead\n", ck.UID)
			return
		}
	}
	mydebug.DPrintf("Client %s %s OK, Key: %s, Value: %s\n", ck.UID, op, key, value)
}

//
// tell the primary to update key's value.
// must keep trying until it succeeds.
//
func (ck *Clerk) Put(key string, value string) {
	ck.PutAppend(key, value, Put)
}

//
// tell the primary to append to key's value.
// must keep trying until it succeeds.
//
func (ck *Clerk) Append(key string, value string) {
	ck.PutAppend(key, value, Append)
}
