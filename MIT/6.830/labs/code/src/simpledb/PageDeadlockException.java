package simpledb;

public class PageDeadlockException extends Exception {
    public PageDeadlockException() {
        super();
    }

    public PageDeadlockException(String message) {
        super(message);
    }
}
