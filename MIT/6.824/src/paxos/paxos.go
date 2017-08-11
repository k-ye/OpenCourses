package paxos

//
// Paxos library, to be included in an application.
// Multiple applications will run, each including
// a Paxos peer.
//
// Manages a sequence of agreed-on values.
// The set of peers is fixed.
// Copes with network failures (partition, msg loss, &c).
// Does not store anything persistently, so cannot handle crash+restart.
//
// The application interface:
//
// px = paxos.Make(peers []string, me string)
// px.Start(seq int, v interface{}) -- start agreement on new instance
// px.Status(seq int) (Fate, v interface{}) -- get info about an instance
// px.Done(seq int) -- ok to forget all instances <= seq
// px.Max() int -- highest instance seq known, or -1
// px.Min() int -- instances before this seq have been forgotten
//

/*
Summary:
1. Be extremely careful about mutex! If a function uses mutex inside it,
	 then you can't lock/unlock again outside!
2. Strategy for proposal number:
		each paxos has different ID, therefore if one keeps using: prop(x) = ID + x * len(peers),
		then we can avoid same prop number
3. Little structure optimization, needs to improve into the pseudo-code style! Too much
	 redundant in RPC Args
*/

import "net"
import "net/rpc"
import "log"

import "os"
import "syscall"
import "sync"
import "fmt"
import "math/rand"
import "time"
import "mydebug"

import "encoding/gob"
import "io/ioutil"
import "bytes"
import "strconv"

// px.Status() return values, indicating
// whether an agreement has been decided,
// or Paxos has not yet reached agreement,
// or it was agreed but forgotten (i.e. < Min()).
type Fate int

const (
	Decided   Fate = 1
	Pending        = 2 // not yet decided.
	Forgotten      = 3 // decided but forgotten.
)

type Paxos struct {
	mu         sync.Mutex
	l          net.Listener
	dead       bool
	unreliable bool
	rpcCount   int
	peers      []string
	me         int // index into peers[]

	// Your data here.
	nextPropNum    map[int]int           // maps seq to the highest proposal N
	acceptorStates map[int]AcceptorState // acceptor's status for each Paxos server
	chosenSeqs     map[int]bool          // the chosen value map
	peersDone      []int                 // store all the done val infomation
	peersMaxSeqs   []int // max seq seen in each peers

	dir            string                // each replica has its own data directory
	needLocal      bool                  // determine is is needed to write to local
	noonvoting bool
}

type AccpValWrapper struct {
	Val interface{}
}

type PaxosLocalState struct {
	PeersMaxSeqs []int // max seq seen for each value
}

func MaxInt(a, b int) int {
	if (a < b) {
		return b
	}
	return a
}

func MinInt(a, b int) int {
	if (a > b) {
		return b
	}
	return a
}

func InitPeersDone(l int) []int {
	var result []int
	for i := 0; i < l; i++ {
		result = append(result, -1)
	}
	return result
}

//
// call() sends an RPC to the rpcname handler on server srv
// with arguments args, waits for the reply, and leaves the
// reply in reply. the reply argument should be a pointer
// to a reply structure.
//
// the return value is true if the server responded, and false
// if call() was not able to contact the server. in particular,
// the replys contents are only valid if call() returned true.
//
// you should assume that call() will time out and return an
// error after a while if it does not get a reply from the server.
//
// please use call() to send all RPCs, in client.go and server.go.
// please do not change this function.
//
func call(srv string, name string, args interface{}, reply interface{}) bool {
	c, err := rpc.Dial("unix", srv)
	if err != nil {
		err1 := err.(*net.OpError)
		if err1.Err != syscall.ENOENT && err1.Err != syscall.ECONNREFUSED {
			fmt.Printf("paxos Dial() failed: %v\n", err1)
		}
		return false
	}
	defer c.Close()

	err = c.Call(name, args, reply)
	if err == nil {
		return true
	}

	fmt.Println(err)
	return false
}

