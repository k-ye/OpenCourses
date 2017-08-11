package simpledb;

import java.util.*;

/**
 * Tuple maintains information about the contents of a tuple.
 * Tuples have a specified schema specified by a TupleDesc object and contain
 * Field objects with the data for each field.
 */
public class Tuple {
    private TupleDesc tupleDesc;
    private RecordId recId;
    private final List<Field> fields;

    /**
     * Create a new tuple with the specified schema (type).
     *
     * @param td the schema of this tuple. It must be a valid TupleDesc
     * instance with at least one field.
     */
    public Tuple(TupleDesc td) {
        this.tupleDesc = td;
        this.fields = Arrays.asList(new Field[td.numFields()]);
    }

    /**
     * @return The TupleDesc representing the schema of this tuple.
     */
    public TupleDesc getTupleDesc() {
        return tupleDesc;
    }

    /**
     * @return The RecordId representing the location of this tuple on
     *   disk. May be null.
     */
    public RecordId getRecordId() {
        return recId;
    }

    /**
     * Set the RecordId information for this tuple.
     * @param rid the new RecordId for this tuple.
     */
    public void setRecordId(RecordId rid) {
        this.recId = rid;
    }

    private void validatedFieldIndex(int i) throws NoSuchElementException {
        if (i < 0 || i >= tupleDesc.numFields()) {
            throw new NoSuchElementException("Invalid index: " + i);
        }
    }

    /**
     * Change the value of the ith field of this tuple.
     *
     * @param i index of the field to change. It must be a valid index.
     * @param f new value for the field.
     */
    public void setField(int i, Field f) throws NoSuchElementException {
        validatedFieldIndex(i);
        fields.set(i, f);
    }

    /**
     * @return the value of the ith field, or null if it has not been set.
     *
     * @param i field index to return. Must be a valid index.
     */
    public Field getField(int i) throws NoSuchElementException {
        validatedFieldIndex(i);
        return fields.get(i);
    }

    /**
     * Returns the contents of this Tuple as a string.
     * Note that to pass the system tests, the format needs to be as
     * follows:
     *
     * column1\tcolumn2\tcolumn3\t...\tcolumnN\n
     *
     * where \t is any whitespace, except newline, and \n is a newline
     */
    public String toString() {
        // some code goes here
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < fields.size(); ++i) {
            sb.append(fields.get(i).toString());
            sb.append(((i < fields.size() - 1) ? "\t" : "\n"));
        }
        return sb.toString();
    }
}
