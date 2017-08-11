package kvpaxos

import "net/rpc"
import "crypto/rand"
import "math/big"

import "fmt"
import "strconv"
import "strings"

type Clerk struct {
	servers []string
	// You will have to modify this struct.
	uid        string
	reqCounter int64
	serverPt   int // pointer to servers
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}

func MakeClerk(servers []string) *Clerk {
	ck := new(Clerk)
	ck.servers = servers
	// You'll have to add code here.
	ck.uid = strconv.FormatInt(nrand(), 10)
	ck.reqCounter = 0
	ck.serverPt = int(nrand() % int64(len(servers))) // random init to balance init workload

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

// Must call this to generate new request ID for ck!
func (ck *Clerk) GenerateReqID(op string) string {
	newReqID := fmt.Sprintf("%s.%s.%d", ck.uid, op, ck.reqCounter)
	ck.reqCounter += 1
	return newReqID
}

// Parse a valid request ID to all the information it contains
func ParseReqID(reqID string) (ck, op string, reqNum int) {
	arr := strings.Split(reqID, ".")
	ck, op = arr[0], arr[1]
	reqNum64, _ := strconv.ParseInt(arr[2], 10, 0)
	reqNum = int(reqNum64)
	return
}

// return the current server name this ck is communicating with
func (ck *Clerk) CurrentServer() string {
	return ck.servers[ck.serverPt]
}

// Advance to next server to communicate
func (ck *Clerk) AdvanceToNextServer() {
	ck.serverPt++
	ck.serverPt = ck.serverPt % len(ck.servers)
}

//
// fetch the current value for a key.
// returns "" if the key does not exist.
// keeps trying forever in the face of all other errors.
//
func (ck *Clerk) Get(key string) string {
	// You will have to modify this function.
	args := GetArgs{Key: key, CkUID: ck.uid, ReqID: ck.GenerateReqID(Get)}
	var reply GetReply
	ok := false
	value := ""
	for !ok {
		ok = call(ck.CurrentServer(), "KVPaxos.Get", &args, &reply)
		if !ok {
			// this server is down, move to next
			ck.AdvanceToNextServer()
		} else {
			value = reply.Value
		}
	}
	return value
}

//
// shared by Put and Append.
//
func (ck *Clerk) PutAppend(key string, value string, op string) {
	// You will have to modify this function.
	args := PutAppendArgs{Key: key, Value: value, Op: op, CkUID: ck.uid, ReqID: ck.GenerateReqID(op)}
	var reply PutAppendReply
	ok := false
	for !ok {
		ok = call(ck.CurrentServer(), "KVPaxos.PutAppend", &args, &reply)
		if !ok {
			// this server is down, move to next
			ck.AdvanceToNextServer()
		}
	}
}

func (ck *Clerk) Put(key string, value string) {
	ck.PutAppend(key, value, "Put")
}
func (ck *Clerk) Append(key string, value string) {
	ck.PutAppend(key, value, "Append")
}