// directory to store paxos local files
func (px *Paxos) paxosDir() string {
	if !px.needLocal {
		return ""
	}

	d := px.dir + "/paxos/"
	// create directory if needed.
	_, err := os.Stat(d)
	if err != nil {
		if err := os.Mkdir(d, 0777); err != nil {
			log.Fatalf("Mkdir(%v): %v", d, err)
		}
	}
	return d
}

func (px *Paxos) writePXState() error {
	if !px.needLocal {
		return nil
	}

	localstate := PaxosLocalState{}
	localstate.PeersMaxSeqs = px.peersMaxSeqs

	w := new(bytes.Buffer)
	e := gob.NewEncoder(w)
	e.Encode(localstate)

	tempname := px.paxosDir() + "temp-state"
	fullname := px.paxosDir() + "pxstate"

	if err := ioutil.WriteFile(tempname, w.Bytes(), 0666); err != nil {
		return err
	}
	if err := os.Rename(tempname, fullname); err != nil {
		return err
	}
	return nil
}

// SO MANY DUPLICATE CODE!
func (px *Paxos) readPXState() (PaxosLocalState, error) {
	var result PaxosLocalState
	if !px.needLocal {
		return result, nil
	}

	fullname := px.paxosDir() + "pxstate"
	buf, err := ioutil.ReadFile(fullname)

	if err != nil {
		return result, err
	}
	r := bytes.NewBuffer(buf)
	d := gob.NewDecoder(r)
	d.Decode(&result)

	fmt.Printf("Decoded Paxos Local State: %v\n", result)
	return result, err
}

func (px *Paxos) diskIntact() bool {
	if !px.needLocal {
		return true
	}

	d := px.dir + "/paxos/"
	_, err := os.Stat(d) //ioutil.ReadDir(d)
	fmt.Printf("Is Paxos: %v Disk Intact? : %v\n", px.me, err)
	return err == nil
}

// replace the content of a given seq's acceptor state
// uses rename() to make the replacement atomic with
// respect to crashes.
func (px *Paxos) instancePut(acstate *AcceptorState) error {
	if !px.needLocal {
		return nil
	}

	fullname := px.paxosDir() + "instance-" + strconv.Itoa(acstate.Seq)
	tempname := px.paxosDir() + "temp-" + strconv.Itoa(acstate.Seq)

	w := new(bytes.Buffer)
	e := gob.NewEncoder(w)

	wrappedVal := AccpValWrapper{Val:acstate.AccpVal}
	e.Encode(acstate.MaxPropNum)
	e.Encode(acstate.MaxAccpNum)
	e.Encode(wrappedVal)
	//fmt.Printf("%v paxos put instance: %v onto disk.\n", px.me, acstate)

	if err := ioutil.WriteFile(tempname, w.Bytes(), 0666); err != nil {
		return err
	}
	if err := os.Rename(tempname, fullname); err != nil {
		return err
	}

	return nil
}

// read the content of a given seq's acceptor state
func (px *Paxos) instanceGet(seq int) (AcceptorState, error) {
	result := AcceptorState{}
	if !px.needLocal {
		return result, nil
	}

	fullname := px.paxosDir() + "instance-" + strconv.Itoa(seq)
	buf, err := ioutil.ReadFile(fullname)

	if err != nil {
		return result, err
	}

	r := bytes.NewBuffer(buf)
	d := gob.NewDecoder(r)

	var maxPropNum, maxAccpNum int
	var wrappedVal AccpValWrapper
	d.Decode(&maxPropNum)
	d.Decode(&maxAccpNum)
	d.Decode(&wrappedVal)
	fmt.Printf("%v paxos decoded accp val: %v for seq: %v\n", px.me, wrappedVal, seq)

	result.Seq = seq
	result.MaxPropNum = maxPropNum
	result.MaxAccpNum = maxAccpNum
	result.AccpVal = wrappedVal.Val

	return result, err
}

func (px *Paxos) instanceRemove(seq int) error {
	if !px.needLocal {
		return nil
	}
	//fmt.Printf("Paxos log removing seq: %d from disk\n", seq)
	fullname := px.paxosDir() + "instance-" + strconv.Itoa(seq)
	err := os.Remove(fullname)
	return err
}

