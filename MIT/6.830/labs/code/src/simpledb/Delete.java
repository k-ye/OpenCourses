package simpledb;

/**
 * The delete operator.  Delete reads tuples from its child operator and
 * removes them from the table they belong to.
 */
public class Delete extends AbstractDbIterator {
    private final TransactionId txId;
    private DbIterator child;
    private final TupleDesc td;
    /**
     * Constructor specifying the transaction that this delete belongs to as
     * well as the child to read from.
     * @param t The transaction this delete runs in
     * @param child The child operator from which to read tuples for deletion
     */
    public Delete(TransactionId t, DbIterator child) {
        this.txId = t;
        this.child = child;
        this.td = new TupleDesc(new Type[]{Type.INT_TYPE}, new String[]{"deleted"});
    }

    public TupleDesc getTupleDesc() {
        return td;
    }

    public void open() throws DbException, TransactionAbortedException {
        if (child != null) {
            child.open();
        }
    }

    public void close() {
        if (child != null) {
            child.close();
        }
    }

    public void rewind() throws DbException, TransactionAbortedException {
        if (child != null) {
            child.rewind();
        }
    }

    /**
     * Deletes tuples as they are read from the child operator. Deletes are
     * processed via the buffer pool (which can be accessed via the
     * Database.getBufferPool() method.
     * @return A 1-field tuple containing the number of deleted records.
     * @see Database#getBufferPool
     * @see BufferPool#deleteTuple
     */
    protected Tuple readNext() throws TransactionAbortedException, DbException {
        if (child != null) {
            int count = 0;
            while (child.hasNext()) {
                Tuple t = child.next();
                Database.getBufferPool().deleteTuple(txId, t);
                count += 1;
            }
            child = null;
            Tuple result = new Tuple(new TupleDesc(new Type[]{Type.INT_TYPE}));
            result.setField(0, new IntField(count));
            return result;
        }
        return null;
    }
}
