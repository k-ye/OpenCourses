package raft

//
// this is an outline of the API that raft must expose to
// the service (or tester). see comments below for
// each of these functions for more details.
//
// rf = Make(...)
//   create a new Raft server.
// rf.Start(command interface{}) (index, term, isleader)
//   start agreement on a new log entry
// rf.GetState() (term, isLeader)
//   ask a Raft for its current term, and whether it thinks it is leader
// ApplyMsg
//   each time a new entry is committed to the log, each Raft peer
//   should send an ApplyMsg to the service (or tester)
//   in the same server.
//

import (
	"fmt"
	"labrpc"
	"math/rand"
	"sync"
	"time"
	// "bytes"
	// "encoding/gob"
)

//
// as each Raft peer becomes aware that successive log entries are
// committed, the peer should send an ApplyMsg to the service (or
// tester) on the same server, via the applyCh passed to Make().
//
type ApplyMsg struct {
	Index       int
	Command     interface{}
	UseSnapshot bool   // ignore for lab2; only used in lab3
	Snapshot    []byte // ignore for lab2; only used in lab3
}

type LogEntry struct {
	Term    int
	Command interface{}
}

// Helpers to use channels as event sync
type EventChan chan bool

func clearEvent(c EventChan) {
	select {
	case <-c:
	default:
	}
}

func setEvent(c EventChan) {
	clearEvent(c)
	c <- true
}

func makeEvent() EventChan {
	ch := make(chan bool, 1)
	return ch
}

// Raft State constants
const (
	FOLLOWER  int = 0
	CANDIDATE int = 1
	LEADER    int = 2
)

const (
	kHeartbeatIntervalMs int = 105
	kEletionTimeoutLoMs  int = 500
	kEletionTimeoutStdMs int = 300
)

//
// A Go object implementing a single Raft peer.
//
type Raft struct {
	mu        sync.Mutex          // Lock to protect shared access to this peer's state
	peers     []*labrpc.ClientEnd // RPC end points of all peers
	persister *Persister          // Object to hold this peer's persisted state
	me        int                 // this peer's index into peers[]

	// Your data here (2A, 2B, 2C).
	// Look at the paper's Figure 2 for a description of what
	// state a Raft server must maintain.

	// Invariants
	// - |len(logEntries)| starts at 1 and will always stay >= 1.
	// - |numLogEntries()| == |len(logEntries) - 1|
	// - index 0 for |logEntries| is invalid
	// - index range for |logEntries| are 1, 2, ..., |numLogEntries()|; Or, 1 to |len(logEntries) - 1|
	// - term 0 is invalid

	// persistent
	state       int
	currentTerm int
	votedFor    int
	// numVotes    int
	logEntries []LogEntry
	// volatile
	commitIndex int
	// lastAppliedIndex int
	// volatile, only for LEADER
	peerNextIndex  []int
	peerMatchIndex []int

	resetTimerCh    EventChan
	skipTimerCh     EventChan
	commitUpdatedCh EventChan
	applyCh         chan ApplyMsg
	// system status
	alive     bool
	debugging bool
}

func (rf *Raft) logsf(format string, a ...interface{}) string {
	ts := time.Now().Format("15:04:05.000000")
	args := append(make([]interface{}, 0), ts, rf.me)
	args = append(args, a...)
	return fmt.Sprintf("%s Raft=%d :: "+format, args...)
}

func (rf *Raft) dlogf(format string, a ...interface{}) {
	if rf.debugging {
		TsPrintf(rf.logsf(format, a...))
	}
}

func (rf *Raft) dcheck(cond bool, format string, a ...interface{}) {
	if rf.debugging {
		Check(cond, rf.logsf(format, a...))
	}
}

func GetStateName(s int) string {
	switch s {
	case LEADER:
		return "LEADER"
	case CANDIDATE:
		return "CANDIDATE"
	case FOLLOWER:
		return "FOLLOWER"
	}
	return "UNKNOWN_STATE"
}

// Returns the string representing the current state of |rf|
func (rf *Raft) getStateName() string {
	return GetStateName(rf.state)
}

// Checks a subset of size |count| is a majority of
// the full set of size |total|.
// Note: |total| should be odd
func hasMajority(count, total int) bool {
	return count >= ((total + 1) / 2)
}

// Returns the "logical" number of entries in the log.
// Note: This is |len(rf.logEntries)| - 1 because we are using the
// 0-th element as a placeholder.
func (rf *Raft) numLogEntries() int {
	// |rf.mu| is held
	return len(rf.logEntries) - 1
}

// Get the index of the last log entry.
func (rf *Raft) getLastLogIndex() int {
	// |rf.mu| is held
	return rf.numLogEntries()
}