func (px *Paxos) allinstanceFreeMem(maxSeq int) {
	if !px.needLocal {
		return
	}

	d := px.paxosDir()
	files, err := ioutil.ReadDir(d)
	if err != nil {
		log.Fatalf("fileReadShard could not read %v: %v", d, err)
	}
	for _, fi := range files {
		n1 := fi.Name()
		if (len(n1) > 10) && (n1[0:9] == "instance-") {
			seq, err := strconv.Atoi(n1[9:])
			if err != nil {
				log.Fatalf("fileReadShard bad file name %v: %v", n1, err)
			}
			if seq <= maxSeq {
				fmt.Printf("lfjlakdjfa\n")
				err = px.instanceRemove(seq)
				if err != nil {
					log.Fatalf("Allinstance free mem failed for %v: %v", seq, err)
				}
			}
		}
	}
}

// return all the acceptor instances on local disk
func (px *Paxos) allinstanceGet() map[int]AcceptorState {
	m := make(map[int]AcceptorState)
	if !px.needLocal {
		return m
	}

	d := px.paxosDir()
	files, err := ioutil.ReadDir(d)
	if err != nil {
		log.Fatalf("fileReadShard could not read %v: %v", d, err)
	}
	for _, fi := range files {
		n1 := fi.Name()
		if (len(n1) > 10) && (n1[0:9] == "instance-") {
			seq, err := strconv.Atoi(n1[9:])
			if err != nil {
				log.Fatalf("fileReadShard bad file name %v: %v", n1, err)
			}
			content, err := px.instanceGet(seq)
			if err != nil {
				log.Fatalf("fileReadShard fileGet failed for %v: %v", seq, err)
			}
			m[seq] = content
			//px.nextPropNum[seq] = px.me
			//px.chosenSeqs[seq] = true
		}
	}
	return m
}

// Check if Paxos server has seen a given instance seq no.
func (px *Paxos) HasSeenSeq(seq int) bool {
	_, ok := px.nextPropNum[seq]
	return ok
}

// Create an entry for new instance labeled as seq
func (px *Paxos) CreateSeqEntry(seq int) {
	if px.HasSeenSeq(seq) {
		mydebug.DPrintf("Paxos %d already has %d seq instance\n", px.me, seq)
		return
	}
	px.nextPropNum[seq] = px.me

	_, fromrestart := px.acceptorStates[seq]
	if !fromrestart {
		// default -1, indicates lack of value. This is because proposal number can be 0
		newstate := AcceptorState{Seq: seq, MaxPropNum: -1, MaxAccpNum: -1}
		px.acceptorStates[seq] = newstate
	}
}

func IsMajority(check int, all int) bool {
	return check*2 > all
}

// Increase proposal number beyond lower bound for seq instance
func (px *Paxos) IncreasePropNum(seq int, lower int) {
	if px.HasSeenSeq(seq) {
		px.mu.Lock()
		defer px.mu.Unlock()
		for px.nextPropNum[seq] <= lower {
			px.nextPropNum[seq] += len(px.peers)
		}
	}
}

// Get z_i, maximum done value been received
func (px *Paxos) MaxDoneVal() int {
	return px.peersDone[px.me]
}

func (px *Paxos) MinDoneVal() int {
	minDone := px.MaxDoneVal()
	for i := 0; i < len(px.peersDone); i++ {
		if px.peersDone[i] < minDone {
			minDone = px.peersDone[i]
		}
	}
	return minDone
}

func (px *Paxos) reconstructFromLocal() bool {
	localstate, err := px.readPXState()
	if err != nil {
		return false
	}
	px.peersMaxSeqs = localstate.PeersMaxSeqs
	px.acceptorStates = px.allinstanceGet()
	return true
}

func (px *Paxos) RequestPxState(args *RequestPxStateArgs, reply *RequestPxStateReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	reply.AcceptorStates = px.acceptorStates
	reply.ChosenSeqs = px.chosenSeqs
	reply.PeersDone = px.peersDone
	reply.PeersMaxSeqs = px.peersMaxSeqs
	return nil
}

