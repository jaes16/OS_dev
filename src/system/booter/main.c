#include <system.h>

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <libc/string.h>
#include <libc/stdio.h>

#include <multiboot.h>

#include <mmngr_virtual.h>

#include <ata.h>


// inline assembly for reading from port
unsigned char inportb (unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

// inline assembly for writing out to port
void outportb (unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

// inline assembly for reading from port
unsigned short inportw (unsigned short _port)
{
    unsigned short rv;
    __asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

// inline assembly for writing out to port
void outportw (unsigned short _port, unsigned short _data)
{
    __asm__ __volatile__ ("outw %1, %0" : : "dN" (_port), "a" (_data));
}



int start_input(void)
{
  char buf[80];

  memset(buf, 0, 80);
  int index = 0;

  int ret_val = 1;

  puts("\n>");
  int looper = 1;
  while(looper == 1){

    char c = getChar();

    if(c == '\b'){
      if(index > 0){
        index--;
        buf[index] = 0;
      }
    }
    else if(c == '\n'){
      char temp[80];
      memset(temp, 0, 80);
      int j = 0;
      for(int i = 0; i < 80; i++){
        if (buf[i] != 0) temp[j++] = buf[i];
      }
      if(terminal_command(temp) == 0) ret_val = 0;
      looper = 0;
    }
    else if((index < 79) && (c != 0)){
      buf[index] = c;
      index++;
    }
  }
  return ret_val;
}


int terminal_command(char *command)
{
  if(strcmp(command, "echo") == 0) printf("\nEcho is a figure from Greek mythology.\n");
  else if(strcmp(command, "quit") == 0){
    printf("\nQuitting...\n");
    return 0;
  }
  else printf("Command not recognized: %s", command);
  return 1;
}


/*/////////////////////////////////////////////////////////////////////////////
/////////////////// MAIN FUNCTION: STARTS HERE ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////*/
void _main(multiboot_info_t* mbd, unsigned long kernel_end_addr, unsigned long kernel_start, unsigned int magic)
{
  // if magic number for multiboot is incorrect, halt. something don goofed
  if(magic != 0x2BADB002){
    // init_video();
    // puts("Incorrect magic number.");
    __asm__ __volatile__ ("hlt");
    for(;;);
  }



  // initialize VGA; gdt; idt, irq, isrs; timer; keyboard input
  gdt_install();
  idt_install();
  irq_install();
  isrs_install();
  timer_install();
  keyboard_install();
  init_video();


  printf("Kernel initiated, kernel size: %x, kernel start address: %x, kernel end address: %x.\n\n",
              kernel_end_addr - kernel_start, kernel_start, kernel_end_addr);


  // then start interrupts so those installs above have an effect

  // start reading multiboot info to get memory size and locations
  struct multiboot_mmap_entry *m_region = (struct multiboot_mmap_entry *) mbd->mmap_addr;
  // mem_lower is return in kilobytes and mem_upper is returned in a multiple of 64 kilobytes
  int phys_mem_size = mbd->mem_lower + (mbd->mem_upper * 64); // leave it in kilobytes because we want ints
  pmmngr_init(phys_mem_size, kernel_end_addr);

  //pmmngr_init_region(0xB8000, 4000); // VGA location
  //char *b = pmmngr_alloc_block();

  printf("Memory size is %x KB: Memory map returned by GRUB multiboot info:\n", phys_mem_size);
  // free those regions that grub said are free
  for(int i = 0; m_region[i].size > 0; i++){
    printf("Region %d: Address: %x, Length: %x, Type: %d.\n",
                i, (uint32_t) m_region[i].addr, (uint32_t) m_region[i].len, m_region[i].type);
    if(m_region[i].type == 1){
      pmmngr_init_region(m_region[i].addr, m_region[i].len);
    }
  }
  putc('\n');

  // set as used: memorymap used for physical_memory manager; kernel space;
  pmmngr_deinit_region(kernel_start, kernel_end_addr-kernel_start); // deep space, outer space, inner space, kernel space. I'm no astrophysicist
  pmmngr_deinit_region(kernel_end_addr, (phys_mem_size/PMMNGR_BLOCK_SIZE)/PMMNGR_BLOCKS_PER_BYTE); // space for physical memory manager


  vmmngr_initialize();
  ATA_PIO_init();

  volatile int main_looper = 1;
  while(main_looper){
    if(start_input() == 0) main_looper = 0;
  }

  for (;;);
}
