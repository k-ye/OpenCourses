package simpledb;
import java.util.*;

/**
 * TupleDesc describes the schema of a tuple.
 */
public class TupleDesc {
    private final List<Type> types;
    private final int byteSize;
    private final List<String> fieldNames;
    /**
     * Merge two TupleDescs into one, with td1.numFields + td2.numFields
     * fields, with the first td1.numFields coming from td1 and the remaining
     * from td2.
     * @param td1 The TupleDesc with the first fields of the new TupleDesc
     * @param td2 The TupleDesc with the last fields of the TupleDesc
     * @return the new TupleDesc
     */
    public static TupleDesc combine(TupleDesc td1, TupleDesc td2) {
        // some code goes here
        Type[] typeAr = new Type[td1.types.size() + td2.types.size()];

        int i = 0;
        for (Type t : td1.types) {
            typeAr[i] = t;
            ++i;
        }
        for (Type t : td2.types) {
            typeAr[i] = t;
            ++i;
        }

        boolean fieldArIsNull = ((td1.fieldNames == null) || (td2.fieldNames == null));
        String[] fieldAr = (fieldArIsNull ? null : new String[typeAr.length]);
        if (fieldAr != null) {
            i = 0;
            for (String f : td1.fieldNames) {
                fieldAr[i] = f;
                ++i;
            }
            for (String f : td2.fieldNames) {
                fieldAr[i] = f;
                ++i;
            }
        }
        return new TupleDesc(typeAr, fieldAr);
    }

    /**
     * Create a new TupleDesc with typeAr.length fields with fields of the
     * specified types, with associated named fields.
     *
     * @param typeAr array specifying the number of and types of fields in
     *        this TupleDesc. It must contain at least one entry.
     * @param fieldAr array specifying the names of the fields. Note that names may be null.
     */
    public TupleDesc(Type[] typeAr, String[] fieldAr) {
        assert(typeAr.length > 0);
        types = Arrays.asList(typeAr);
        int bsz = 0;
        for (Type t : types) {
            bsz += t.getLen();
        }
        byteSize = bsz;

        if (fieldAr == null) {
            fieldNames = null;
        } else {
            assert(typeAr.length == fieldAr.length);
            fieldNames = Arrays.asList(fieldAr);
        }
    }

    /**
     * Constructor.
     * Create a new tuple desc with typeAr.length fields with fields of the
     * specified types, with anonymous (unnamed) fields.
     *
     * @param typeAr array specifying the number of and types of fields in
     *        this TupleDesc. It must contain at least one entry.
     */
    public TupleDesc(Type[] typeAr) {
        this(typeAr, /*fieldAr=*/null);
    }

    /**
     * @return the number of fields in this TupleDesc
     */
    public int numFields() {
        return types.size();
    }

    /**
     * Gets the (possibly null) field name of the ith field of this TupleDesc.
     *
     * @param i index of the field name to return. It must be a valid index.
     * @return the name of the ith field
     * @throws NoSuchElementException if i is not a valid field reference.
     */
    public String getFieldName(int i) throws NoSuchElementException {
        if (fieldNames == null) {
            return null;
        }
        try {
            return fieldNames.get(i);
        } catch (IndexOutOfBoundsException e) {
            throw new NoSuchElementException(e.getMessage());
        }
    }

    /**
     * Find the index of the field with a given name.
     *
     * @param name name of the field.
     * @return the index of the field that is first to have the given name.
     * @throws NoSuchElementException if no field with a matching name is found.
     */
    public int nameToId(String name) throws NoSuchElementException {
        // some code goes here
        if (fieldNames == null) {
            throw new NoSuchElementException("Field names is not set.");
        }
        int result = fieldNames.indexOf(name);
        if (result < 0) {
            throw new NoSuchElementException("Did not find field: " + name);
        }
        return result;
    }

    /**
     * Gets the type of the ith field of this TupleDesc.
     *
     * @param i The index of the field to get the type of. It must be a valid index.
     * @return the type of the ith field
     * @throws NoSuchElementException if i is not a valid field reference.
     */
    public Type getType(int i) throws NoSuchElementException {
        try {
            return types.get(i);
        } catch (IndexOutOfBoundsException e) {
            throw new NoSuchElementException(e.getMessage());
        }
    }

    /**
     * @return The size (in bytes) of tuples corresponding to this TupleDesc.
     * Note that tuples from a given TupleDesc are of a fixed size.
     */
    public int getSize() {
        return byteSize;
    }

    /**
     * Compares the specified object with this TupleDesc for equality.
     * Two TupleDescs are considered equal if they are the same size and if the
     * n-th type in this TupleDesc is equal to the n-th type in td.
     *
     * @param o the Object to be compared for equality with this TupleDesc.
     * @return true if the object is equal to this TupleDesc.
     */
    public boolean equals(Object o) {
        if (o == null) {
            return false;
        }
        if (this == o) {
            return true;
        }
        if (!getClass().equals(o.getClass())) {
            return false;
        }
        TupleDesc otd = (TupleDesc) o;
        if (types.size() != otd.types.size()) {
            return false;
        }
        for (int i = 0; i < types.size(); ++i) {
            if (!getType(i).equals(otd.getType(i))) {
                return false;
            }
        }
        return true;
    }

    public int hashCode() {
        int result = 0xdbca143 ^ types.size();
        for (Type t : types) {
            result = ((result * 73) ^ t.hashCode()) + ((result * 1493) ^ 2011);
        }
        return result;
    }

    /**
     * Returns a String describing this descriptor. It should be of the form
     * "fieldType[0](fieldName[0]), ..., fieldType[M](fieldName[M])", although
     * the exact format does not matter.
     * @return String describing this descriptor.
     */
    public String toString() {
        StringJoiner sj = new StringJoiner(", ");
        for (Type t : types) {
            sj.add(t.toString());
        }
        return sj.toString();
    }
}
