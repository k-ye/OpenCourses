package simpledb;

import java.util.ArrayList;
import java.util.List;

/** A class to represent a fixed-width histogram over a single integer-based field.
 */
public class IntHistogram {
    private final int min;
    private final int max;
    private final int width;
    private int numVals;
    private final int[] buckets;
    /**
     * Create a new IntHistogram.
     * 
     * This IntHistogram should maintain a histogram of integer values that it receives.
     * It should split the histogram into "buckets" buckets.
     * 
     * The values that are being histogrammed will be provided one-at-a-time through the "addValue()" function.
     * 
     * Your implementation should use space and have execution time that are both
     * constant with respect to the number of values being histogrammed.  For example, you shouldn't 
     * simply store every value that you see in a sorted list.
     * 
     * @param buckets The number of buckets to split the input value into.
     * @param min The minimum integer value that will ever be passed to this class for histogramming
     * @param max The maximum integer value that will ever be passed to this class for histogramming
     */
    public IntHistogram(int buckets, int min, int max) {
    	// some code goes here
        this.min = min;
        this.max = max + 1;
        buckets = Integer.min(buckets, (this.max - this.min));
        this.width = (this.max - this.min) / buckets;
        this.numVals = 0;
        this.buckets = new int[buckets];
    }

    /**
     * Add a value to the set of values that you are keeping a histogram of.
     * @param v Value to add to the histogram
     */
    public void addValue(int v) {
        if (inRange(v)) {
            numVals += 1;
            int bi = findBucket(v);
            buckets[bi] += 1;
        }
    }

    private boolean inRange(int v) {
        return ((min <= v) && (v < max));
    }

    private int findBucket(int v) {
        assert(inRange(v));
        if (v == (max - 1)) {
            return buckets.length - 1;
        }
        return ((v - min) / width);
    }

    /**
     * Estimate the selectivity of a particular predicate and operand on this table.
     * 
     * For example, if "op" is "GREATER_THAN" and "v" is 5, 
     * return your estimate of the fraction of elements that are greater than 5.
     * 
     * @param op Operator
     * @param v Value
     * @return Predicted selectivity of this particular operator and value
     */
    public double estimateSelectivity(Predicate.Op op, int v) {
        switch (op) {
            case LESS_THAN:
                return estimateLt(v);
            case LESS_THAN_OR_EQ:
                return estimateLt(v + 1);
            case GREATER_THAN:
                return 1.0 - estimateSelectivity(Predicate.Op.LESS_THAN_OR_EQ, v);
            case GREATER_THAN_OR_EQ:
                return 1.0 - estimateSelectivity(Predicate.Op.LESS_THAN, v);
            case EQUALS:
                return estimateSelectivity(Predicate.Op.LESS_THAN_OR_EQ, v) - estimateSelectivity(Predicate.Op.LESS_THAN, v);
            case NOT_EQUALS:
                return 1.0 - estimateSelectivity(Predicate.Op.EQUALS, v);
            default:
                throw new RuntimeException("Unsupported Op");
        }
    }

    private double estimateLt(int v) {
        if (v <= min) {
            return 0.0;
        }
        if (v >= max) {
            return 1.0;
        }

        final int bi = findBucket(v);
        double sum = 0.0;
        for (int i = 0; i < bi; ++i) {
            sum += (double) buckets[i];
        }
        final double bRatio = ((double) v - min - (width * bi)) / width;
        sum += bRatio * buckets[bi];
        return sum / (double) numVals;
    }

    /**
     * @return A string describing this histogram, for debugging purposes
     */
    public String toString() {
        // TODO(k-ye): Add more info
        return String.format("Total %d", numVals);
    }
}