func (px *Paxos) CollectPxStates() RequestPxStateReply {
	finalreply := RequestPxStateReply{AcceptorStates:make(map[int]AcceptorState), 
		ChosenSeqs:make(map[int]bool), PeersDone:InitPeersDone(len(px.peers)), PeersMaxSeqs:InitPeersDone(len(px.peers))}
	
	for i := 0; i < len(px.peers); i++ {
		args := RequestPxStateArgs{}
		reply := RequestPxStateReply{AcceptorStates:make(map[int]AcceptorState), 
			ChosenSeqs:make(map[int]bool), PeersDone:InitPeersDone(len(px.peers))}

		if i != px.me {
			ok := call(px.peers[i], "Paxos.RequestPxState", &args, &reply)
			if ok {
				finalreply.MergeReply(&reply)
			}
		}
	}

	return finalreply
}

func (px *Paxos) reconstructFromRemote() {
	finalreply := px.CollectPxStates()

	px.acceptorStates = finalreply.AcceptorStates
	px.chosenSeqs = finalreply.ChosenSeqs
	px.peersDone = finalreply.PeersDone
	px.peersMaxSeqs = finalreply.PeersMaxSeqs

	for seq, _ := range px.acceptorStates {
		px.nextPropNum[seq] = px.me
	}
}

// function to send prepare request to all Paxos server
func (px *Paxos) SendPrepareToAll(seq int, v interface{}) ([]int, int, interface{}) {
	var prepOKAcceptors []int

	propNum := px.nextPropNum[seq]
	maxAccpNum := -1
	valToSend := v

	fmt.Printf(">>> %d in Phase 1 Prepare, seq: %d, n: %d, val: %v\n", px.me, seq, propNum, v)
	// loop over all the acceptors
	for i := 0; i < len(px.peers); i++ {
		pArgs := PrepareArgs{PxID: px.me, Seq: seq, PropNum: propNum}
		var pReply PrepareReply

		pok := true
		mydebug.DPrintf("%d sending prepare request to %d for seq: %d, n: %d\n", px.me, i, seq, propNum)
		if i == px.me {
			px.Prepare(&pArgs, &pReply)
		} else {
			pok = call(px.peers[i], "Paxos.Prepare", &pArgs, &pReply)
		}

		if pok {
			fmt.Printf("%d px prepare response: %v from %d for seq: %d, n: %d\n", px.me, pReply, i, seq, propNum)
			// Good connection
			if pReply.RcverMaxSeq > px.peersMaxSeqs[i] {
				px.peersMaxSeqs[i] = pReply.RcverMaxSeq
				px.writePXState()
			}
			if pReply.SnderMaxSeq > px.peersMaxSeqs[px.me] {
				px.peersMaxSeqs[px.me] = pReply.SnderMaxSeq
				px.writePXState()
			}

			if pReply.PrepareStatus == RequestOK {
				// Prepare_Ok
				mydebug.DPrintf("%d got prepare OK from %d for seq: %d, n: %d\n", px.me, i, seq, propNum)
				prepOKAcceptors = append(prepOKAcceptors, i)
				if pReply.HasAcceptValue() && pReply.MaxAccpNum > maxAccpNum && pReply.MaxAccpNum <= propNum {
					maxAccpNum = pReply.MaxAccpNum
					valToSend = pReply.AccpVal
				}
			} else {
				mydebug.DPrintf("%d got prepare reject from %d for seq: %d, n: %d\n", px.me, i, seq, propNum)
				// Propose Rejected. Proposal number is not large enough
				px.IncreasePropNum(seq, pReply.MaxPropNum)
				mydebug.DPrintf("%d will use n: %d for seq: %d in next round\n", px.me, px.nextPropNum[seq], seq)
			}
		} else {
			mydebug.DPrintf("%d failed to receive prepare response from %d for seq: %d\n", px.me, i, seq)
		}
	}
	return prepOKAcceptors, propNum, valToSend
}