// Get the Term of the log entry indexed at |i|
func (rf *Raft) getLogTermAt(i int) int {
	// |rf.mu| does NOT need to be held
	return rf.logEntries[i].Term
}

// Get the Term of the last log entry.
func (rf *Raft) getLastLogTerm() int {
	// |rf.mu| is held
	return rf.getLogTermAt(rf.getLastLogIndex())
}

func (rf *Raft) becomeFollower() {
	// |rf.mu| is held
	rf.state = FOLLOWER
	setEvent(rf.resetTimerCh)
}

// This function is different from |becomeFollower| in that it gets invoked when
// |rf| detects, in the RPC reply handler, that its |currentTerm| is outdated,
// which means |rf| should fall back to FOLLOWER immediately.
func (rf *Raft) fallbackToFollower(newTerm int) {
	// |rf.mu| is held
	rf.dcheck(newTerm > rf.currentTerm, "newTerm=%d is not greater than currentTerm=%d\n", newTerm, rf.currentTerm)
	rf.currentTerm = newTerm
	rf.votedFor = -1

	rf.becomeFollower()
}

func (rf *Raft) becomeCandidate() {
	// |rf.mu| is held
	rf.state = CANDIDATE

	rf.currentTerm++
	rf.votedFor = rf.me
}

func (rf *Raft) becomeLeader() {
	// |rf.mu| is held
	rf.state = LEADER

	numPeers := len(rf.peers)
	rf.peerNextIndex = make([]int, numPeers)
	rf.peerMatchIndex = make([]int, numPeers)
	for i := 0; i < numPeers; i++ {
		rf.peerNextIndex[i] = rf.getLastLogIndex() + 1
		rf.peerMatchIndex[i] = 0
	}
	rf.peerMatchIndex[rf.me] = rf.getLastLogIndex()

	setEvent(rf.skipTimerCh)
}

// return currentTerm and whether this server
// believes it is the leader.
func (rf *Raft) GetState() (int, bool) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	term := rf.currentTerm
	isLeader := (rf.state == LEADER)
	// Your code here (2A).
	return term, isLeader
}

//
// save Raft's persistent state to stable storage,
// where it can later be retrieved after a crash and restart.
// see paper's Figure 2 for a description of what should be persistent.
//
func (rf *Raft) persist() {
	// Your code here (2C).
	// Example:
	// w := new(bytes.Buffer)
	// e := gob.NewEncoder(w)
	// e.Encode(rf.xxx)
	// e.Encode(rf.yyy)
	// data := w.Bytes()
	// rf.persister.SaveRaftState(data)
}

//
// restore previously persisted state.
//
func (rf *Raft) readPersist(data []byte) {
	// Your code here (2C).
	// Example:
	// r := bytes.NewBuffer(data)
	// d := gob.NewDecoder(r)
	// d.Decode(&rf.xxx)
	// d.Decode(&rf.yyy)
	if data == nil || len(data) < 1 { // bootstrap without any state?
		return
	}
}

//
// example RequestVote RPC arguments structure.
// field names must start with capital letters!
//
type RequestVoteArgs struct {
	// Your data here (2A, 2B).
	Term         int
	State        int
	CandidateId  int
	LastLogIndex int
	LastLogTerm  int
}

//
// example RequestVote RPC reply structure.
// field names must start with capital letters!
//
type RequestVoteReply struct {
	// Your data here (2A).
	Term        int
	VoteGranted bool
	// Non-standard Raft message
	By int
}

// Checks if the peer requested for vote has its log
// at least as up-to-date as that of |rf|.
func (rf *Raft) isRequestVodeUpToDate(args *RequestVoteArgs) bool {
	// |rf.mu| is held
	if args.LastLogTerm != rf.getLastLogTerm() {
		return args.LastLogTerm > rf.getLastLogTerm()
	}
	return args.LastLogIndex >= rf.getLastLogIndex()
}

