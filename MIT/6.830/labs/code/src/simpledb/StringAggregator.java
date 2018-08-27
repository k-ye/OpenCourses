package simpledb;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;

/**
 * Knows how to compute some aggregate over a set of StringFields.
 *
 * TODO(k-ye): Tons of duplicate code!
 */
public class StringAggregator implements Aggregator {
    private final int gbFieldIdx;
    private final Type gbFieldType;
    private final int aggrFieldIdx;

    private Integer singleAggrVal;
    private final Map<Field, Integer> groupToVals;
    private final TupleDesc resultTd;

    /**
     * Aggregate constructor
     * @param gbfield the 0-based index of the group-by field in the tuple, or NO_GROUPING if there is no grouping
     * @param gbfieldtype the type of the group by field (e.g., Type.INT_TYPE), or null if there is no grouping
     * @param afield the 0-based index of the aggregate field in the tuple
     * @param what aggregation operator to use -- only supports COUNT
     * @throws IllegalArgumentException if what != COUNT
     */

    public StringAggregator(int gbfield, Type gbfieldtype, int afield, Op what) {
        assert(afield != Aggregator.NO_GROUPING);
        if (what != Op.COUNT) {
            throw new IllegalArgumentException("StringAggregator only supports COUNT");
        }

        this.gbFieldIdx = gbfield;
        this.gbFieldType = gbfieldtype;
        this.aggrFieldIdx = afield;

        if (hasGroupBy()) {
            groupToVals = new HashMap<>();
            singleAggrVal = null;
            resultTd = new TupleDesc(new Type[]{gbfieldtype, Type.INT_TYPE}, new String[]{"groupBy", "aggregate"});
        } else {
            groupToVals = null;
            singleAggrVal = 0;
            resultTd = new TupleDesc(new Type[]{Type.INT_TYPE}, new String[]{"aggregate"});
        }
    }

    private boolean hasGroupBy() {
        return gbFieldIdx != Aggregator.NO_GROUPING;
    }

    @Override
    public TupleDesc getTupleDesc() {
        return resultTd;
    }

    /**
     * Merge a new tuple into the aggregate, grouping as indicated in the constructor
     * @param tup the Tuple containing an aggregate field and a group-by field
     */
    @Override
    public void merge(Tuple tup) {
        assert(tup.getField(aggrFieldIdx).getType().equals(Type.STRING_TYPE));

        if (hasGroupBy()) {
            Field gbField = tup.getField(gbFieldIdx);
            assert(gbField.getType().equals(gbFieldType));
            int s = groupToVals.getOrDefault(gbField, 0);
            groupToVals.put(gbField, s + 1);
        } else {
            singleAggrVal += 1;
        }
    }

    /**
     * Create a DbIterator over group aggregate results.
     *
     * @return a DbIterator whose tuples are the pair (groupVal,
     *   aggregateVal) if using group, or a single (aggregateVal) if no
     *   grouping. The aggregateVal is determined by the type of
     *   aggregate specified in the constructor.
     */
    @Override
    public AbstractDbIterator iterator() {
        if (hasGroupBy()) {
            return new GbResultIter();
        }
        return new SingelValIter();
    }

    private class GbResultIter extends AbstractDbIterator {
        Iterator<Map.Entry<Field, Integer>> iter;

        public GbResultIter() {
            iter = null;
        }

        @Override
        public TupleDesc getTupleDesc() {
            return resultTd;
        }

        @Override
        public void open()
                throws DbException, NoSuchElementException, TransactionAbortedException {
            rewind();
        }

        @Override
        public void close() {
            iter = null;
        }

        @Override
        public void rewind() throws DbException, TransactionAbortedException {
            iter = groupToVals.entrySet().iterator();
        }

        @Override
        protected Tuple readNext() throws DbException, TransactionAbortedException {
            if ((iter != null) && iter.hasNext()) {
                Map.Entry<Field, Integer> cur = iter.next();
                Tuple result = new Tuple(getTupleDesc());
                result.setField(0, cur.getKey());
                result.setField(1, new IntField(cur.getValue()));
                return result;
            }
            return null;
        }
    }

    private class SingelValIter extends AbstractDbIterator {
        private boolean hasItered;
        private final Tuple result;

        public SingelValIter() {
            hasItered = true;
            result = new Tuple(getTupleDesc());
            result.setField(0, new IntField(singleAggrVal));
        }

        @Override
        public TupleDesc getTupleDesc() {
            return resultTd;
        }

        @Override
        public void open()
                throws DbException, NoSuchElementException, TransactionAbortedException {
            rewind();
        }

        @Override
        public void close() {
            hasItered = true;
        }

        @Override
        public void rewind() throws DbException, TransactionAbortedException {
            hasItered = false;
        }

        @Override
        protected Tuple readNext() throws DbException, TransactionAbortedException {
            if (hasItered) {
                return null;
            }
            hasItered = true;
            return result;
        }
    }
}
