package net.dasm;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Target({})
@Retention(RetentionPolicy.CLASS)
public @interface Method {
    @Depreacted String value() default "";

    Class<?>  ret() default void.class;
    String name() default "";
    Class<?>[] args() default {};
}