//
// example RequestVote RPC handler.
//
func (rf *Raft) RequestVote(args *RequestVoteArgs, reply *RequestVoteReply) {
	// Your code here (2A, 2B).
	rf.mu.Lock()
	defer rf.mu.Unlock()

	reply.Term = rf.currentTerm
	reply.VoteGranted = false
	reply.By = rf.me

	if args.Term < rf.currentTerm || args.State != CANDIDATE {
		// It could be that by the time the peer sent out this request, it is
		// no longer CANDIDATE.
		// |rf| could be in any of the three states
		return
	} else if rf.currentTerm < args.Term {
		rf.fallbackToFollower(args.Term)
		// We cannot return yet because |rf| might grant vote now
	}

	if (rf.votedFor == -1 || rf.votedFor == args.CandidateId) && rf.isRequestVodeUpToDate(args) {
		// |rf| must be in FOLLOWER state, since
		// 1. |rf.votedFor| == -1 => |rf| is a FOLLOWER
		// 2. if |rf| is CANDIDATE or LEADER, then |rf.votedFor| == |rf.me|, and
		//      because a Raft peer doesn't send RPC calls to itself, |args.CandidateId| != |rf.me|.
		rf.dcheck(rf.state == FOLLOWER, "Only FOLLOWER can grant vote, state=%s", rf.getStateName())
		rf.votedFor = args.CandidateId
		reply.VoteGranted = true
		rf.dlogf("Granted vote to peer=%d, term=%d\n", rf.votedFor, rf.currentTerm)
	}
}

//
// example code to send a RequestVote RPC to a server.
// server is the index of the target server in rf.peers[].
// expects RPC arguments in args.
// fills in *reply with RPC reply, so caller should
// pass &reply.
// the types of the args and reply passed to Call() must be
// the same as the types of the arguments declared in the
// handler function (including whether they are pointers).
//
// The labrpc package simulates a lossy network, in which servers
// may be unreachable, and in which requests and replies may be lost.
// Call() sends a request and waits for a reply. If a reply arrives
// within a timeout interval, Call() returns true; otherwise
// Call() returns false. Thus Call() may not return for a while.
// A false return can be caused by a dead server, a live server that
// can't be reached, a lost request, or a lost reply.
//
// Call() is guaranteed to return (perhaps after a delay) *except* if the
// handler function on the server side does not return.  Thus there
// is no need to implement your own timeouts around Call().
//
// look at the comments in ../labrpc/labrpc.go for more details.
//
// if you're having trouble getting RPC to work, check that you've
// capitalized all field names in structs passed over RPC, and
// that the caller passes the address of the reply struct with &, not
// the struct itself.
//
func (rf *Raft) sendRequestVote(server int, args *RequestVoteArgs, numVotes *int) bool {
	var reply RequestVoteReply
	ok := rf.peers[server].Call("Raft.RequestVote", args, &reply)
	rf.handleRequestVoteReply(server, args, ok, &reply, numVotes)
	return ok
}

func (rf *Raft) handleRequestVoteReply(server int, args *RequestVoteArgs, rpcOk bool, reply *RequestVoteReply, numVotes *int) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if rpcOk {
		if rf.currentTerm < reply.Term {
			rf.fallbackToFollower(reply.Term)
			return
		}
		if (rf.state != CANDIDATE) || (args.Term != rf.currentTerm) {
			// check for stale Raft state
			// check for stale RPC reply
			return
		}
		if reply.VoteGranted {
			(*numVotes)++
			rf.dlogf("state=CANDIDATE, got %d votes\n", *numVotes)
			if hasMajority(*numVotes, len(rf.peers)) {
				rf.becomeLeader()
				rf.dlogf("Enough votes, became LEADER at term=%d\n", rf.currentTerm)
			}
		}
	}
}

func (rf *Raft) broadcastRequestVote() {
	// The lock is not required for (2A), but it might be needed in futur'e labs.
	rf.mu.Lock()
	args := RequestVoteArgs{Term: rf.currentTerm, State: rf.state, CandidateId: rf.me}
	args.LastLogIndex = rf.getLastLogIndex()
	args.LastLogTerm = rf.getLastLogTerm()
	rf.mu.Unlock()
	numVotes := 1
	for i := 0; i < len(rf.peers); i++ {
		if i != rf.me {
			go rf.sendRequestVote(i, &args, &numVotes)
		}
	}
}

func (rf *Raft) updateCommitIndex(c int) {
	// |rf.mu| is held
	if rf.commitIndex < c {
		rf.commitIndex = c
		setEvent(rf.commitUpdatedCh)
	}
}

//
// AppendEntries RPC args and reply types
//
type AppendEntriesArgs struct {
	Term              int
	State             int
	LeaderId          int
	PrevLogIndex      int
	PrevLogTerm       int
	Entries           []LogEntry
	LeaderCommitIndex int
}

type AppendEntriesReply struct {
	Term    int
	Success bool // probably not used in (2A)
	// used when |Success| == true
	MatchIndex int
	// used when |Success| == false
	// NextIndex int
}

