package com.Jsample;


import javax.sound.midi.Soundbank;
import java.io.PrintStream;
import java.security.PublicKey;
import java.sql.SQLOutput;
import java.util.Date;
import java.util.StringTokenizer;

import static java.lang.Math.*;

/**
 * 这个是一个测试自动生成文档的注释格式。
 * 测试中文注释
 * @version 0.001 2018-09-05
 * @author LinCong
 */



public class HelloWorld {
    public static final double CM_PER_INCH = 2.54;
    public static void main(String[] args) {
/*
String greeting ="Welcome to Core Java!";

System.out.println(greeting);
for (int i = 0; i < greeting.length(); i++) {
System.out.print("=");
}
System.out.println();
System.out.println("Hello Linc");

if (!Double.isNaN(5.008))
System.out.println("这是一个数字");
else System.out.println("这是一个数字");
*/
//
//        double paperWidth = 8.5;
//        double paperHeight = 11;
//        double x = 4;
//        double a = 2;
//        double y = Math.sqrt(x);
//        y = Math.pow(x, a);
//        System.out.println(y);
//        System.out.println("Paper size in centimeters: "
//                + paperWidth * CM_PER_INCH + " by " + paperHeight * CM_PER_INCH);

//        double y = pow(2, 5);
//        System.out.println(y);
//        System.out.println((-y+1)%2);
//        y = log(E);
//        System.out.println(y);
//        System.out.println(exp(2));
//        System.out.println(PI);
//        double s = abs(-5);
//        System.out.println(s);
//        int n = 123456789;
//        float f = n;
//        System.out.println(f);
//        double x = 7.997;
//        int nx = (int) Math.round(x);
//        nx +=3.5;
//        System.out.println(3!=7);
//        double y = 8;
//        double z = x < y ? x : y;
//        System.out.println(z);
//        int n = 8;
//        int fourthBitFromRight = (n & (1 << 3)) >> 3;
//        System.out.println(fourthBitFromRight);//>>>运算符会用0填充高位，>>会用符号位填充高位。，不存在<<<运算符
//        System.out.println(1 << 35);
//        enum Size { SMALL, MEDIUM, LARGE, EXTRA_LARGE };
//        Size s = Size.MEDIUM;
//        System.out.println(s);

//        String e = "";
//        String greeting = "Hello";
//        String s = greeting.substring(0, 3);
//        System.out.println(s);
//        System.out.println(greeting);
//        System.out.println(e);
//        String expletive = "Expletive";
//        String PG13 = "deleted";
//        String message = expletive + PG13;
//        System.out.println(message);
//        int age = 13;
//        String rating = "PG" + age;
//        System.out.println(rating);
//        System.out.println(String.join("-", "S", "M", "L", "XL"));
//        greeting = greeting.substring(0, 3) + "p!";
//        System.out.println(greeting);
//        System.out.println("Hello".equals(greeting));
//        boolean a = "Hello".equalsIgnoreCase("hello");
//        System.out.println(a);
        double x = 10000.0 / 3.0;
        System.out.println(x);
        System.out.printf("%8.2f", x);
        System.out.println();
        System.out.printf("%,.2f", 10000.0 / 3.0);
        System.out.println();
        System.out.printf("%tc", new Date());
        System.out.println();
        PrintStream printf = System.out.printf("%1$s %2$tB %2$te, %2$tY", "Due date:", new Date());
        System.out.println(printf);
    }

}
