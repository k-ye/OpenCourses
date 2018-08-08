package simpledb;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;

/**
 * Knows how to compute some aggregate over a set of IntFields.
 */
public class IntAggregator implements Aggregator {

    /**
     * Aggregate constructor
     * @param gbfield the 0-based index of the group-by field in the tuple, or {@code Aggregator.NO_GROUPING} if there is no grouping
     * @param gbfieldtype the type of the group by field (e.g., Type.INT_TYPE), or null if there is no grouping
     * @param afield the 0-based index of the aggregate field in the tuple
     * @param what the aggregation operator
     */
    private final int gbFieldIdx;
    private final Type gbFieldType;
    private final int aggrFieldIdx;
    private final Op what;

    private final AggrRecord singleAggrVal;
    private final Map<Field, AggrRecord> groupToVals;
    private final TupleDesc resultTd;

    public IntAggregator(int gbfield, Type gbfieldtype, int afield, Op what) {
        assert(afield != Aggregator.NO_GROUPING);

        this.gbFieldIdx = gbfield;
        this.gbFieldType = gbfieldtype;
        this.aggrFieldIdx = afield;
        this.what = what;

        if (hasGroupBy()) {
            groupToVals = new HashMap<>();
            singleAggrVal = null;
            resultTd = new TupleDesc(new Type[]{gbfieldtype, Type.INT_TYPE}, new String[]{"groupBy", "aggregate"});
        } else {
            groupToVals = null;
            singleAggrVal = makeAggrRecord();
            resultTd = new TupleDesc(new Type[]{Type.INT_TYPE}, new String[]{"aggregate"});
        }
    }

    private boolean hasGroupBy() {
        return gbFieldIdx != Aggregator.NO_GROUPING;
    }

    private AggrRecord makeAggrRecord() {
        switch (what) {
            case MIN: case MAX:
                return new MinMaxRecord(what == Op.MIN);
            case AVG: case SUM: case COUNT:
                return new AvgSumCountRecord(what);
            default:
                throw new RuntimeException("Unknown Op " + what.toString());
        }
    }

    private interface AggrRecord {
        void handle(int x);
        int result();
    }

    private static class MinMaxRecord implements AggrRecord {
        private int m;
        private final boolean getMin;

        public MinMaxRecord(boolean getMin) {
            this.getMin = getMin;
            this.m = (getMin ? Integer.MAX_VALUE : Integer.MIN_VALUE);
        }

        @Override
        public void handle(int x) {
            if (getMin) {
                m = Integer.min(m, x);
            } else {
                m = Integer.max(m, x);
            }
        }

        @Override
        public int result() {
            return m;
        }
    }

    private static class AvgSumCountRecord implements AggrRecord {
        private int s;
        private int n;
        private final Op op;

        public AvgSumCountRecord(Op op) {
            this.s = 0;
            this.n = 0;
            this.op = op;
        }

        @Override
        public void handle(int x) {
            s += x;
            n += 1;
        }

        @Override
        public int result() {
            switch (op) {
                case AVG:
                    return (s / n);
                case SUM:
                    return s;
                case COUNT:
                    return n;
                default:
                    throw new RuntimeException("Unexpected op " + op.toString());
            }
        }
    }

    /**
     * Merge a new tuple into the aggregate, grouping as indicated in the constructor
     * @param tup the Tuple containing an aggregate field and a group-by field
     */
    public void merge(Tuple tup) {
        Field aggrField = tup.getField(aggrFieldIdx);
        assert(aggrField.getType().equals(Type.INT_TYPE));
        int val = ((IntField) aggrField).getValue();

        if (hasGroupBy()) {
            Field gbField = tup.getField(gbFieldIdx);
            assert(gbField.getType().equals(gbFieldType));
            groupToVals.computeIfAbsent(gbField, f -> makeAggrRecord());
            groupToVals.get(gbField).handle(val);
        } else {
            singleAggrVal.handle(val);
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
    public AbstractDbIterator iterator() {
        if (hasGroupBy()) {
            return new GbResultIter();
        }
        return new SingelValIter();
    }

    private class GbResultIter extends AbstractDbIterator {
        Iterator<Map.Entry<Field, AggrRecord>> iter;

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
                Map.Entry<Field, AggrRecord> cur = iter.next();
                Tuple result = new Tuple(getTupleDesc());
                result.setField(0, cur.getKey());
                result.setField(1, new IntField(cur.getValue().result()));
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
            result.setField(0, new IntField(singleAggrVal.result()));
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