func (rf *Raft) AppendEntries(args *AppendEntriesArgs, reply *AppendEntriesReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	reply.Term = rf.currentTerm
	reply.Success = false
	if args.Term < rf.currentTerm || args.State != LEADER {
		// It could be that by the time the peer sent out this request, it is
		// no longer LEADER.
		return
	} else if rf.currentTerm < args.Term {
		rf.fallbackToFollower(args.Term)
	} else {
		// |rf.currentTerm| == |args.Term|
		rf.dcheck(rf.state != LEADER, "AppendEntries(): multiple leaders in the same term=%d\n", rf.currentTerm)
		rf.becomeFollower()
	}

	if rf.getLastLogIndex() < args.PrevLogIndex || rf.getLogTermAt(args.PrevLogIndex) != args.PrevLogTerm {
		// consistency check failed
		reply.Success = false
		// TODO: set |reply.NextIndex|
		return
	}

	newLogIndexBegin := args.PrevLogIndex + 1
	for i := 0; i < len(args.Entries); i++ {
		newLogIndex := newLogIndexBegin + i
		if rf.getLastLogIndex() < newLogIndex || rf.getLogTermAt(newLogIndex) != args.Entries[i].Term {
			// truncate the log only in face of a conflict
			rf.logEntries = append(rf.logEntries[:newLogIndex], args.Entries[i:]...)
		}
	}

	reply.MatchIndex = args.PrevLogIndex + len(args.Entries)
	rf.dcheck(reply.MatchIndex <= rf.getLastLogIndex(),
		"FOLLOWER updating log, MatchIndex=%d > LastLogIndex=%d\n", reply.MatchIndex, rf.getLastLogIndex())
	reply.Success = true

	if args.LeaderCommitIndex > rf.commitIndex {
		rf.updateCommitIndex(Min(args.LeaderCommitIndex, reply.MatchIndex))
	}
}

func (rf *Raft) sendAppendEntries(server int) bool {
	rf.mu.Lock()
	args := AppendEntriesArgs{Term: rf.currentTerm, State: rf.state, LeaderId: rf.me}
	args.PrevLogIndex = rf.peerNextIndex[server] - 1
	args.PrevLogTerm = rf.getLogTermAt(args.PrevLogIndex)
	args.Entries = rf.logEntries[(args.PrevLogIndex + 1):]
	args.LeaderCommitIndex = rf.commitIndex

    if args.State != LEADER {
        rf.dlogf("Sending out AppendEntries as state=%s anyway, it will be rejected\n", rf.getStateName())
    }
	rf.mu.Unlock()

	var reply AppendEntriesReply
	ok := rf.peers[server].Call("Raft.AppendEntries", &args, &reply)
	rf.handleAppendEntries(server, &args, ok, &reply)
	return ok
}

// Tries to update |rf.commitIndex| when it is LEADER. This requires that
// 1. |rf| knows that a majority of the peers has stored some log entry at an index > |rf.commitIndex|
// 2. The term of the log entry at that index == |rf.currentTerm|
func (rf *Raft) tryUpdateCommitIndexAsLeader() {
	// |rf.mu| is held
	rf.dcheck(rf.state == LEADER, "Only LEADER calls tryUpdateCommitIndexAsLeader(), state=%s\n", rf.getStateName())
	N, count := rf.getLastLogIndex(), 0
	if N <= rf.commitIndex {
		return
	}
	for i := 0; i < len(rf.peers); i++ {
		if rf.peerMatchIndex[i] > rf.commitIndex {
			N = Min(N, rf.peerMatchIndex[i])
			count++
		}
	}
	if hasMajority(count, len(rf.peers)) && (rf.getLogTermAt(N) == rf.currentTerm) {
		rf.updateCommitIndex(N)
	}
}

func (rf *Raft) handleAppendEntries(server int, args *AppendEntriesArgs, rpcOk bool, reply *AppendEntriesReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if rpcOk {
		if rf.currentTerm < reply.Term {
			rf.fallbackToFollower(reply.Term)
			return
		}
		if (rf.state != LEADER) || (args.Term != rf.currentTerm) {
			// check for stale Raft state
			// check for stale RPC reply
			return
		}

		if reply.Success {
			rf.peerNextIndex[server] = Max(rf.peerNextIndex[server], reply.MatchIndex+1)
			rf.peerMatchIndex[server] = Max(rf.peerMatchIndex[server], reply.MatchIndex)
			rf.tryUpdateCommitIndexAsLeader()
		} else {
			rf.peerNextIndex[server]--
			// this won't deadlock because it is on another gorountine
			go rf.sendAppendEntries(server)
		}
	}
}

func (rf *Raft) broadcastAppendEntries() {
	// The lock is not required for (2A), but it might be needed in future labs.
	// rf.mu.Lock()
	// args := AppendEntriesArgs{Term: rf.currentTerm, LeaderId: rf.me}
	// rf.mu.Unlock()

	for i := 0; i < len(rf.peers); i++ {
		if i != rf.me {
			go rf.sendAppendEntries(i)
		}
	}
}

