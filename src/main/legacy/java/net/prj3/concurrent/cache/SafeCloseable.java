package net.prj3.concurrent.cache;

public interface SafeCloseable extends AutoCloseable {
    void close();
}