// Acceptor's Phase 1 Prepare RPC Handler
func (px *Paxos) Prepare(args *PrepareArgs, reply *PrepareReply) error {
	seq := args.Seq
	propNum := args.PropNum

	px.mu.Lock()
	defer px.mu.Unlock()
	// create an entry for new Paxos algorithm instance
	if !px.HasSeenSeq(seq) {
		px.CreateSeqEntry(seq)
	}

	reply.SnderMaxSeq = px.peersMaxSeqs[args.PxID]
	reply.RcverMaxSeq = px.peersMaxSeqs[px.me]

	if px.noonvoting {
		reply.PrepareStatus = Reject
		return nil
	}

	if seq > px.peersMaxSeqs[px.me] {
		px.peersMaxSeqs[px.me] = seq
		px.writePXState()
	}

	mydebug.DPrintf("%d begin Prepare handler from %d for seq: %d, n: %d\n", px.me, args.PxID, seq, propNum)
	//fmt.Printf("%d current acceptor status for seq: %d, state: %v\n", px.me, seq, px.acceptorStates[seq])
	if propNum > px.acceptorStates[seq].MaxPropNum {
		// it might be OK for this proposal to go to Phase 2
		tempstate := px.acceptorStates[seq]
		// Current acceptor has accepted some proposal before
		tempstate.MaxPropNum = propNum
		reply.AccpVal = tempstate.AccpVal
		reply.MaxAccpNum = tempstate.MaxAccpNum
		px.acceptorStates[seq] = tempstate
		px.instancePut(&tempstate)

		mydebug.DPrintf("%d return Prepare OK to %d for seq: %d, reply: %v\n", px.me, args.PxID, seq, reply)
		reply.PrepareStatus = RequestOK
	} else {
		// Don't even think about Phase 2, update your proposal number
		mydebug.DPrintf("%d return Prepare reject to %d for seq: %d\n", px.me, args.PxID, seq)
		reply.PrepareStatus = Reject
	}
	// If RequestOK, this can act as a promise that Acceptor knows never to accept
	// any proposal with number < MaxPropNum
	// Otherwise, it's a hint for the Proposer to know how much it should update for its propNum
	reply.MaxPropNum = px.acceptorStates[seq].MaxPropNum
	reply.MaxAccpNum = px.acceptorStates[seq].MaxAccpNum
	return nil
}

// function to send accept request to all Paxos servers
func (px *Paxos) SendAcceptToAll(majAcceptors []int, seq int, propNum int, val interface{}) int {
	mydebug.DPrintf(">>> %d in Phase 2 Accept, majority: %v\n", px.me, majAcceptors)
	mydebug.DPrintf("%d in Phase 2 Accept, seq: %d, n: %d, v: %v\n", px.me, seq, propNum, val)
	accpResponseCount := 0

	for i := 0; i < len(majAcceptors); i++ {
		aArgs := AcceptArgs{PxID: px.me, Seq: seq, PropNum: propNum, PropVal: val}
		var aReply AcceptReply
		acceptorIdx := majAcceptors[i]
		aok := true
		mydebug.DPrintf("%d sending accept request to %d for seq: %d, n: %d, v: %v\n", px.me, acceptorIdx, seq, propNum, val)

		if acceptorIdx == px.me {
			px.Accept(&aArgs, &aReply)
		} else {
			aok = call(px.peers[acceptorIdx], "Paxos.Accept", &aArgs, &aReply)
		}

		if aok {
			if aReply.RcverMaxSeq > px.peersMaxSeqs[i] {
				px.peersMaxSeqs[i] = aReply.RcverMaxSeq
				px.writePXState()
			}
			if aReply.SnderMaxSeq > px.peersMaxSeqs[px.me] {
				px.peersMaxSeqs[px.me] = aReply.SnderMaxSeq
				px.writePXState()
			}
			// Good Connection
			if aReply.AcceptStatus == RequestOK {
				mydebug.DPrintf("%d got accepted by %d for proposal n: %d, v: %v, seq:%d\n", px.me, acceptorIdx, propNum, val, seq)
				accpResponseCount++
			}
		} else {
			mydebug.DPrintf("%d got accept reject from %d for seq: %d, n: %d\n", px.me, acceptorIdx, seq, propNum)
		}
	}
	return accpResponseCount
}

