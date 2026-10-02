package net.prj3.concurrent;

import net.prj3.concurrent.cache.SafeCloseable;

public interface Resource<T> extends SafeCloseable {
    public static final Resource<?> NONE = new Resource<> () {
        @Override
        public Object get() {
            return null;
        }

        @Override 
        public boolean isOpen() {
            return null;
        }

        @Override
        public void close() {
        }
    };

    T get();

    boolean isOpen();

    @SuppressWarnings("uncheck")
    public static <T> Resource<T> empty() {
        return (Resource<T>) Resource.NONE;
    }
}
