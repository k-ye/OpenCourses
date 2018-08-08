package simpledb;
import java.util.*;

/**
 * The Join operator implements the relational join operation.
 */
public class Join extends AbstractDbIterator {
    private final DbIterator child1;
    private final DbIterator child2;
    private final JoinPredicate pred;
    private final TupleDesc td;
    private Tuple cur1;

    /**
     * Constructor.  Accepts two children to join and the predicate
     * to join them on
     *
     * @param p The predicate to use to join the children
     * @param child1 Iterator for the left(outer) relation to join
     * @param child2 Iterator for the right(inner) relation to join
     */
    public Join(JoinPredicate p, DbIterator child1, DbIterator child2) {
        this.child1 = child1;
        this.child2 = child2;
        this.pred = p;
        this.td = TupleDesc.combine(child1.getTupleDesc(), child2.getTupleDesc());
    }

    /**
     * @see simpledb.TupleDesc#combine(TupleDesc, TupleDesc) for possible implementation logic.
     */
    public TupleDesc getTupleDesc() {
        return td;
    }

    public void open()
        throws DbException, NoSuchElementException, TransactionAbortedException {
        child1.open();
        child2.open();
        resetCur1();
    }

    public void close() {
        child1.close();
        child2.close();
        cur1 = null;
    }

    public void rewind() throws DbException, TransactionAbortedException {
        child1.rewind();
        child2.rewind();
        resetCur1();
    }

    private void resetCur1() throws DbException, TransactionAbortedException {
        if (child1.hasNext()) {
            cur1 = child1.next();
        }
    }

    /**
     * Returns the next tuple generated by the join, or null if there are no more tuples.
     * Logically, this is the next tuple in r1 cross r2 that satisfies the join
     * predicate.  There are many possible implementations; the simplest is a
     * nested loops join.
     * <p>
     * Note that the tuples returned from this particular implementation of
     * Join are simply the concatenation of joining tuples from the left and
     * right relation. Therefore, if an equality predicate is used 
     * there will be two copies of the join attribute
     * in the results.  (Removing such duplicate columns can be done with an
     * additional projection operator if needed.)
     * <p>
     * For example, if one tuple is {1,2,3} and the other tuple is {1,5,6},
     * joined on equality of the first column, then this returns {1,2,3,1,5,6}.
     *
     * @return The next matching tuple.
     * @see JoinPredicate#filter
     */
    protected Tuple readNext() throws TransactionAbortedException, DbException {
        while (cur1 != null) {
            while (child2.hasNext()) {
                Tuple cur2 = child2.next();
                if (pred.filter(cur1, cur2)) {
                    Tuple result = new Tuple(td);
                    int t1NumFields = cur1.getTupleDesc().numFields();
                    int t2NumFields = cur2.getTupleDesc().numFields();
                    for (int i = 0; i < t1NumFields; ++i) {
                        result.setField(i, cur1.getField(i));
                    }
                    for (int i = 0; i < t2NumFields; ++i) {
                        result.setField(i + t1NumFields, cur2.getField(i));
                    }
                    return result;
                }
            }
            if (child1.hasNext()) {
                cur1 = child1.next();
                child2.rewind();
            } else {
                cur1 = null;
            }
        }
        return null;
    }
}
