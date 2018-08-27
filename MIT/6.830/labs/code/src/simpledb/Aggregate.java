package simpledb;

import java.util.*;

/**
 * The Aggregator operator that computes an aggregate (e.g., sum, avg, max,
 * min).  Note that we only support aggregates over a single column, grouped
 * by a single column.
 */
public class Aggregate extends AbstractDbIterator {
    private final DbIterator child;
    private final Aggregator aggregator;
    private AbstractDbIterator aggrIter;
    /**
     * Constructor.  
     *
     *  Implementation hint: depending on the type of afield, you will want to construct an 
     *  IntAggregator or StringAggregator to help you with your implementation of readNext().
     * 
     *
     * @param child The DbIterator that is feeding us tuples.
     * @param afield The column over which we are computing an aggregate.
     * @param gfield The column over which we are grouping the result, or -1 if there is no grouping
     * @param aop The aggregation operator to use
     */
    public Aggregate(DbIterator child, int afield, int gfield, Aggregator.Op aop) {
        this.child = child;
        this.aggrIter = null;

        TupleDesc td = child.getTupleDesc();
        Type atype = td.getType(afield);
        Type gtype;
        if (gfield == Aggregator.NO_GROUPING) {
            gtype = Type.INT_TYPE;  // Fake type
        } else {
            gtype = td.getType(gfield);
        }

        if (atype == Type.INT_TYPE) {
            this.aggregator = new IntAggregator(gfield, gtype, afield, aop);
        } else {
            assert (atype == Type.STRING_TYPE);
            this.aggregator = new StringAggregator(gfield, gtype, afield, aop);
        }
    }

    private void initAggrIterIfNull() throws DbException, TransactionAbortedException {
        if (aggrIter != null) {
            return;
        }
        child.open();
        while (child.hasNext()) {
            aggregator.merge(child.next());
        }
        child.close();
        aggrIter = aggregator.iterator();
    }

    public static String aggName(Aggregator.Op aop) {
        switch (aop) {
        case MIN:
            return "min";
        case MAX:
            return "max";
        case AVG:
            return "avg";
        case SUM:
            return "sum";
        case COUNT:
            return "count";
        }
        return "";
    }

    public void open()
        throws NoSuchElementException, DbException, TransactionAbortedException {
        initAggrIterIfNull();
        aggrIter.open();
    }

    /**
     * Returns the next tuple.  If there is a group by field, then 
     * the first field is the field by which we are
     * grouping, and the second field is the result of computing the aggregate,
     * If there is no group by field, then the result tuple should contain
     * one field representing the result of the aggregate.
     * Should return null if there are no more tuples.
     */
    protected Tuple readNext() throws TransactionAbortedException, DbException {
        if (aggrIter == null) {
            return null;
        }
        return aggrIter.readNext();
    }

    public void rewind() throws DbException, TransactionAbortedException {
        initAggrIterIfNull();
        aggrIter.rewind();
    }

    /**
     * Returns the TupleDesc of this Aggregate.
     * If there is no group by field, this will have one field - the aggregate column.
     * If there is a group by field, the first field will be the group by field, and the second
     * will be the aggregate value column.
     * 
     * The name of an aggregate column should be informative.  For example:
     * "aggName(aop) (child_td.getFieldName(afield))"
     * where aop and afield are given in the constructor, and child_td is the TupleDesc
     * of the child iterator. 
     */
    public TupleDesc getTupleDesc() {
        return aggregator.getTupleDesc();
    }

    public void close() {
        if (aggrIter != null) {
            aggrIter.close();
        }
    }
}