// Acceptor's Phase 2 Accept RPC Handler
func (px *Paxos) Accept(args *AcceptArgs, reply *AcceptReply) error {
	seq := args.Seq
	propNum := args.PropNum
	propVal := args.PropVal
	// don't have to create new entry for seq, as it should already be stored
	mydebug.DPrintf("%d begin Accept handler from %d for seq: %d, n: %d, v:%v\n", px.me, args.PxID, seq, propNum, propVal)
	mydebug.DPrintf("%d current acceptor status for seq: %d, state: %v\n", px.me, seq, px.acceptorStates[seq])
	px.mu.Lock()
	defer px.mu.Unlock()

	reply.SnderMaxSeq = px.peersMaxSeqs[args.PxID]
	reply.RcverMaxSeq = px.peersMaxSeqs[px.me]

	if px.noonvoting {
		reply.AcceptStatus = Reject
		return nil
	}

	if seq > px.peersMaxSeqs[px.me] {
		px.peersMaxSeqs[px.me] = seq
		px.writePXState()
	}

	if propNum >= px.acceptorStates[seq].MaxPropNum {
		// acceptor accepts the accept request
		newstate := AcceptorState{Seq: seq, MaxPropNum: propNum, MaxAccpNum: propNum, AccpVal: propVal}
		px.acceptorStates[seq] = newstate
		px.instancePut(&newstate)

		reply.AcceptStatus = RequestOK
		mydebug.DPrintf("%d return Accept OK to %d for seq: %d, acceptor: %v\n", px.me, args.PxID, seq, newstate)
	} else {
		// reject it
		reply.AcceptStatus = Reject
		mydebug.DPrintf("%d return Accept reject to %d for seq: %d, acceptor: %v\n", px.me, args.PxID, seq, px.acceptorStates[seq])
	}
	reply.PropNum = propNum
	return nil
}

// function to send decision to all Paxos servers, also piggyback Done Value if RPC succeeds
func (px *Paxos) SendDecisionToAll(seq int, propNum int, val interface{}) {
	mydebug.DPrintf("%d broadcasting the decision for seq: %d val: %v\n", px.me, seq, val)
	for i := 0; i < len(px.peers); i++ {
		dArgs := DecideArgs{PxID: px.me, Seq: seq, PropNum: propNum, PropVal: val, DoneVal: px.MaxDoneVal()}
		dReply := DecideReply{}
		dok := true
		if i == px.me {
			px.Decide(&dArgs, &dReply)
		} else {
			dok = call(px.peers[i], "Paxos.Decide", &dArgs, &dReply)
		}
		if dok {
			if dReply.RcverMaxSeq > px.peersMaxSeqs[i] {
				px.peersMaxSeqs[i] = dReply.RcverMaxSeq
				px.writePXState()
			}
			if dReply.SnderMaxSeq > px.peersMaxSeqs[px.me] {
				px.peersMaxSeqs[px.me] = dReply.SnderMaxSeq
				px.writePXState()
			}

			px.peersDone[dReply.PxID] = dReply.DoneVal
		}
	}
}

// Decide handler
func (px *Paxos) Decide(args *DecideArgs, reply *DecideReply) error {
	pxID := args.PxID
	seq := args.Seq
	propNum := args.PropNum
	propVal := args.PropVal
	doneVal := args.DoneVal

	px.mu.Lock()
	defer px.mu.Unlock()

	reply.SnderMaxSeq = px.peersMaxSeqs[args.PxID]
	reply.RcverMaxSeq = px.peersMaxSeqs[px.me]

	if px.noonvoting && (seq > px.peersMaxSeqs[px.me]) {
		px.noonvoting = false
	}

	if !px.HasSeenSeq(seq) {
		px.CreateSeqEntry(seq)
	}

	if seq > px.peersMaxSeqs[px.me] {
		px.peersMaxSeqs[px.me] = seq
		px.writePXState()
	}

	// Piggyback the DoneVal
	px.peersDone[pxID] = doneVal
	reply.PxID = px.me
	reply.DoneVal = px.MaxDoneVal()
	mydebug.DPrintf("%d receive Decide from %d, seq: %d, val: %d\n", px.me, pxID, seq, propVal)

	//px.chosenVals[seq] = propVal
	px.chosenSeqs[seq] = true
	// update the Acceptor's status
	newstate := AcceptorState{Seq: seq, MaxPropNum: propNum, MaxAccpNum: propNum, AccpVal: propVal}
	px.acceptorStates[seq] = newstate
	px.instancePut(&newstate)

	reply.DecideStatus = RequestOK
	return nil
}

