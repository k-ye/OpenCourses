package simpledb;
import java.util.*;

/**
 * Filter is an operator that implements a relational select.
 */
public class Filter extends AbstractDbIterator {

    /**
     * Constructor accepts a predicate to apply and a child
     * operator to read tuples to filter from.
     *
     * @param p The predicate to filter tuples with
     * @param child The child operator
     */
    private final Predicate pred;
    private final DbIterator childIter;

    public Filter(Predicate p, DbIterator child) {
        this.pred = p;
        this.childIter = child;
    }

    public TupleDesc getTupleDesc() {
        return childIter.getTupleDesc();
    }

    public void open()
        throws DbException, NoSuchElementException, TransactionAbortedException {
        childIter.open();
    }

    public void close() {
        childIter.close();
    }

    public void rewind() throws DbException, TransactionAbortedException {
        childIter.rewind();
    }

    /**
     * AbstractDbIterator.readNext implementation.
     * Iterates over tuples from the child operator, applying the predicate
     * to them and returning those that pass the predicate (i.e. for which
     * the Predicate.filter() returns true.)
     *
     * @return The next tuple that passes the filter, or null if there are no more tuples
     * @see Predicate#filter
     */
    protected Tuple readNext()
        throws NoSuchElementException, TransactionAbortedException, DbException {
        while (childIter.hasNext()) {
            Tuple cur = childIter.next();
            if (pred.filter(cur)) {
                return cur;
            }
        }
        return null;
    }
}
