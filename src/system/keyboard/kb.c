#include <system.h>

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <libc/string.h>
#include <libc/stdio.h>


#define VGA_ATTRIBUTE  0xf
#define VGA_BLANK ((VGA_ATTRIBUTE << 8) | 0)

short *VGA_text_buf = 0xB8000;

/* KBDUS means US Keyboard Layout. This is a scancode table
*  used to layout a standard US keyboard. I have left some
*  comments in to give you an idea of what key is what, even
*  though I set it's array index to 0. You can change that to
*  whatever you want using a macro, if you wish! */
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0x80,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0x81,	/* Left Arrow */
    0,
    0x82,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0x83,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

volatile int kb_receiving_input = 1;
char kb_command[80];
volatile char cur_char;

/* Handles the keyboard interrupt */
void keyboard_handler(struct regs *r)
{
  if(kb_receiving_input == 0) return;

  if(inportb(0x64) & 1){
    unsigned char scancode;

    /* Read from the keyboard's data buffer */
    scancode = inportb(0x60);
    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    if (scancode & 0x80)
    {
      /* You can use this one to see if the user released the
      *  shift, alt, or control keys... */
    }
    else
    {
      // put character that has been pressed
      if(kbdus[scancode] != 0){
        switch(kbdus[scancode]){

          case('\b'):{
            VGA_backspace();
            cur_char = '\b';

            break;
          }

          case('\n'):{
            putc('\n');
            cur_char = '\n';
            break;
          }

          default:
            if((VGA_crsr_pos() % 80) < 79){
              putc(kbdus[scancode]);
              cur_char = kbdus[scancode];

            }
            break;
        }
      }
    }
  }
  interrupt_done(0);

  kb_receiving_input = 0;
  return;
}

char getChar(void)
{
  kb_receiving_input = 1;
  while(kb_receiving_input == 1);
  char ret_val = cur_char;
  cur_char = 0;
  return ret_val;
}


void keyboard_install(void)
{
  irq_install_handler(1, keyboard_handler);
}