// Free memory with seq leq minDone
func (px *Paxos) FreeMemory(minDone int) {
	//fmt.Printf("%d, Before free memory, length: %d\n", px.me, len(px.acceptorStates))
	//fmt.Printf("%d paxos, given min done is: %d\n", px.me, minDone)
	for key, _ := range px.nextPropNum {
		if key <= minDone {
			delete(px.nextPropNum, key)
		}
	}
	for key, _ := range px.acceptorStates {
		if key <= minDone {
			delete(px.acceptorStates, key)
			px.instanceRemove(key)
		}
	}
	//for key, _ := range px.chosenVals {
	for key, _ := range px.chosenSeqs {
		if key <= minDone {
			delete(px.chosenSeqs, key)
		}
	}
	//fmt.Printf("%d, After free memory, length: %d\n", px.me, len(px.acceptorStates))
}

//
// the application wants paxos to start agreement on
// instance seq, with proposed value v.
// Start() returns right away; the application will
// call Status() to find out if/when agreement
// is reached.
//
func (px *Paxos) Start(seq int, v interface{}) {
	// Your code here.
	go func() {
		if seq < px.Min() {
			return
		}

		//_, decided := px.chosenVals[seq]
		_, decided := px.chosenSeqs[seq]
		
		for !decided && !px.dead {
			// Phase 1 Prepare
			prepOKAcceptors, propNum, valToSend := px.SendPrepareToAll(seq, v)
			//fmt.Printf("Paxos done preparing\n")
			// Phase 2 Accept
			if IsMajority(len(prepOKAcceptors), len(px.peers)) {
				// a majority is formed, send accept request
				accpResponseCount := px.SendAcceptToAll(prepOKAcceptors, seq, propNum, valToSend)
				//fmt.Printf("Paxos done accepting\n")
				if IsMajority(accpResponseCount, len(px.peers)) {
					// decide to be this value
					px.SendDecisionToAll(seq, propNum, valToSend)
				}
			} else {
				mydebug.DPrintf(">>> %d: A majority is not formed for seq: %d, n: %d\n", px.me, seq, propNum)
				time.Sleep(time.Millisecond * time.Duration(100+rand.Int63n(100)))
			}
			_, decided = px.chosenSeqs[seq]
		}

	}()
}

//
// the application on this machine is done with
// all instances <= seq.
//
// see the comments for Min() for more explanation.
//
func (px *Paxos) Done(seq int) {
	// Your code here.
	px.mu.Lock()
	defer px.mu.Unlock()

	if seq > px.MaxDoneVal() {
		px.peersDone[px.me] = seq
	}
}

//
// the application wants to know the
// highest instance sequence known to
// this peer.
//
func (px *Paxos) Max() int {
	// Your code here.
	maxSeq := -1
	for seq, _ := range px.acceptorStates {
		if seq > maxSeq {
			maxSeq = seq
		}
	}
	return maxSeq
}

//
// Min() should return one more than the minimum among z_i,
// where z_i is the highest number ever passed
// to Done() on peer i. A peers z_i is -1 if it has
// never called Done().
//
// Paxos is required to have forgotten all information
// about any instances it knows that are < Min().
// The point is to free up memory in long-running
// Paxos-based servers.
//
// Paxos peers need to exchange their highest Done()
// arguments in order to implement Min(). These
// exchanges can be piggybacked on ordinary Paxos
// agreement protocol messages, so it is OK if one
// peers Min does not reflect another Peers Done()
// until after the next instance is agreed to.
//
// The fact that Min() is defined as a minimum over
// *all* Paxos peers means that Min() cannot increase until
// all peers have been heard from. So if a peer is dead
// or unreachable, other peers Min()s will not increase
// even if all reachable peers call Done. The reason for
// this is that when the unreachable peer comes back to
// life, it will need to catch up on instances that it
// missed -- the other peers therefor cannot forget these
// instances.
//
func (px *Paxos) Min() int {
	// You code here.
	px.mu.Lock()
	defer px.mu.Unlock()
	//fmt.Printf("%d paxos: %v\n", px.me, px.peersDone)
	minDone := px.MinDoneVal()
	px.FreeMemory(minDone)
	px.allinstanceFreeMem(minDone)
	return minDone + 1
}

