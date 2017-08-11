package paxos

// const values for Request result status
const (
	RequestOK = "RequestOK"
	Reject    = "Reject"
)

// State to be stored in an acceptor
type AcceptorState struct {
	Seq        int
	MaxPropNum int
	MaxAccpNum int
	AccpVal    interface{}
}

// Prepare request args
type PrepareArgs struct {
	PxID    int
	Seq     int
	PropNum int
}

// Prepare request reply
type PrepareReply struct {
	PxID int
	PrepareStatus string
	MaxPropNum    int
	MaxAccpNum    int
	AccpVal       interface{}

	SnderMaxSeq int
	RcverMaxSeq int
}

// Check to see if a prepare reply contains an accepted value
func (reply *PrepareReply) HasAcceptValue() bool {
	return reply.MaxAccpNum > -1 && (reply.AccpVal != nil)
}

// Accept request args
type AcceptArgs struct {
	PxID    int
	Seq     int
	PropNum int
	PropVal interface{}
}

// Accept request reply
type AcceptReply struct {
	AcceptStatus string
	PropNum      int

	SnderMaxSeq int
	RcverMaxSeq int
}

// Decide RPC handler args
type DecideArgs struct {
	PxID    int
	Seq     int
	PropNum int
	PropVal interface{}
	DoneVal int
}

// Dcide RPC handler reply
type DecideReply struct {
	PxID         int
	DecideStatus string
	DoneVal      int

	SnderMaxSeq int
	RcverMaxSeq int
}

type RequestPxStateArgs struct {
}

type RequestPxStateReply struct {
	AcceptorStates map[int]AcceptorState
	ChosenSeqs map[int]bool
	PeersDone []int
	PeersMaxSeqs []int
}

func (r *RequestPxStateReply) MergeReply(other *RequestPxStateReply) {
	// merge all the accptorStates, max(maxPropNum), max(maxAccpNum)
	for seq, oastate := range other.AcceptorStates {
		sastate, exists := r.AcceptorStates[seq]
		if !exists {
			r.AcceptorStates[seq] = oastate
		} else {
			// don't need to update AccpVal as we know these two should be the same
			sastate.MaxPropNum = MaxInt(sastate.MaxPropNum, oastate.MaxPropNum)
			sastate.MaxAccpNum = MaxInt(sastate.MaxAccpNum, oastate.MaxAccpNum)
			r.AcceptorStates[seq] = sastate
		}
	}
	// merge all chosenSeqs
	for seq, _ := range other.ChosenSeqs {
		r.ChosenSeqs[seq] = true
	}
	// merge all other PeersDone
	for pxid, val := range other.PeersDone {
		r.PeersDone[pxid] = MaxInt(r.PeersDone[pxid], val)
	}

	for pxid, val := range other.PeersMaxSeqs {
		r.PeersMaxSeqs[pxid] = MaxInt(r.PeersMaxSeqs[pxid], val)
	}
}