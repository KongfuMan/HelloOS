# HelloOS
HelloOS is a mini operating system kernel that support:
- Preemptive task 
- Address space
- IO device management
- ...

#### build the kernel code:
```shell
 make build
```

#### run kernel from the virtual machine
```shell
make VBOXRUN
```

#### re-build code and start the kernel from the virtual machine:
```shell
make vboxtest
```


### Configure the virtualbox:
We use linux OS as our development environment, and virtualbox to simulate hardware bootstrap.
1. Download and install [virtualbox](https://www.virtualbox.org/wiki/Downloads)
2. Create OS image file:
    ```shell
    dd bs=512 if=/dev/zero of=hd.img count=204800
    ```
   > bs:	表示block size;<br>
   > if：	表示输入文件(intput file)，/dev/zero就是Linux下专门返回0数据的设备文件，读取它就返回0<br>
   > of：表示输出文件(output file)，即我们的硬盘文件。<br>
   > count：表示number of output blocks. 这里文件大小= 204800*512Byte = 100MB
3. format the file, install `ext4` into the file
    ```shell
    sudo losetup /dev/loop0 hd.img
    sudo mkfs.ext4 -q /dev/loop0  #安装ext4文件系统到loop0
    sudo mount -o loop ./hd.img ./hdisk/  #挂载硬盘文件,创建hdisk if not exist
    ```

 4. install grub on the image file
    ```shell
    sudo grub-install --boot-directory=./hdisk/boot/ --force --allow-floppy /dev/loop0
    ```
    > --boot-directory 指向先前我们在虚拟硬盘中建立的boot目录。<br>
    --force --allow-floppy 指向我们的虚拟硬盘设备文件/dev/loop0
    
 4. 在./hdisk/boot/grub/目录下创建grub.cfg 文本文件，GRUB 正是通过这个文件内容，查找到我们的操作系统映像文件的。
    ```
    menuentry 'Cosmos' {
      insmod part_msdos
      insmod ext2
      set root='hd0'
      multiboot2 /boot/Cosmos.eki #加载boot目录下的Cosmos.eki文件
      boot #引导启动
    }
    set timeout_style=menu
    if [ "${timeout}" = 0 ]; then
    set timeout=10 #等待10秒钟自动启动
    fi
    ```
 5. Package multiple bin files into a single eki file
    ```shell
    lmoskrlimg -m k -lhf initldrimh.bin -o Cosmos.eki -f initldrkrl.bin initldrsve.bin
    ```

 6. Copy Cosmos.eki file into ./hdisk/boot/ folder
    ```shell
    sudo cp Cosmos.eki ./hdisk/boot/
    ```
    
 7. Detach the loop device
    ```shell
    sudo losetup --detach /dev/loop0
    ```
     
 8. unmount ./hdisk
    ```shell
    sudo umount ./hdisk/
    ```
     
 9. Convert .img format into .vdi
    ```shell
    VBoxManage convertfromraw ./hd.img --format VDI ./hd.vdi
    ```

### Kernel Boot Sequence:
1、grub启动后，选择对应的启动菜单项，grub会通过自带文件系统驱动，定位到对应的[Cosmos.eki]文件

2、grub会尝试加载eki文件【eki文件需要满足grub多协议引导头的格式要求】
这些是在imginithead.asm中实现的，所以要包括：
- A、grub文件头，包括魔数、grub1和grub2支持等
- B、定位的_start符号等

3、grub校验成功后，会调用_start，然跳转到_entry
- A、_entry中:关闭中断
- B、加载GDT
- C、然后进入_32bits_mode，清理寄存器，设置栈顶
- D、调用inithead_entry() in `inithead.c`

4、inithead_entry():
- A、从imginithead.asm进入后，首先进入函数调用inithead_entry 
- B、初始化光标，清屏
- C、从eki文件内部，找到initldrsve.bin文件，并分别拷贝到内存的指定物理地址
- D、从eki文件内部，找到initldrkrl.bin文件，并分别拷贝到内存的指定物理地址
- E、返回imginithead.asm

5、imginithead.asm中继续执行
- jmp 0x200000. 而这个位置，就是initldrkrl.bin在内存的位置ILDRKRL_PHYADR
- 所以后面要执行initldrkrl.bin的内容

6、这样就到了ldrkrl32.asm的_entry
- A、将GDT加载到GDTR寄存器【内存】
- B、将IDT加载到IDTR寄存器【中断】
- C、跳转到_32bits_mode
    - 初始寄存器
    - 初始化栈
    - 调用ldrkrl_entry() in `ldrkrlentry.c` 

7、ldrkrl_entry()
- A、初始化光标，清屏
- B、收集机器参数 init_bstartparm() in `bstartparm.c`

8、init_bstartparm():
- A、初始化machbstart_t
- B、各类初始化函数，填充machbstart_t的内容
  
  - machbstart_t *mbsp = MBSPADR;
  - machbstart_t_init(mbsp);
  - init_chkcpu(mbsp);
  - init_mem(mbsp);

  - init_krlinitstack(mbsp);
  - init_krlfile(mbsp);
  - init_defutfont(mbsp);
  - init_meme820(mbsp);
  - init_bstartpages(mbsp);
  - init_graph(mbsp);
- C、返回

9、ldrkrlentry.c
- A、return from ldrkrl_entry();

10、ldrkrl32.asm
- A、跳转到`IMGKRNL_PHYADR`地址继续执行: `Cosmos.bin`
- B. Execute `init_entry.asm` file under `hal` layer
- C. Call kernel main function: `hal_start()`