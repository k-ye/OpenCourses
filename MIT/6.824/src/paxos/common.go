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
	PrepareStatus string
	MaxPropNum    int
	MaxAccpNum    int
	AccpVal       interface{}
}

// Check to see if a prepare reply contains an accepted value
func (reply *PrepareReply) HasAcceptValue() bool {
	return reply.MaxAccpNum > -1
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
}