func (rf *Raft) getTimeoutInterval() time.Duration {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	var intvMs int
	if rf.state == LEADER {
		intvMs = kHeartbeatIntervalMs
	} else {
		seed := (time.Now().Unix() % 10000) + int64(rf.me*10000)
		rand.Seed(seed)
		intvMs = kEletionTimeoutLoMs + rand.Intn(kEletionTimeoutStdMs)
	}
	return time.Duration(intvMs) * time.Millisecond
}

func (rf *Raft) tick() {
	for rf.alive {
		select {
		case <-time.After(rf.getTimeoutInterval()):
		case <-rf.skipTimerCh:
		case <-rf.resetTimerCh:
			continue
		}

		rf.mu.Lock()
		if rf.state == LEADER {
			go rf.broadcastAppendEntries()
		} else {
			rf.dlogf("Election Timeout! state=%s, term=%d\n", rf.getStateName(), rf.currentTerm+1)
			rf.becomeCandidate()
			go rf.broadcastRequestVote()
		}
		// This won't interfere with other goroutines because |rf.mu| is held
		clearEvent(rf.skipTimerCh)
		clearEvent(rf.resetTimerCh)
		rf.mu.Unlock()
	}
}

func (rf *Raft) sendApplyMsg() {
	lastAppliedIndex := 0
	for rf.alive {
		select {
		case <-time.After(50 * time.Millisecond):
		case <-rf.commitUpdatedCh:
			i := lastAppliedIndex + 1
			curCommitIndex := rf.commitIndex
			for ; i <= curCommitIndex; i++ {
				rf.applyCh <- ApplyMsg{Index: i, Command: rf.logEntries[i].Command, UseSnapshot: false}
				lastAppliedIndex = i
			}
		}
	}
}

//
// the service using Raft (e.g. a k/v server) wants to start
// agreement on the next command to be appended to Raft's log. if this
// server isn't the leader, returns false. otherwise start the
// agreement and return immediately. there is no guarantee that this
// command will ever be committed to the Raft log, since the leader
// may fail or lose an election.
//
// the first return value is the index that the command will appear at
// if it's ever committed. the second return value is the current
// term. the third return value is true if this server believes it is
// the leader.
//
func (rf *Raft) Start(command interface{}) (index int, term int, isLeader bool) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	index = 0
	term = rf.currentTerm
	isLeader = (rf.state == LEADER)

	// Your code here (2B).
	if !isLeader {
		return
	}

	newEntry := LogEntry{Term: term, Command: command}
	rf.logEntries = append(rf.logEntries, newEntry)
	index = rf.getLastLogIndex()
	rf.peerNextIndex[rf.me] = index + 1
	rf.peerMatchIndex[rf.me] = index

	return
}

//
// the tester calls Kill() when a Raft instance won't
// be needed again. you are not required to do anything
// in Kill(), but it might be convenient to (for example)
// turn off debug output from this instance.
//
func (rf *Raft) Kill() {
	// Your code here, if desired.
	rf.alive = false
	rf.debugging = false
}

//
// the service or tester wants to create a Raft server. the ports
// of all the Raft servers (including this one) are in peers[]. this
// server's port is peers[me]. all the servers' peers[] arrays
// have the same order. persister is a place for this server to
// save its persistent state, and also initially holds the most
// recent saved state, if any. applyCh is a channel on which the
// tester or service expects Raft to send ApplyMsg messages.
// Make() must return quickly, so it should start goroutines
// for any long-running work.
//
func Make(peers []*labrpc.ClientEnd, me int,
	persister *Persister, applyCh chan ApplyMsg) *Raft {
	rf := &Raft{}
	rf.peers = peers
	rf.persister = persister
	rf.me = me

	// Your initialization code here (2A, 2B, 2C).
	rf.state = FOLLOWER
	rf.currentTerm = 0
	rf.votedFor = -1
	// logEntries[0] is unused for the convenience of index computation
	rf.logEntries = make([]LogEntry, 1)
	rf.logEntries[0] = LogEntry{Term: 0, Command: nil}

	rf.commitIndex = 0

	rf.resetTimerCh = makeEvent()
	rf.skipTimerCh = makeEvent()
	rf.commitUpdatedCh = makeEvent()
	rf.applyCh = applyCh

	rf.alive = true
	rf.debugging = true

	// initialize from state persisted before a crash
	rf.readPersist(persister.ReadRaftState())

	go rf.tick()
	go rf.sendApplyMsg()

	return rf
}
