package net.prj3.concurrent;

public interface Disposable {
    void dispose();

    public interface Listener<T> {
        void onDispose(t ctx);
    }
}
