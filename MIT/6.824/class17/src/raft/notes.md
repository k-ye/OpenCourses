This is the note/draft for lab2's Raft implementation. It may contain either pseudo code or real `golang` code.

```go
package notes

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

type raft struct {
	state       int
	currentTerm int
	votedFor    int
	voteCount   int

	mu             Mutex
	resetTimerChan EventChan // buffered of size 1
	skipTimerChan  EventChan // buffered of size 1
}

func (rf *raft) getTimeoutInterval() int {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if rf.state == LEADER {
		return HEARTBEAT_INTERVAL
	}
	var t int
	// t = some random number between [550, 800)
	return t
}

func (rf *raft) tick() {
	for {
		sleepInterval := rf.getTimeoutInterval()
		select {
		case <-time.After(sleepInterval):
		case <-rf.skipTimerChan:
		case <-rf.resetTimerChan:
			continue
		}

		rf.mu.Lock()
		if rf.state == LEADER {
			go rf.bcastAppendEntries()
		} else {
			rf.becomeCandidate()
			// it is important to spawn another goroutine, otherwise
			// the system will deadlock
			go rf.bcastRequestVote()
		}
		clearEvent(rf.skipTimerChan)
		clearEvent(rf.resetTimerChan)

		rf.mu.Unlock()
	}
}

func (rf *raft) bcastAppendEntries() {
	args := RequestVoteArgs{Term: rf.currentTerm, LeaderId: rf.me}
	for i := 0; i < len(rf.peers); i++ {
		if i != rf.me {
			go rf.sendAppendEntries(i, args)
		}
	}
}

func (rf *raft) sendAppendEntries(i int, args AppendEntriesArgs) {
	var reply AppendEntriesReply
	ok := rf.peers[i].Call("raft.AppendEntries", args, &reply)
	rf.handleAppendEntriesReply(ok, reply)
}

func (rf *raft) AppendEntries(args AppendEntriesArgs, reply *AppendEntriesReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if args.Term < rf.currentTerm {
		// reject
		reply.Term = rf.currentTerm
		reply.Success = false
		return
	}

	if args.Term > rf.currentTerm {
		rf.fallbackToFollower(args.Term)
	} else if args.Term == rf.currentTerm {
		DCheck(rf.state != LEADER, "AppendEntries(): multiple leaders in same term")
		rf.becomeFollower()
	}
}

func (rf *raft) handleAppendEntriesReply(rpcOk bool, reply AppendEntriesReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	if ok {
		if reply.Term > rf.currentTerm {
			rf.fallbackToFollower(reply.Term)
		}
	}
}

func (rf *raft) bcastRequestVote() {
	args := RequestVoteArgs{Term: rf.currentTerm, CandidateId: rf.me}
	rf.voteCount = 1
	for i := 0; i < len(rf.peers); i++ {
		if i != rf.me {
			go rf.sendRequestVote(i, args)
		}
	}
}

func (rf *raft) sendRequestVote(i int, args RequestVoteArgs) {
	var reply RequestVoteReply
	ok := rf.peers[i].Call("raft.RequestVote", args, &reply)
	rf.handleRequestVoteReply(ok, reply)
}

func (rf *raft) RequestVote(args RequestVoteArgs, reply *RequestVoteReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	reply.Term = rf.currentTerm
	reply.VoteGranted = false
	if args.Term < rf.currentTerm {
		// reject
		return
	} else if rf.currentTerm < args.Term {
		rf.fallbackToFollower(args.Term)
	}
	if rf.votedFor == -1 || rf.votedFor == args.CandidateId {
		DCheck(rf.state != CANDIDATE, "raft=%d RequestVote(): shouldn't grant vote as CANDIDATE\n", rf.me)
		DCheck(rf.state != LEADER, "raft=%d RequestVote(): shouldn't grant vote as LEADER\n", rf.me)
		reply.VoteGranted = true
		rf.votedFor = args.CandidateId
		rf.becomeFollower()
	}
}

func (rf *raft) handleRequestVoteReply(rpcOk bool, reply RequestVoteReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if rpcOk && reply.Term > rf.currentTerm {
		rf.fallbackToFollower(reply.Term)
		return
	}
	if rf.state == CANDIDATE && rpcOk && reply.VoteGranted {
		rf.voteCount++
		if rf.isMajority() {
			rf.becomeLeader()
		}
	}
}

func (rf *raft) fallbackToFollower(term int) {
	// |rf.mu| is held
	DCheck(rf.currentTerm < term, "Term is smaller when fallback to FOLLOWER\n")

	rf.currentTerm = term
	rf.votedFor = -1
	rf.becomeFollower()
}

func (rf *raft) becomeFollower() {
	// |rf.mu| is held
	rf.state = FOLLOWER
	setEvent(rf.resetTimerChan)
}

func (rf *raft) becomeCandidate() {
	// |rf.mu| is held
	rf.state = CANDIDATE
	rf.currentTerm++
	rf.votedFor = rf.me
}

func (rf *raft) becomeLeader() {
	// |rf.mu| is held
	rf.state = LEADER
	setEvent(rf.skipTimerChan)
}
```