//
// the application wants to know whether this
// peer thinks an instance has been decided,
// and if so what the agreed value is. Status()
// should just inspect the local peer state;
// it should not contact other Paxos peers.
//
func (px *Paxos) Status(seq int) (Fate, interface{}) {
	// Your code here.
	if seq < px.Min() {
		return Forgotten, nil
	} else if _, ok := px.chosenSeqs[seq]; ok {
		//else if val, ok := px.chosenVals[seq]; ok {
		val := px.acceptorStates[seq].AccpVal
		return Decided, val
	} else {
		return Pending, nil
	}
}

//
// tell the peer to shut itself down.
// for testing.
// please do not change this function.
//
func (px *Paxos) Kill() {
	px.dead = true
	if px.l != nil {
		px.l.Close()
	}
}

//
// the application wants to create a paxos peer.
// the ports of all the paxos peers (including this one)
// are in peers[]. this servers port is peers[me].
//

func Make(peers []string, me int, rpcs *rpc.Server) *Paxos {
	px := MakeComplete(peers, me, rpcs, "", false)
	px.needLocal = false
	return px
}

func MakeComplete(peers []string, me int, rpcs *rpc.Server, dir string, restart bool) *Paxos {
	px := &Paxos{}
	px.peers = peers
	px.me = me

	// Your initialization code here.
	px.dir = dir
	px.needLocal = true
	px.nextPropNum = make(map[int]int)
	px.acceptorStates = make(map[int]AcceptorState)
	//px.chosenVals = make(map[int]interface{})
	px.chosenSeqs = make(map[int]bool)
	px.peersDone = InitPeersDone(len(px.peers))
	px.peersMaxSeqs = InitPeersDone(len(px.peers))
	px.noonvoting = false

	if restart {
		if px.diskIntact() {
			fmt.Printf("Paxos %v Reading from Disk...\n", px.me)
			px.reconstructFromLocal()
			//px.reconstructFromRemote()
		} else {
			fmt.Printf("Paxos %v Disk gone, reading from others...\n", px.me)
			px.reconstructFromRemote()
			px.noonvoting = true
		}

		fmt.Printf("Restarted Paxos AcceptorStates: %v\n", px.acceptorStates)
	}

	if rpcs != nil {
		// caller will create socket &c
		rpcs.Register(px)
	} else {
		rpcs = rpc.NewServer()
		rpcs.Register(px)

		// prepare to receive connections from clients.
		// change "unix" to "tcp" to use over a network.
		os.Remove(peers[me]) // only needed for "unix"
		l, e := net.Listen("unix", peers[me])
		if e != nil {
			log.Fatal("listen error: ", e)
		}
		px.l = l

		// please do not change any of the following code,
		// or do anything to subvert it.

		// create a thread to accept RPC connections
		go func() {
			for px.dead == false {
				conn, err := px.l.Accept()
				if err == nil && px.dead == false {
					if px.unreliable && (rand.Int63()%1000) < 100 {
						// discard the request.
						conn.Close()
					} else if px.unreliable && (rand.Int63()%1000) < 200 {
						// process the request but force discard of reply.
						c1 := conn.(*net.UnixConn)
						f, _ := c1.File()
						err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
						if err != nil {
							fmt.Printf("shutdown: %v\n", err)
						}
						px.rpcCount++
						go rpcs.ServeConn(conn)
					} else {
						px.rpcCount++
						go rpcs.ServeConn(conn)
					}
				} else if err == nil {
					conn.Close()
				}
				if err != nil && px.dead == false {
					fmt.Printf("Paxos(%v) accept: %v\n", me, err.Error())
				}
			}
		}()
	}

	return px
}