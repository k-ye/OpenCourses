package simpledb;

import java.util.*;

public class AggregatorOpFactory {
    private static AggregatorOpFactory instance_ = new AggregatorOpFactory();
    private AggregatorOpFactory() {}

    public static abstract class OpBase {
        protected final int afield;

        public OpBase(int afield) {
            this.afield = afield;
        }

        public abstract void merge(Tuple t);

        public abstract DbIterator getResult();

        protected abstract Iterable<Tuple> getInternalResult();

        protected DbIterator toDbIterator(TupleDesc td) {
            class Impl implements DbIterator {
                private Iterator<Tuple> curIter;
                private boolean closed;
                private TupleDesc td;

                public Impl(TupleDesc td) {
                    curIter = null;
                    closed = true;
                    this.td = td;
                }

                @Override
                public void open()
                        throws DbException, TransactionAbortedException {
                    curIter = getInternalResult().iterator();
                    closed = false;
                }

                @Override
                public boolean hasNext() throws DbException, TransactionAbortedException {
                    if (closed) return false;
                    return curIter.hasNext();
                }

                @Override
                public Tuple next() throws DbException, TransactionAbortedException, NoSuchElementException {
                    if (!hasNext()) {
                        throw new NoSuchElementException("No more");
                    }
                    return curIter.next();
                }

                @Override
                public void rewind() throws DbException, TransactionAbortedException {
                    open();
                }

                @Override
                public TupleDesc getTupleDesc() {
                    return td;
                }

                @Override
                public void close() {
                    curIter = null;
                    closed = true;
                }

            }
            return new Impl(td);
        }
    }

    private static int extractIntFieldVal(Tuple t, int afield) {
        Field tmp = t.getField(afield);
        assert tmp instanceof IntField;
        IntField af = (IntField)tmp;
        return af.getValue();
    }

    private static Iterable<Tuple> makeSingleResult(int val) {
        TupleDesc td = TupleDesc.fromTypes(Type.INT_TYPE);
        Tuple t = new Tuple(td);
        t.setField(0, new IntField(val));
        ArrayList<Tuple> result = new ArrayList<Tuple>();
        result.add(t);
        return result;
    }

    public static class CountOp extends OpBase {
        private int num;

        public CountOp(int afield) {
            super(afield);
            num = 0;
        }

        @Override
        public void merge(Tuple t) {
            ++num;
        }

        @Override
        protected Iterable<Tuple> getInternalResult() {
            return makeSingleResult(num);
        }

        @Override
        public DbIterator getResult() {
            return toDbIterator(TupleDesc.fromTypes(Type.INT_TYPE));
        }
    }

    public static class AverageOp extends OpBase {
        private int sum;
        private int num;

        public AverageOp(int afield) {
            super(afield);
            sum = 0;
            num = 0;
        }

        @Override
        public void merge(Tuple t) {
            sum += extractIntFieldVal(t, afield);
            ++num;
        }

        @Override
        public Iterable<Tuple> getInternalResult() {
            return makeSingleResult(sum / num);
        }

        @Override
        public DbIterator getResult() {
            return toDbIterator(TupleDesc.fromTypes(Type.INT_TYPE));
        }
    }

    public static class SumOp extends OpBase {
        private int sum;

        public SumOp(int afield) {
            super(afield);
            sum = 0;
        }

        @Override
        public void merge(Tuple t) {
            sum += extractIntFieldVal(t, afield);
        }

        @Override
        public Iterable<Tuple> getInternalResult() {
            return makeSingleResult(sum);
        }

        @Override
        public DbIterator getResult() {
            return toDbIterator(TupleDesc.fromTypes(Type.INT_TYPE));
        }
    }

    public static class MinOp extends OpBase {
        private int curMin;

        public MinOp(int afield) {
            super(afield);
            curMin = Integer.MAX_VALUE;
        }

        @Override
        public void merge(Tuple t) {
            int val = extractIntFieldVal(t, afield);
            curMin =  ((val < curMin) ? val : curMin);
        }

        @Override
        public Iterable<Tuple> getInternalResult() {
            return makeSingleResult(curMin);
        }

        @Override
        public DbIterator getResult() {
            return toDbIterator(TupleDesc.fromTypes(Type.INT_TYPE));
        }
    }

    public static class MaxOp extends OpBase {
        private int curMax;

        public MaxOp(int afield) {
            super(afield);
            curMax = Integer.MIN_VALUE;
        }

        @Override
        public void merge(Tuple t) {
            int val = extractIntFieldVal(t, afield);
            curMax =  ((val > curMax) ? val : curMax);
        }

        @Override
        public Iterable<Tuple> getInternalResult() {
            return makeSingleResult(curMax);
        }

        @Override
        public DbIterator getResult() {
            return toDbIterator(TupleDesc.fromTypes(Type.INT_TYPE));
        }
    }

    private static OpBase createLeafOp(int afield, Aggregator.Op op) {
        switch (op) {
            case MIN:
                return new MinOp(afield);
            case MAX:
                return new MaxOp(afield);
            case AVG:
                return new AverageOp(afield);
            case SUM:
                return new SumOp(afield);
            case COUNT:
                return new CountOp(afield);
        }
        return null;
    }

    public static class GroupByOp extends OpBase {
        private int gbfield;
        private Type gbfieldtype;
        private Aggregator.Op op;
        private HashMap<Field, OpBase> groupMap;

        public GroupByOp(int gbfield, Type gbfieldtype, int afield, Aggregator.Op op) {
            super(afield);
            this.gbfield = gbfield;
            this.gbfieldtype = gbfieldtype;
            this.op = op;
            this.groupMap = new HashMap<Field, OpBase>();
        }

        @Override
        public void merge(Tuple t) {
            Field gbf = t.getField(gbfield);
            if (!groupMap.containsKey(gbf)) {
                groupMap.put(gbf, createLeafOp(afield, op));
            }
            groupMap.get(gbf).merge(t);
        }

        @Override
        public Iterable<Tuple> getInternalResult() {
            Type[] typeAr = {gbfieldtype, Type.INT_TYPE};
            TupleDesc td = new TupleDesc(typeAr, null);
            ArrayList<Tuple> result = new ArrayList<Tuple>();
            for (Map.Entry<Field, OpBase> entry : groupMap.entrySet()) {
                Tuple t = new Tuple(td);
                t.setField(0, entry.getKey());
                Field af = entry.getValue().getInternalResult().iterator().next().getField(0);
                t.setField(1, af);
                result.add(t);
            }
            return result;
        }

        @Override
        public DbIterator getResult() {
            return toDbIterator(TupleDesc.fromTypes(gbfieldtype, Type.INT_TYPE));
        }
    }

    public static OpBase createOp(int gbfield, Type gbfieldtype, int afield, Aggregator.Op op) {
        if (gbfield == Aggregator.NO_GROUPING) {
            return createLeafOp(afield, op);
        }
        return new GroupByOp(gbfield, gbfieldtype, afield, op);
    }
}
