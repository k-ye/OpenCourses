package simpledb;

import java.util.ArrayList;
import java.util.List;

/** A class to represent a fixed-width histogram over a single integer-based field.
 */
public class IntHistogram {
    private final double min;
    private final double max;
    private final double width;
    private int numVals;
    private final List<Integer> buckets;
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
        this.min = (double) min;
        this.max = (double) max;
        this.width = (this.max - this.min) / buckets;
        this.numVals = 0;
        this.buckets = new ArrayList<>();
        for (int i = 0; i < buckets; ++i) {
            this.buckets.add(0);
        }
    }

    /**
     * Add a value to the set of values that you are keeping a histogram of.
     * @param v Value to add to the histogram
     */
    public void addValue(int v) {
        numVals += 1;

        int bi = findBucket(v);
        int h = buckets.get(bi);
        buckets.set(bi, h + 1);
    }

    private int findBucket(int v) {
        double dv = (double) v;
        double dist = dv - min;
        int index = (int) (Math.floor(dist / width) + 0.01);
        if (index == buckets.size()) {
            // This happens when v == max
            index -= 1;
        }
        assert(getLeft(index) <= dv);
        assert(dv <= getRight(index));
        return index;
    }

    private double getLeft(int bi) {
        return min + (width * bi);
    }

    private double getRight(int bi) {
        return getLeft(bi) + width;
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
            case EQUALS:
                return estimateEq(v);
            case NOT_EQUALS:
                return 1.0 - estimateEq(v);
            case LESS_THAN:
                return estimateLt(v);
            case LESS_THAN_OR_EQ:
                return estimateLe(v);
            case GREATER_THAN:
                return 1.0 - estimateLe(v);
            case GREATER_THAN_OR_EQ:
                return 1.0 - estimateLt(v);
            default:
                throw new RuntimeException("Unsupported Op");
        }
    }

    private double estimateEq(int v) {
        if ((v <= min) || (v >= max)) {
            return 0.0;
        }

        int bi = findBucket(v);
        double h = (double) buckets.get(bi);
        return (h / width) / (double) numVals;
    }

    private double estimateLt(int v) {
        if (v <= min) {
            return 0.0;
        }
        if (v >= max) {
            return 1.0;
        }

        final int bi = findBucket(v);
        final double bRatio = ((double) v - getLeft(bi)) / width;
        double sum = bRatio * buckets.get(bi);
        for (int i = 0; i < bi; ++i) {
            sum += (double) buckets.get(i);
        }
        return sum / (double) numVals;
    }

    private double estimateLe(int v) {
        return (estimateLt(v) + 0.5 * estimateEq(v));
    }

    /**
     * @return A string describing this histogram, for debugging purposes
     */
    public String toString() {
        // TODO(k-ye): Add more info
        return String.format("Total %d", numVals);
    }
}
