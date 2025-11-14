本人于2025.10.20完成该lab。

这个lab需要我们模拟shell中对多进程的处理，完善tsh.c的部分函数，使其能正确处理多进程的功能。这个lab的源代码并不难读，题目给出的注释也非常详尽，书上的内容完全足够完成这个lab。

Hint的第一条还是非常重要的，书上给出了所有需要用到的函数的使用说明。

>Read every word of Chapter 8 (Exceptional Control Flow) in your textbook.

这个lab主要就是理清思路，注意在恰当的地方调用sigprocmask函数进行原子操作，之后再对照tshref的输出校正自己代码的实现就可以了。