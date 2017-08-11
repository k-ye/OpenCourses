package kvpaxos

const (
	OK       = "OK"
	ErrNoKey = "ErrNoKey"

	Get    = "Get"
	Put    = "Put"
	Append = "Append"
	Nop    = "Nop"
)

type Err string

// Put or Append
type PutAppendArgs struct {
	// You'll have to add definitions here.
	Key   string
	Value string
	Op    string // "Put" or "Append"
	// You'll have to add definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	CkUID string // Client's UID
	ReqID string // indicate the Request ID
}

type PutAppendReply struct {
	Err Err
}

type GetArgs struct {
	Key string
	// You'll have to add definitions here.
	CkUID string // Client's UID
	ReqID string // indicate the Request ID
}

type GetReply struct {
	Err   Err
	Value string
}
