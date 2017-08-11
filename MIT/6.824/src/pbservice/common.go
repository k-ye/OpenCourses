package pbservice

const (
	OK             = "OK"
	ErrNoKey       = "ErrNoKey"
	ErrWrongServer = "ErrWrongServer"
	ErrForwardReject = "ErrForwardReject" // occured on brain split
	ErrFailBackup = "ErrFailBackup" // occured when backup-fowarding failed
	ErrServerBusy = "ErrServerBusy" // occured when there's uncommited P/A ops
)

type Err string
type DatabaseType map[string]string

// Put or Append
type PutAppendArgs struct {
	Key   string
	Value string
	// You'll have to add definitions here.

	// Field names must start with capital letters,
	// otherwise RPC will break.
	Operation string // indicate the operation
	IsFromClient bool // indicate if this RPC is from Client
	From string // indicate sender's name
	ReqID string // indicate the Request ID
}

type PutAppendReply struct {
	Err Err
}

type GetArgs struct {
	Key string
	// You'll have to add definitions here.
	IsFromClient bool // indicate if this RPC is from Client
	From string // indicate sender's name
	ReqID string // indicate the Request ID
}

type GetReply struct {
	Err   Err
	Value string
}

// Your RPC definitions here.
// Argument for transfering the entire database from Primary to Backup
type TransferStateArgs struct {
	Database DatabaseType
	SeenReqIDs map[string]bool
	OldvaluesGet map[string]string
	LastUncommitPAReqID string
}

type TransferStateReply struct {
	Err Err
}