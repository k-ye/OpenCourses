package simpledb;

import java.util.HashMap;
import java.util.Map;

/** TableStats represents statistics (e.g., histograms) about base tables in a query */
public class TableStats {
    
    /**
     * Number of bins for the histogram.
     * Feel free to increase this value over 100,
     * though our tests assume that you have at least 100 bins in your histograms.
     */
    static final int NUM_HIST_BINS = 100;

    private final DbFile table;
    private final TupleDesc tupleDesc;
    private final int ioCostPerPage;
    private final int numTuples;
    private final int numPages;
    private final Map<Integer, IntHistogram> intFieldToHists;
    private final Map<Integer, StringHistogram> strFieldToHists;

    /**
     * Create a new TableStats object, that keeps track of statistics on each column of a table
     * 
     * @param tableid The table over which to compute statistics
     * @param ioCostPerPage The cost per page of IO.  
     * 		                This doesn't differentiate between sequential-scan IO and disk seeks.
     */
    public TableStats (int tableid, int ioCostPerPage) {
        // For this function, you'll have to get the DbFile for the table in question,
    	// then scan through its tuples and calculate the values that you need.
    	// You should try to do this reasonably efficiently, but you don't necessarily
    	// have to (for example) do everything in a single scan of the table.
    	// some code goes here
        this.table = Database.getCatalog().getDbFile(tableid);
        this.tupleDesc = this.table.getTupleDesc();
        this.ioCostPerPage = ioCostPerPage;

        this.intFieldToHists = new HashMap<>();
        this.strFieldToHists = new HashMap<>();
        this.numTuples = generateStats();
        this.numPages = (numTuples * tupleDesc.getSize() + BufferPool.PAGE_SIZE - 1) / BufferPool.PAGE_SIZE;
    }

    private int generateStats() {
        int numTuples = 0;
        class Interval {
            private int min;
            private int max;
            public Interval() {
                this.min = Integer.MAX_VALUE;
                this.max = Integer.MIN_VALUE;
            }

            public void update(int v) {
                min = Integer.min(min, v);
                max = Integer.max(max, v);
            }

            public int getMin() {
                return min;
            }

            public int getMax() {
                return max;
            }
        }
        // Only int fields need to record min/max.
        Map<Integer, Interval> intFieldToIntervals = new HashMap<>();
        for (int i = 0; i < tupleDesc.numFields(); ++i) {
            if (tupleDesc.getType(i) == Type.INT_TYPE) {
                intFieldToIntervals.put(i, new Interval());
            }
        }
        DbFileIterator iter = table.iterator(new TransactionId());
        try {
            // 1. Construct the int interval
            iter.open();
            numTuples = 0;
            while (iter.hasNext()) {
                numTuples += 1;
                Tuple t = iter.next();
                for (Map.Entry<Integer, Interval> kv : intFieldToIntervals.entrySet()) {
                    int val = ((IntField) t.getField(kv.getKey())).getValue();
                    kv.getValue().update(val);
                }
            }

            for (int i = 0; i < tupleDesc.numFields(); ++i) {
                Type type = tupleDesc.getType(i);
                if (type == Type.INT_TYPE) {
                    Interval intv = intFieldToIntervals.get(i);
                    intFieldToHists.put(i, new IntHistogram(NUM_HIST_BINS, intv.getMin(), intv.getMax()));
                } else if (type == Type.STRING_TYPE) {
                    strFieldToHists.put(i, new StringHistogram(NUM_HIST_BINS));
                }
            }
            // 2. Construct the histograms
            iter.rewind();
            while (iter.hasNext()) {
                Tuple t = iter.next();
                for (int i = 0; i < tupleDesc.numFields(); ++i) {
                    Type type = tupleDesc.getType(i);
                    if (type == Type.INT_TYPE) {
                        intFieldToHists.get(i).addValue(((IntField) t.getField(i)).getValue());
                    } else if (type == Type.STRING_TYPE) {
                        strFieldToHists.get(i).addValue(((StringField) t.getField(i)).getValue());
                    }
                }
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        } finally {
            iter.close();
        }
        return numTuples;
    }
    /** 
     * Estimates the
     * cost of sequentially scanning the file, given that the cost to read
     * a page is costPerPageIO.  You can assume that there are no
     * seeks and that no pages are in the buffer pool.
     * 
     * Also, assume that your hard drive can only read entire pages at once,
     * so if the last page of the table only has one tuple on it, it's just as
     * expensive to read as a full page.  (Most real hard drives can't efficiently
     * address regions smaller than a page at a time.)
     * 
     * @return The estimated cost of scanning the table.
     */ 
    public double estimateScanCost() {
        return ioCostPerPage * numPages;
    }

    /** 
     * This method returns the number of tuples in the relation,
     * given that a predicate with selectivity selectivityFactor is
     * applied.
	 *
     * @param selectivityFactor The selectivity of any predicates over the table
     * @return The estimated cardinality of the scan with the specified selectivityFactor
     */
    public int estimateTableCardinality(double selectivityFactor) {
        return (int) (numTuples * selectivityFactor);
    }

    /** 
     * Estimate the selectivity of predicate <tt>field op constant</tt> on the table.
     * 
     * @param field The field over which the predicate ranges
     * @param op The logical operation in the predicate
     * @param constant The value against which the field is compared
     * @return The estimated selectivity (fraction of tuples that satisfy) the predicate
     */
    public double estimateSelectivity(int field, Predicate.Op op, Field constant) {
        Type fieldType = tupleDesc.getType(field);
        if (fieldType == Type.INT_TYPE) {
            return intFieldToHists.get(field).estimateSelectivity(op, ((IntField) constant).getValue());
        } else if (fieldType == Type.STRING_TYPE) {
            return strFieldToHists.get(field).estimateSelectivity(op, ((StringField) constant).getValue());
        }
        throw new RuntimeException("Unknown Type");
    }

}
