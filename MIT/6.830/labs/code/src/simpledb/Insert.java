package simpledb;
import java.io.IOException;
import java.util.*;

/**
 * Inserts tuples read from the child operator into
 * the tableid specified in the constructor
 */
public class Insert extends AbstractDbIterator {
    private final TransactionId txId;
    private DbIterator child;
    private final int tableId;
    private final TupleDesc td;

    /**
     * Constructor.
     * @param t The transaction running the insert.
     * @param child The child operator from which to read tuples to be inserted.
     * @param tableid The table in which to insert tuples.
     * @throws DbException if TupleDesc of child differs from table into which we are to insert.
     */
    public Insert(TransactionId t, DbIterator child, int tableid)
        throws DbException {
        this.txId = t;
        this.child = child;
        this.tableId = tableid;
        this.td = new TupleDesc(new Type[]{Type.INT_TYPE}, new String[]{"inserted"});
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
     * Inserts tuples read from child into the tableid specified by the
     * constructor. It returns a one field tuple containing the number of
     * inserted records. Inserts should be passed through BufferPool.
     * An instances of BufferPool is available via Database.getBufferPool().
     * Note that insert DOES NOT need check to see if a particular tuple is
     * a duplicate before inserting it.
     *
     * @return A 1-field tuple containing the number of inserted records, or
    * null if called more than once.
     * @see Database#getBufferPool
     * @see BufferPool#insertTuple
     */
    protected Tuple readNext()
            throws TransactionAbortedException, DbException {
        if (child != null) {
            try {
                int count = 0;
                while (child.hasNext()) {
                    Tuple t = child.next();
                    Database.getBufferPool().insertTuple(txId, tableId, t);
                    count += 1;
                }
                child = null;
                Tuple result = new Tuple(new TupleDesc(new Type[]{Type.INT_TYPE}));
                result.setField(0, new IntField(count));
                return result;
            } catch (IOException e) {
                throw new DbException(e.getMessage());
            }
        }
        return null;
    }
}
